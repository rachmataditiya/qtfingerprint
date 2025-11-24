package com.arkana.fingerprint.sdk

import android.content.Context
import com.arkana.fingerprint.sdk.config.FingerprintSdkConfig
import com.arkana.fingerprint.sdk.model.*
import com.arkana.fingerprint.sdk.capture.CaptureManager
import com.arkana.fingerprint.sdk.matching.MatchingEngine
import com.arkana.fingerprint.sdk.backend.BackendClient
import com.arkana.fingerprint.sdk.backend.BackendResult
import com.arkana.fingerprint.sdk.cache.SecureCache
import com.arkana.fingerprint.sdk.util.FingerError
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.withContext

/**
 * Main SDK class for fingerprint operations.
 * 
 * Provides clean API for:
 * - Enrollment
 * - Verification (1:1)
 * - Identification (1:N)
 */
/**
 * Enrollment progress data class.
 */
data class EnrollmentProgress(
    val currentScan: Int,
    val totalScans: Int,
    val message: String
)

object FingerprintSdk {
    private var isInitialized = false
    private lateinit var config: FingerprintSdkConfig
    private lateinit var context: Context
    private lateinit var captureManager: CaptureManager
    private lateinit var matchingEngine: MatchingEngine
    private lateinit var backendClient: BackendClient
    private lateinit var secureCache: SecureCache

    // Progress flow for enrollment
    private val _enrollmentProgress = MutableStateFlow<EnrollmentProgress?>(null)
    val enrollmentProgress: StateFlow<EnrollmentProgress?> = _enrollmentProgress.asStateFlow()

    /**
     * Initialize the SDK.
     * 
     * @param config SDK configuration
     * @param context Android Context
     * @throws IllegalStateException if already initialized
     */
    @JvmStatic
    fun init(config: FingerprintSdkConfig, context: Context) {
        android.util.Log.d("FingerprintSdk", "init: Starting SDK initialization...")
        
        if (isInitialized) {
            android.util.Log.w("FingerprintSdk", "init: SDK already initialized")
            throw IllegalStateException("SDK already initialized")
        }

        this.config = config
        this.context = context.applicationContext

        android.util.Log.d("FingerprintSdk", "init: Initializing components...")
        
        // Initialize components
        captureManager = CaptureManager(context)
        matchingEngine = MatchingEngine()
        backendClient = BackendClient(config.backendUrl)
        secureCache = SecureCache(context, config.enableCache)

        android.util.Log.d("FingerprintSdk", "init: Loading native library...")
        
        // Load native library
        try {
            System.loadLibrary("arkana_fprint")
            android.util.Log.d("FingerprintSdk", "init: Native library loaded successfully")
        } catch (e: Exception) {
            android.util.Log.e("FingerprintSdk", "init: Failed to load native library", e)
            throw e
        }

        isInitialized = true
        android.util.Log.d("FingerprintSdk", "init: SDK initialized successfully")
    }

    /**
     * Capture fingerprint once.
     * 
     * @param timeoutMs Timeout in milliseconds
     * @return CaptureResult
     */
    suspend fun captureOnce(timeoutMs: Long = 30000): CaptureResult {
        ensureInitialized()
        return withContext(Dispatchers.IO) {
            captureManager.captureOnce(timeoutMs)
        }
    }

    /**
     * Enroll fingerprint for a user.
     * 
     * @param userId User ID
     * @param finger Finger type (optional)
     * @return EnrollResult
     */
    suspend fun enroll(
        userId: Int,
        finger: Finger = Finger.UNKNOWN
    ): EnrollResult {
        ensureInitialized()
        return withContext(Dispatchers.IO) {
            try {
                _enrollmentProgress.value = null
                
                // Use fp_device_enroll_sync which performs all required scans internally
                // This is a single call that handles all enrollment scans (typically 5)
                _enrollmentProgress.value = EnrollmentProgress(
                    currentScan = 0,
                    totalScans = config.enrollmentScans,
                    message = "Starting enrollment... Place your finger on the scanner..."
                )
                
                val enrollResult = captureManager.enroll()
                when (enrollResult) {
                    is CaptureResult.Success -> {
                        _enrollmentProgress.value = EnrollmentProgress(
                            currentScan = config.enrollmentScans,
                            totalScans = config.enrollmentScans,
                            message = "Enrollment complete, processing template..."
                        )
                        
                        // Template is already in FP3 format from enrollFingerprint()
                        val template = enrollResult.template

                        _enrollmentProgress.value = EnrollmentProgress(
                            currentScan = config.enrollmentScans,
                            totalScans = config.enrollmentScans,
                            message = "Saving to backend..."
                        )

                        // Store in backend
                        val storeResult = backendClient.storeTemplate(userId, template, finger)
                        if (storeResult !is BackendResult.Success) {
                            val errorMsg = if (storeResult is BackendResult.Error && storeResult.message != null) {
                                storeResult.message
                            } else {
                                "Backend error"
                            }
                            android.util.Log.e("FingerprintSdk", "Backend error: $errorMsg")
                            _enrollmentProgress.value = null
                            return@withContext EnrollResult.Error(FingerError.BACKEND_ERROR)
                        }

                        // Cache locally (encrypted)
                        secureCache.store(userId, template)

                        _enrollmentProgress.value = null
                        EnrollResult.Success(userId)
                    }
                    is CaptureResult.Error -> {
                        _enrollmentProgress.value = null
                        return@withContext EnrollResult.Error(enrollResult.error)
                    }
                }
            } catch (e: Exception) {
                _enrollmentProgress.value = null
                EnrollResult.Error(FingerError.UNKNOWN_ERROR)
            }
        }
    }

