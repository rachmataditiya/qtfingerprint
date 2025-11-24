#include "fingerprint_manager_android.h"
#include "fingerprint_capture.h"
#include <android/log.h>
#include <thread>
#include <chrono>
#include <map>

#define LOG_TAG "FingerprintManagerAndroid"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

FingerprintManagerAndroid::FingerprintManagerAndroid()
    : m_jniEnv(nullptr)
    , m_context(nullptr)
    , m_activity(nullptr)
    , m_fingerprintJNI(nullptr)
    , m_jniClass(nullptr)
    , m_isOpen(false)
    , m_currentUserId(-1)
    , m_enrollmentInProgress(false)
    , m_verificationScore(0)
    , m_verificationComplete(false)
    , m_verificationResult(false)
    , m_identificationUserId(-1)
    , m_identificationScore(0)
    , m_identificationComplete(false)
    , m_capture(nullptr)
{
    m_capture = new FingerprintCapture();
}

FingerprintManagerAndroid::~FingerprintManagerAndroid()
{
    cleanup();
}

bool FingerprintManagerAndroid::initialize(JNIEnv* jniEnv, jobject context)
{
    m_jniEnv = jniEnv;
    m_context = jniEnv->NewGlobalRef(context);
    
    // Find FingerprintJNI class
    jclass jniClass = jniEnv->FindClass("com/arkana/libdigitalpersona/FingerprintJNI");
    if (!jniClass) {
        setError("Failed to find FingerprintJNI class");
        return false;
    }
    
    m_jniClass = (jclass)jniEnv->NewGlobalRef(jniClass);
    
    // Call initialize method
    jmethodID initMethod = jniEnv->GetStaticMethodID(jniClass, "initialize", "(Landroid/content/Context;)V");
    if (!initMethod) {
        setError("Failed to find initialize method");
        return false;
    }
    
    jniEnv->CallStaticVoidMethod(jniClass, initMethod, context);
    
    if (jniEnv->ExceptionCheck()) {
        jniEnv->ExceptionDescribe();
        jniEnv->ExceptionClear();
        setError("Exception during JNI initialization");
        return false;
    }
    
    LOGI("FingerprintManagerAndroid initialized");
    return true;
}

void FingerprintManagerAndroid::cleanup()
{
    if (m_capture) {
        m_capture->cleanup();
        delete m_capture;
        m_capture = nullptr;
    }
    
    if (m_jniEnv) {
        if (m_jniClass) {
            jmethodID cleanupMethod = m_jniEnv->GetStaticMethodID(m_jniClass, "cleanup", "()V");
            if (cleanupMethod) {
                m_jniEnv->CallStaticVoidMethod(m_jniClass, cleanupMethod);
            }
            
            m_jniEnv->DeleteGlobalRef(m_jniClass);
            m_jniClass = nullptr;
        }
        
        if (m_activity) {
            m_jniEnv->DeleteGlobalRef(m_activity);
            m_activity = nullptr;
        }
        
        if (m_context) {
            m_jniEnv->DeleteGlobalRef(m_context);
            m_context = nullptr;
        }
    }
    
    m_isOpen = false;
}

int FingerprintManagerAndroid::getDeviceCount()
{
    if (!m_capture) {
        return 0;
    }
    
    // Initialize if not already done
    if (!m_capture->initialize()) {
        LOGE("Failed to initialize FingerprintCapture: %s", m_capture->getLastError().c_str());
        return 0;
    }
    
    return m_capture->getDeviceCount();
}

bool FingerprintManagerAndroid::isAvailable()
{
    return getDeviceCount() > 0;
}

bool FingerprintManagerAndroid::openReader(jobject activity)
{
    if (!activity || !m_jniEnv) {
        setError("Activity is null or JNI not initialized");
        return false;
    }
    
    if (m_activity) {
        m_jniEnv->DeleteGlobalRef(m_activity);
    }
    
    m_activity = m_jniEnv->NewGlobalRef(activity);
    
    // Initialize capture and open device
    if (!m_capture) {
        m_capture = new FingerprintCapture();
    }
    
    if (!m_capture->initialize()) {
        setError("Failed to initialize libfprint: " + m_capture->getLastError());
        return false;
    }
    
    if (m_capture->getDeviceCount() == 0) {
        setError("No fingerprint devices found");
        return false;
    }
    
    if (!m_capture->openDevice(0)) {
        setError("Failed to open device: " + m_capture->getLastError());
        return false;
    }
    
    m_isOpen = true;
    LOGI("Fingerprint reader opened successfully");
    
    return true;
}

