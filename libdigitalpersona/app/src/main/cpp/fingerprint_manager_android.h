#ifndef FINGERPRINT_MANAGER_ANDROID_H
#define FINGERPRINT_MANAGER_ANDROID_H

#include <jni.h>
#include <string>
#include <vector>
#include <map>
#include <functional>

// Forward declaration
class FingerprintCapture;

/**
 * @brief Device information structure (Android version)
 */
struct DeviceInfo {
    std::string name;
    std::string driver;
    std::string deviceId;
    bool isOpen;
    bool supportsCapture;
    bool supportsIdentify;
};

/**
 * @brief Fingerprint template with metadata (Android version)
 */
struct FingerprintTemplate {
    std::vector<uint8_t> data;  // Key alias as template identifier
    int qualityScore;
    int scanCount;
};

/**
 * @brief Main fingerprint manager class for Android
 * 
 * Provides same API as desktop FingerprintManager but uses Android BiometricPrompt
 * via JNI bridge to FingerprintService.kt
 */
class FingerprintManagerAndroid {
public:
    FingerprintManagerAndroid();
    ~FingerprintManagerAndroid();
    
    // ========== Initialization & Cleanup ==========
    
    /**
     * @brief Initialize the fingerprint library
     * @param jniEnv JNI environment
     * @param context Android Context
     * @return true on success, false on failure
     */
    bool initialize(JNIEnv* jniEnv, jobject context);
    
    /**
     * @brief Cleanup all resources
     */
    void cleanup();
    
    // ========== Device Management ==========
    
    /**
     * @brief Get number of available fingerprint readers (always 1 for Android)
     * @return 1 if available, 0 otherwise
     */
    int getDeviceCount();
    
    /**
     * @brief Check if device is available
     * @return true if fingerprint hardware is available
     */
    bool isAvailable();
    
    /**
     * @brief Open the fingerprint reader
     * @param activity Android Activity (FragmentActivity)
     * @return true on success
     */
    bool openReader(jobject activity);
    
    /**
     * @brief Check if reader is open
     * @return true if open
     */
    bool isReaderOpen() const { return m_isOpen; }
    
    // ========== Enrollment Operations ==========
    
    /**
     * @brief Start enrollment session
     * @param userId User ID for this enrollment
     * @return true on success
     */
    bool startEnrollment(int userId);
    
    /**
     * @brief Set progress callback for enrollment
     */
    void setProgressCallback(std::function<void(int, int, const std::string&)> callback);
    
    // ========== Verification Operations ==========
    
    /**
     * @brief Verify fingerprint against stored template (1:1) using libfprint
     * @param storedTemplate Template data from database (FP3 format)
     * @param[out] score Match score (0-100)
     * @return true if match found
     */
    bool verifyFingerprint(const std::vector<uint8_t>& storedTemplate, bool& matched, int& score);
    
    // ========== Identification Operations ==========
    
    /**
     * @brief Identify user from templates (1:N) using libfprint
     * @param templates Map of userId -> template data
     * @param[out] score Match score of best match
     * @return User ID of match, or -1 if no match
     */
    int identifyUser(const std::map<int, std::vector<uint8_t>>& templates, int& score);
    
    /**
     * @brief Cancel current operation
     */
    void cancel();
    
    // ========== Error Handling ==========
    
    /**
     * @brief Get last error message
     */
    std::string getLastError() const { return m_lastError; }
    
    /**
     * @brief Get FingerprintCapture instance (for JNI access)
     */
    FingerprintCapture* getFingerprintCaptureInstance() const { return m_capture; }
    
    // ========== Callbacks from JNI ==========
    void onEnrollmentProgress(int current, int total, const std::string& message);
    void onEnrollmentComplete(const std::vector<uint8_t>& templateData);
    void onEnrollmentError(const std::string& error);
    void onVerificationSuccess(int score);
    void onVerificationFailure(const std::string& error);
    void onIdentificationMatch(int userId, int score);
    void onIdentificationNoMatch(const std::string& error);
    
private:
    // libfprint capture instance
    FingerprintCapture* m_capture;
    JNIEnv* m_jniEnv;
    jobject m_context;
    jobject m_activity;
    jobject m_fingerprintJNI;
    jclass m_jniClass;
    bool m_isOpen;
    std::string m_lastError;
    
    // Enrollment state
    int m_currentUserId;
    bool m_enrollmentInProgress;
    std::function<void(int, int, const std::string&)> m_progressCallback;
    
    // Verification/Identification state
    int m_verificationScore;
    bool m_verificationComplete;
    bool m_verificationResult;
    
    int m_identificationUserId;
    int m_identificationScore;
    bool m_identificationComplete;
    
    void setError(const std::string& error);
    void waitForCompletion();
};

#endif // FINGERPRINT_MANAGER_ANDROID_H