    /**
     * Verify fingerprint for a user (1:1 matching).
     * 
     * @param userId User ID to verify
     * @param finger Optional finger type. If null, uses the most recent template for the user.
     * @return VerifyResult
     */
    suspend fun verify(userId: Int, finger: Finger? = null): VerifyResult {
        ensureInitialized()
        return withContext(Dispatchers.IO) {
            try {
                android.util.Log.d("FingerprintSdk", "verify: Starting verification for user $userId")
                
                // Ensure device is open before verification
                if (!captureManager.isDeviceOpen()) {
                    android.util.Log.d("FingerprintSdk", "verify: Device not open, opening device...")
                    val openResult = captureManager.ensureDeviceOpen()
                    if (openResult is CaptureResult.Error) {
                        android.util.Log.e("FingerprintSdk", "verify: Failed to open device: ${openResult.error}")
                        return@withContext VerifyResult.Error(openResult.error)
                    }
                }
                
                // Load template (from cache or backend)
                val fingerStr = finger?.name
                android.util.Log.d("FingerprintSdk", "verify: Loading template for user $userId, finger: ${fingerStr ?: "most recent"}")
                // Note: Cache key should include finger, but for simplicity we'll always load from backend when finger is specified
                val template = if (finger == null) {
                    secureCache.load(userId) ?: run {
                        // Load from backend if not in cache
                        android.util.Log.d("FingerprintSdk", "verify: Template not in cache, loading from backend...")
                        val backendResult = backendClient.loadTemplate(userId, null)
                        when (backendResult) {
                            is BackendResult.Template -> {
                                android.util.Log.d("FingerprintSdk", "verify: Template loaded from backend, size: ${backendResult.template.size} bytes")
                                backendResult.template.also {
                                    secureCache.store(userId, it)
                                }
                            }
                            else -> {
                                android.util.Log.e("FingerprintSdk", "verify: Template not found for user $userId")
                                return@withContext VerifyResult.Error(FingerError.TEMPLATE_NOT_FOUND)
                            }
                        }
                    }
                } else {
                    // Load from backend when finger is specified
                    android.util.Log.d("FingerprintSdk", "verify: Loading template from backend for specific finger...")
                    val backendResult = backendClient.loadTemplate(userId, fingerStr)
                    when (backendResult) {
                        is BackendResult.Template -> {
                            android.util.Log.d("FingerprintSdk", "verify: Template loaded from backend, size: ${backendResult.template.size} bytes")
                            backendResult.template
                        }
                        else -> {
                            android.util.Log.e("FingerprintSdk", "verify: Template not found for user $userId, finger: $fingerStr")
                            return@withContext VerifyResult.Error(FingerError.TEMPLATE_NOT_FOUND)
                        }
                    }
                }

                if (template.isEmpty()) {
                    android.util.Log.e("FingerprintSdk", "verify: Template is empty")
                    return@withContext VerifyResult.Error(FingerError.TEMPLATE_NOT_FOUND)
                }

                android.util.Log.d("FingerprintSdk", "verify: Template loaded, size: ${template.size} bytes. Starting match...")
                android.util.Log.d("FingerprintSdk", "verify: Please place your finger on the scanner...")

                // Match using libfprint (matchWithTemplate will capture and match in one step)
                // matchWithTemplate internally calls fp_device_verify_sync which waits for finger scan
                // No need to capture separately - matchWithTemplate handles capture internally
                val score = matchingEngine.match(ByteArray(0), template) // template1 is ignored, native captures live
                android.util.Log.d("FingerprintSdk", "verify: Match completed, score: $score, threshold: ${config.matchThreshold}")

                // Check if score is -1.0f (error) vs 0.0f (no match)
                if (score < 0) {
                    android.util.Log.e("FingerprintSdk", "verify: Error during verification (score < 0)")
                    return@withContext VerifyResult.Error(FingerError.DEVICE_INIT_FAILED)
                }

                if (score >= config.matchThreshold) {
                    android.util.Log.d("FingerprintSdk", "verify: ✓ Verification successful! Score: $score")
                    // Log authentication
                    backendClient.logAuth(userId, true, score)
                    VerifyResult.Success(userId, score)
                } else {
                    android.util.Log.d("FingerprintSdk", "verify: ✗ Verification failed. Score: $score (threshold: ${config.matchThreshold})")
                    backendClient.logAuth(userId, false, score)
                    VerifyResult.Error(FingerError.VERIFICATION_FAILED)
                }
            } catch (e: Exception) {
                android.util.Log.e("FingerprintSdk", "verify: Exception occurred", e)
                VerifyResult.Error(FingerError.UNKNOWN_ERROR)
            }
        }
    }