bool FingerprintManagerAndroid::startEnrollment(int userId)
{
    if (!m_isOpen || !m_activity || !m_jniClass || !m_jniEnv) {
        setError("Reader not open or activity not set");
        return false;
    }
    
    m_currentUserId = userId;
    m_enrollmentInProgress = true;
    m_lastError.clear();
    
    // Call Java method to start enrollment
    jmethodID method = m_jniEnv->GetStaticMethodID(
        m_jniClass, 
        "startEnrollment", 
        "(ILandroidx/fragment/app/FragmentActivity;)V"
    );
    
    if (!method) {
        // Try direct service call
        jclass serviceClass = m_jniEnv->FindClass("com/arkana/libdigitalpersona/FingerprintService");
        if (serviceClass) {
            // Use service directly via JNI
            // For now, return true and let callbacks handle it
            return true;
        }
        setError("Failed to find enrollment method");
        return false;
    }
    
    m_jniEnv->CallStaticVoidMethod(m_jniClass, method, userId, m_activity);
    
    if (m_jniEnv->ExceptionCheck()) {
        m_jniEnv->ExceptionDescribe();
        m_jniEnv->ExceptionClear();
        setError("Exception during enrollment start");
        return false;
    }
    
    return true;
}

bool FingerprintManagerAndroid::verifyFingerprint(const std::vector<uint8_t>& storedTemplate, bool& matched, int& score)
{
    matched = false;
    score = 0;
    
    if (!m_capture || !m_isOpen) {
        setError("Reader not open");
        return false;
    }
    
    // Ensure device is open
    if (!m_capture->openDevice(0)) {
        setError("Failed to open device: " + m_capture->getLastError());
        return false;
    }
    
    // Match using libfprint (like Qt does)
    bool result = m_capture->matchWithTemplate(storedTemplate, matched, score);
    
    // Close device after matching
    m_capture->closeDevice();
    
    if (!result) {
        setError("Matching failed: " + m_capture->getLastError());
        return false;
    }
    
    return true;
}

int FingerprintManagerAndroid::identifyUser(const std::map<int, std::vector<uint8_t>>& templates, int& score)
{
    score = 0;
    
    if (!m_capture || !m_isOpen) {
        setError("Reader not open");
        return -1;
    }
    
    if (templates.empty()) {
        setError("No templates provided");
        return -1;
    }
    
    // Ensure device is open
    if (!m_capture->openDevice(0)) {
        setError("Failed to open device: " + m_capture->getLastError());
        return -1;
    }
    
    // Identify using libfprint (like Qt does)
    int matchedUserId = -1;
    bool result = m_capture->identifyUser(templates, matchedUserId, score);
    
    // Close device after identification
    m_capture->closeDevice();
    
    if (!result) {
        setError("Identification failed: " + m_capture->getLastError());
        return -1;
    }
    
    return matchedUserId;
}

void FingerprintManagerAndroid::cancel()
{
    if (m_jniClass && m_jniEnv) {
        jmethodID method = m_jniEnv->GetStaticMethodID(m_jniClass, "cancel", "()V");
        if (method) {
            m_jniEnv->CallStaticVoidMethod(m_jniClass, method);
        }
    }
    
    m_enrollmentInProgress = false;
    m_verificationComplete = true;
    m_identificationComplete = true;
}

void FingerprintManagerAndroid::setProgressCallback(std::function<void(int, int, const std::string&)> callback)
{
    m_progressCallback = callback;
}

// ========== Callbacks from JNI ==========

void FingerprintManagerAndroid::onEnrollmentProgress(int current, int total, const std::string& message)
{
    if (m_progressCallback) {
        m_progressCallback(current, total, message);
    }
    LOGI("Enrollment progress: %d/%d - %s", current, total, message.c_str());
}

void FingerprintManagerAndroid::onEnrollmentComplete(const std::vector<uint8_t>& templateData)
{
    m_enrollmentInProgress = false;
    LOGI("Enrollment complete, template size: %zu", templateData.size());
}

