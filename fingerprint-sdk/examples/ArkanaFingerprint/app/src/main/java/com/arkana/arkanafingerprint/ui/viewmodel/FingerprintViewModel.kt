package com.arkana.arkanafingerprint.ui.viewmodel

import android.app.Application
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.viewModelScope
import com.arkana.arkanafingerprint.data.PreferencesManager
import com.arkana.fingerprint.sdk.FingerprintSdk
import com.arkana.fingerprint.sdk.config.FingerprintSdkConfig
import com.arkana.fingerprint.sdk.model.Finger
import com.arkana.fingerprint.sdk.model.EnrollResult
import com.arkana.fingerprint.sdk.model.VerifyResult
import com.arkana.fingerprint.sdk.model.IdentifyResult
import com.arkana.fingerprint.sdk.util.FingerError
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow
import kotlinx.coroutines.launch

data class UiState(
    val isLoading: Boolean = false,
    val message: String? = null,
    val isError: Boolean = false,
    val isInitialized: Boolean = false,
    val isDeviceInitialized: Boolean = false
)

class FingerprintViewModel(application: Application) : AndroidViewModel(application) {
    private val preferencesManager = PreferencesManager(application)
    
    private val _uiState = MutableStateFlow(UiState())
    val uiState: StateFlow<UiState> = _uiState.asStateFlow()

    private val _enrollResult = MutableStateFlow<EnrollResult?>(null)
    val enrollResult: StateFlow<EnrollResult?> = _enrollResult.asStateFlow()

    private val _verifyResult = MutableStateFlow<VerifyResult?>(null)
    val verifyResult: StateFlow<VerifyResult?> = _verifyResult.asStateFlow()

    private val _identifyResult = MutableStateFlow<IdentifyResult?>(null)
    val identifyResult: StateFlow<IdentifyResult?> = _identifyResult.asStateFlow()

    // Enrollment progress
    val enrollmentProgress = FingerprintSdk.enrollmentProgress

    init {
        initializeSdk()
    }

    fun initializeSdk() {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, message = "Initializing SDK...")
            try {
                // Get backend URL from preferences
                var backendUrl = preferencesManager.backendUrl
                
                // Auto-detect emulator and use 10.0.2.2 if emulator
                if (android.os.Build.FINGERPRINT.contains("generic") && 
                    !backendUrl.contains("10.0.2.2")) {
                    // If emulator and URL doesn't contain 10.0.2.2, suggest emulator URL
                    // But use what user configured
                }
                
                val config = FingerprintSdkConfig(
                    backendUrl = backendUrl,
                    enrollmentScans = 5,
                    matchThreshold = 0.4f,
                    timeoutMs = 30000L
                )
                FingerprintSdk.init(config, getApplication())
                
                // Check device status after initialization
                val deviceStatus = checkDeviceStatus()
                val statusMessage = if (deviceStatus) {
                    "SDK initialized successfully. Device connected."
                } else {
                    "SDK initialized successfully. Device not found. Please connect USB device."
                }
                
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isInitialized = true,
                    isDeviceInitialized = deviceStatus,
                    message = statusMessage
                )
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isError = true,
                    message = "Failed to initialize SDK: ${e.message}"
                )
            }
        }
    }
    
    private fun checkDeviceStatus(): Boolean {
        // Try to find device to check if it's connected
        android.util.Log.d("FingerprintViewModel", "checkDeviceStatus: Checking for device...")
        return try {
            val usbManager = getApplication<Application>().getSystemService(android.content.Context.USB_SERVICE) as android.hardware.usb.UsbManager
            val deviceList = usbManager.deviceList
            android.util.Log.d("FingerprintViewModel", "checkDeviceStatus: Found ${deviceList.size} USB devices")
            
            for (device in deviceList.values) {
                android.util.Log.d("FingerprintViewModel", "checkDeviceStatus: Checking device VID=${device.vendorId.toString(16)}, PID=${device.productId.toString(16)}")
                if (device.vendorId == 0x05BA && 
                    (device.productId == 0x000A || device.productId == 0x0034)) {
                    android.util.Log.d("FingerprintViewModel", "checkDeviceStatus: DigitalPersona device found!")
                    return true
                }
            }
            android.util.Log.w("FingerprintViewModel", "checkDeviceStatus: DigitalPersona device not found")
            false
        } catch (e: Exception) {
            android.util.Log.e("FingerprintViewModel", "checkDeviceStatus: Error", e)
            false
        }
    }
    
    fun getBackendUrl(): String = preferencesManager.backendUrl
    
    fun setBackendUrl(url: String) {
        preferencesManager.backendUrl = url
        // Just save, don't reinitialize yet
        _uiState.value = _uiState.value.copy(
            message = "Backend URL saved. Click 'Reinitialize SDK' to apply changes."
        )
    }
    
    fun reinitializeSdk() {
        // Reinitialize SDK with current backend URL
        initializeSdk()
    }

    fun enrollFingerprint(userId: Int, finger: Finger) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, message = "Enrolling fingerprint...")
            _enrollResult.value = null
            try {
                val result = FingerprintSdk.enroll(userId, finger)
                _enrollResult.value = result
                when (result) {
                    is EnrollResult.Success -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            message = "Fingerprint enrolled successfully for user $userId"
                        )
                    }
                    is EnrollResult.Error -> {
                        val errorMessage = when (result.error) {
                            FingerError.BACKEND_ERROR -> {
                                "Backend error: Please check if backend server is running at ${getBackendUrl()}"
                            }
                            FingerError.NETWORK_ERROR -> {
                                "Network error: Cannot connect to backend server"
                            }
                            else -> {
                                "Enrollment failed: ${result.error.name}"
                            }
                        }
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = errorMessage
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isError = true,
                    message = "Enrollment error: ${e.message}"
                )
            }
        }
    }

    fun verifyFingerprint(userId: Int, finger: Finger? = null) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, message = "Verifying fingerprint...")
            _verifyResult.value = null
            try {
                val result = FingerprintSdk.verify(userId, finger)
                _verifyResult.value = result
                when (result) {
                    is VerifyResult.Success -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            message = "Verification successful! Score: ${result.score}"
                        )
                    }
                    is VerifyResult.Error -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = "Verification failed: ${result.error.name}"
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isError = true,
                    message = "Verification error: ${e.message}"
                )
            }
        }
    }

    fun identifyFingerprint(scope: String? = null) {
        viewModelScope.launch {
            _uiState.value = _uiState.value.copy(isLoading = true, message = "Identifying fingerprint...")
            _identifyResult.value = null
            try {
                val result = FingerprintSdk.identify(scope)
                _identifyResult.value = result
                when (result) {
                    is IdentifyResult.Success -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            message = "Identification successful! User: ${result.userName} (${result.finger}), Score: ${result.score}"
                        )
                    }
                    is IdentifyResult.Error -> {
                        _uiState.value = _uiState.value.copy(
                            isLoading = false,
                            isError = true,
                            message = "Identification failed: ${result.error.name}"
                        )
                    }
                }
            } catch (e: Exception) {
                _uiState.value = _uiState.value.copy(
                    isLoading = false,
                    isError = true,
                    message = "Identification error: ${e.message}"
                )
            }
        }
    }

    fun clearMessage() {
        _uiState.value = _uiState.value.copy(message = null, isError = false)
    }
}