    /**
     * Identify user from fingerprint (1:N matching).
     * 
     * @param scope Scope for template loading (e.g., branch ID)
     * @return IdentifyResult
     */
    suspend fun identify(scope: String? = null): IdentifyResult {
        ensureInitialized()
        return withContext(Dispatchers.IO) {
            try {
                android.util.Log.d("FingerprintSdk", "identify: Starting identification, scope: ${scope ?: "all"}")
                
                // Ensure device is open before identification
                if (!captureManager.isDeviceOpen()) {
                    android.util.Log.d("FingerprintSdk", "identify: Device not open, opening device...")
                    val openResult = captureManager.ensureDeviceOpen()
                    if (openResult is CaptureResult.Error) {
                        android.util.Log.e("FingerprintSdk", "identify: Failed to open device: ${openResult.error}")
                        return@withContext IdentifyResult.Error(openResult.error)
                    }
                }
                
                // Load templates (from backend)
                android.util.Log.d("FingerprintSdk", "identify: Loading templates from backend...")
                val templatesResult = backendClient.loadTemplates(scope)
                val templates = when (templatesResult) {
                    is BackendResult.Templates -> {
                        android.util.Log.d("FingerprintSdk", "identify: Loaded ${templatesResult.templates.size} templates from backend")
                        templatesResult.templates
                    }
                    else -> {
                        android.util.Log.e("FingerprintSdk", "identify: Failed to load templates from backend")
                        return@withContext IdentifyResult.Error(FingerError.BACKEND_ERROR)
                    }
                }

                if (templates.isEmpty()) {
                    android.util.Log.e("FingerprintSdk", "identify: No templates found")
                    return@withContext IdentifyResult.Error(FingerError.NO_TEMPLATES)
                }

                android.util.Log.d("FingerprintSdk", "identify: Starting identification with ${templates.size} templates (all fingers)...")
                android.util.Log.d("FingerprintSdk", "identify: Please place your finger on the scanner...")

                // Identify using libfprint (identifyUser will capture and match in one step)
                // No need to capture separately - identifyUser handles capture internally
                val (matchedIndex, score) = matchingEngine.identify(templates)
                android.util.Log.d("FingerprintSdk", "identify: Identification completed, matchedIndex: $matchedIndex, score: $score, threshold: ${config.matchThreshold}")

                if (matchedIndex >= 0 && matchedIndex < templates.size && score >= config.matchThreshold) {
                    // Get matched template info directly by index (includes userName and userEmail)
                    val matchedTemplate = templates[matchedIndex]
                    val matchedUserId = matchedTemplate.userId
                    val matchedFinger = matchedTemplate.finger
                    // userName is non-nullable String, should always have value from backend
                    val userName = matchedTemplate.userName
                    val userEmail = matchedTemplate.userEmail
                    
                    android.util.Log.d("FingerprintSdk", "identify: ✓ Identification successful! User: $userName ($matchedUserId), Finger: $matchedFinger, Score: $score")
                    backendClient.logAuth(matchedUserId, true, score)
                    IdentifyResult.Success(matchedUserId, userName, userEmail, matchedFinger, score)
                } else {
                    android.util.Log.d("FingerprintSdk", "identify: ✗ Identification failed. matchedIndex: $matchedIndex, score: $score (threshold: ${config.matchThreshold})")
                    backendClient.logAuth(-1, false, score) // Log even if no match
                    IdentifyResult.Error(FingerError.IDENTIFICATION_FAILED)
                }
            } catch (e: Exception) {
                android.util.Log.e("FingerprintSdk", "identify: Exception occurred", e)
                IdentifyResult.Error(FingerError.UNKNOWN_ERROR)
            }
        }
    }

    private fun ensureInitialized() {
        if (!isInitialized) {
            throw IllegalStateException("SDK not initialized. Call FingerprintSdk.init() first.")
        }
    }
}

