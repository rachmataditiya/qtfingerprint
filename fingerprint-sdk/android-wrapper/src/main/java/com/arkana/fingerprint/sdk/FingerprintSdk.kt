package com.arkana.fingerprint.sdk

import android.content.Context
import com.arkana.fingerprint.sdk.config.FingerprintSdkConfig
import com.arkana.fingerprint.sdk.model.*
import com.arkana.fingerprint.sdk.capture.CaptureManager
import com.arkana.fingerprint.sdk.matching.MatchingEngine
import com.arkana.fingerprint.sdk.backend.BackendClient
import com.arkana.fingerprint.sdk.cache.SecureCache
import com.arkana.fingerprint.sdk.util.FingerError
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext

/**
 * Main SDK class for fingerprint operations.
 * 
 * Provides clean API for:
 * - Enrollment
 * - Verification (1:1)
 * - Identification (1:N)
 */
object FingerprintSdk {
    private var isInitialized = false
    private lateinit var config: FingerprintSdkConfig
    private lateinit var context: Context
    private lateinit var captureManager: CaptureManager
    private lateinit var matchingEngine: MatchingEngine
    private lateinit var backendClient: BackendClient
    private lateinit var secureCache: SecureCache

    /**
     * Initialize the SDK.
     * 
     * @param config SDK configuration
     * @param context Android Context
     * @throws IllegalStateException if already initialized
     */
    @JvmStatic
    fun init(config: FingerprintSdkConfig, context: Context) {
        if (isInitialized) {
            throw IllegalStateException("SDK already initialized")
        }

        this.config = config
        this.context = context.applicationContext

        // Initialize components
        captureManager = CaptureManager(context)
        matchingEngine = MatchingEngine()
        backendClient = BackendClient(config.backendUrl)
        secureCache = SecureCache(context, config.enableCache)

        // Load native library
        System.loadLibrary("arkana_fprint")

        isInitialized = true
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
                // Capture multiple scans for enrollment
                val scans = mutableListOf<ByteArray>()
                repeat(config.enrollmentScans) { scanIndex ->
                    val captureResult = captureManager.captureOnce(config.timeoutMs)
                    when (captureResult) {
                        is CaptureResult.Success -> {
                            scans.add(captureResult.template)
                        }
                        is CaptureResult.Error -> {
                            return@withContext EnrollResult.Error(captureResult.error)
                        }
                    }
                }

                // Create template from scans
                val template = matchingEngine.createTemplate(scans)

                // Store in backend
                val storeResult = backendClient.storeTemplate(userId, template, finger)
                if (storeResult is BackendResult.Error) {
                    return@withContext EnrollResult.Error(storeResult.error)
                }

                // Cache locally (encrypted)
                secureCache.store(userId, template)

                EnrollResult.Success(userId)
            } catch (e: Exception) {
                EnrollResult.Error(FingerError.UNKNOWN_ERROR)
            }
        }
    }

    /**
     * Verify fingerprint for a user (1:1 matching).
     * 
     * @param userId User ID to verify
     * @return VerifyResult
     */
    suspend fun verify(userId: Int): VerifyResult {
        ensureInitialized()
        return withContext(Dispatchers.IO) {
            try {
                // Load template (from cache or backend)
                val template = secureCache.load(userId)
                    ?: run {
                        // Load from backend if not in cache
                        val backendResult = backendClient.loadTemplate(userId)
                        when (backendResult) {
                            is BackendResult.Success -> {
                                backendResult.template.also {
                                    secureCache.store(userId, it)
                                }
                            }
                            is BackendResult.Error -> {
                                return@withContext VerifyResult.Error(backendResult.error)
                            }
                        }
                    }

                // Capture current fingerprint
                val captureResult = captureManager.captureOnce(config.timeoutMs)
                val currentTemplate = when (captureResult) {
                    is CaptureResult.Success -> captureResult.template
                    is CaptureResult.Error -> {
                        return@withContext VerifyResult.Error(captureResult.error)
                    }
                }

                // Match
                val score = matchingEngine.match(currentTemplate, template)

                if (score >= config.matchThreshold) {
                    // Log authentication
                    backendClient.logAuth(userId, true, score)
                    VerifyResult.Success(userId, score)
                } else {
                    backendClient.logAuth(userId, false, score)
                    VerifyResult.Error(FingerError.VERIFICATION_FAILED)
                }
            } catch (e: Exception) {
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
                // Load templates (from backend)
                val templatesResult = backendClient.loadTemplates(scope)
                val templates = when (templatesResult) {
                    is BackendResult.Success -> templatesResult.templates
                    is BackendResult.Error -> {
                        return@withContext IdentifyResult.Error(templatesResult.error)
                    }
                }

                if (templates.isEmpty()) {
                    return@withContext IdentifyResult.Error(FingerError.NO_TEMPLATES)
                }

                // Capture current fingerprint
                val captureResult = captureManager.captureOnce(config.timeoutMs)
                val currentTemplate = when (captureResult) {
                    is CaptureResult.Success -> captureResult.template
                    is CaptureResult.Error -> {
                        return@withContext IdentifyResult.Error(captureResult.error)
                    }
                }

                // Match against all templates
                var bestMatch: Pair<Int, Float>? = null
                for ((userId, template) in templates) {
                    val score = matchingEngine.match(currentTemplate, template)
                    if (score >= config.matchThreshold) {
                        if (bestMatch == null || score > bestMatch!!.second) {
                            bestMatch = userId to score
                        }
                    }
                }

                if (bestMatch != null) {
                    val (userId, score) = bestMatch
                    backendClient.logAuth(userId, true, score)
                    IdentifyResult.Success(userId, score)
                } else {
                    IdentifyResult.Error(FingerError.IDENTIFICATION_FAILED)
                }
            } catch (e: Exception) {
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