void FingerprintManagerAndroid::onEnrollmentError(const std::string& error)
{
    m_enrollmentInProgress = false;
    setError("Enrollment error: " + error);
    LOGE("Enrollment error: %s", error.c_str());
}

void FingerprintManagerAndroid::onVerificationSuccess(int score)
{
    m_verificationComplete = true;
    m_verificationResult = true;
    m_verificationScore = score;
    LOGI("Verification success, score: %d", score);
}

void FingerprintManagerAndroid::onVerificationFailure(const std::string& error)
{
    m_verificationComplete = true;
    m_verificationResult = false;
    m_verificationScore = 0;
    setError("Verification failed: " + error);
    LOGE("Verification failure: %s", error.c_str());
}

void FingerprintManagerAndroid::onIdentificationMatch(int userId, int score)
{
    m_identificationComplete = true;
    m_identificationUserId = userId;
    m_identificationScore = score;
    LOGI("Identification match: userId=%d, score=%d", userId, score);
}

void FingerprintManagerAndroid::onIdentificationNoMatch(const std::string& error)
{
    m_identificationComplete = true;
    m_identificationUserId = -1;
    m_identificationScore = 0;
    LOGI("Identification no match: %s", error.c_str());
}

void FingerprintManagerAndroid::waitForCompletion()
{
    // Simple busy-wait for completion
    // In production, use proper synchronization primitives
    int timeout = 30000; // 30 seconds
    int waited = 0;
    while (!m_verificationComplete && !m_identificationComplete && waited < timeout) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        waited += 100;
    }
}

void FingerprintManagerAndroid::setError(const std::string& error)
{
    m_lastError = error;
    LOGE("Error: %s", error.c_str());
}

// ========== JNI Export Functions ==========

extern "C" {

// Global pointer to FingerprintManagerAndroid instance
static FingerprintManagerAndroid* g_fingerprintManager = nullptr;

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onEnrollmentProgress(JNIEnv* env, jclass clazz, jint current, jint total, jstring message) {
    if (g_fingerprintManager) {
        const char* msg = env->GetStringUTFChars(message, nullptr);
        g_fingerprintManager->onEnrollmentProgress(current, total, std::string(msg));
        env->ReleaseStringUTFChars(message, msg);
    }
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onEnrollmentComplete(JNIEnv* env, jclass clazz, jbyteArray templateData) {
    if (g_fingerprintManager) {
        jsize len = env->GetArrayLength(templateData);
        jbyte* bytes = env->GetByteArrayElements(templateData, nullptr);
        std::vector<uint8_t> data(bytes, bytes + len);
        env->ReleaseByteArrayElements(templateData, bytes, JNI_ABORT);
        g_fingerprintManager->onEnrollmentComplete(data);
    }
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onEnrollmentError(JNIEnv* env, jclass clazz, jstring error) {
    if (g_fingerprintManager) {
        const char* err = env->GetStringUTFChars(error, nullptr);
        g_fingerprintManager->onEnrollmentError(std::string(err));
        env->ReleaseStringUTFChars(error, err);
    }
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onVerificationSuccess(JNIEnv* env, jclass clazz, jint score) {
    if (g_fingerprintManager) {
        g_fingerprintManager->onVerificationSuccess(score);
    }
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onVerificationFailure(JNIEnv* env, jclass clazz, jstring error) {
    if (g_fingerprintManager) {
        const char* err = env->GetStringUTFChars(error, nullptr);
        g_fingerprintManager->onVerificationFailure(std::string(err));
        env->ReleaseStringUTFChars(error, err);
    }
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onIdentificationMatch(JNIEnv* env, jclass clazz, jint userId, jint score) {
    if (g_fingerprintManager) {
        g_fingerprintManager->onIdentificationMatch(userId, score);
    }
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_onIdentificationNoMatch(JNIEnv* env, jclass clazz, jstring error) {
    if (g_fingerprintManager) {
        const char* err = env->GetStringUTFChars(error, nullptr);
        g_fingerprintManager->onIdentificationNoMatch(std::string(err));
        env->ReleaseStringUTFChars(error, err);
    }
}

// Function to set global FingerprintManagerAndroid instance
// This is called from native-lib.cpp after creating instance
extern "C" void setGlobalFingerprintManager(FingerprintManagerAndroid* manager) {
    g_fingerprintManager = manager;
}

} // extern "C"

