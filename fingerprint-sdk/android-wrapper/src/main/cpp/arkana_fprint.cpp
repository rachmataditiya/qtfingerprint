#include <jni.h>
#include <android/log.h>
#include <string>
#include <vector>
#include "fingerprint_capture.h"

#define LOG_TAG "ArkanaFprint"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global instance
static FingerprintCapture* g_capture = nullptr;

extern "C" {

/**
 * Open USB device.
 */
JNIEXPORT jboolean JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_openDevice(
    JNIEnv* env, jclass clazz, jint fileDescriptor) {
    
    LOGI("openDevice: fileDescriptor=%d", fileDescriptor);
    
    if (fileDescriptor < 0) {
        LOGE("Invalid file descriptor: %d", fileDescriptor);
        return JNI_FALSE;
    }
    
    if (g_capture == nullptr) {
        LOGI("Creating new FingerprintCapture instance");
        g_capture = new FingerprintCapture();
    }

    // Set USB file descriptor BEFORE initializing context
    // This ensures LIBUSB_FD is set before libgusb initialization
    LOGI("Setting USB file descriptor: %d", fileDescriptor);
    if (!g_capture->setUsbFileDescriptor(fileDescriptor)) {
        LOGE("Failed to set USB file descriptor");
        if (g_capture) {
            delete g_capture;
            g_capture = nullptr;
        }
        return JNI_FALSE;
    }

    // Open device (deviceIndex=0 for first device)
    LOGI("Opening device (index 0)");
    if (!g_capture->openDevice(0)) {
        LOGE("Failed to open device");
        return JNI_FALSE;
    }

    LOGI("Device opened successfully");
    return JNI_TRUE;
}

/**
 * Close USB device.
 */
JNIEXPORT void JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_closeDevice(
    JNIEnv* env, jclass clazz) {
    
    if (g_capture != nullptr) {
        g_capture->closeDevice();
        // Don't delete here, keep for reuse
    }
}

/**
 * Capture fingerprint (single capture).
 */
JNIEXPORT jbyteArray JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_nativeCapture(
    JNIEnv* env, jclass clazz, jlong timeoutMs) {
    
    if (g_capture == nullptr) {
        LOGE("Device not opened");
        return nullptr;
    }

    // Capture template (single capture)
    std::vector<uint8_t> templateData;
    if (!g_capture->captureTemplate(templateData)) {
        LOGE("Failed to capture template");
        return nullptr;
    }

    // Convert to Java byte array
    jbyteArray result = env->NewByteArray(templateData.size());
    if (result == nullptr) {
        LOGE("Failed to create byte array");
        return nullptr;
    }

    env->SetByteArrayRegion(result, 0, templateData.size(), 
                           reinterpret_cast<const jbyte*>(templateData.data()));
    
    LOGI("Template captured: %zu bytes", templateData.size());
    return result;
}

/**
 * Enroll fingerprint (full enrollment with multiple scans).
 */
JNIEXPORT jbyteArray JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_nativeEnroll(
    JNIEnv* env, jclass clazz) {
    
    if (g_capture == nullptr) {
        LOGE("Device not opened");
        return nullptr;
    }

    // Enroll fingerprint (fp_device_enroll_sync performs all required scans)
    std::vector<uint8_t> templateData;
    if (!g_capture->enrollFingerprint(templateData)) {
        LOGE("Failed to enroll fingerprint");
        return nullptr;
    }

    // Convert to Java byte array
    jbyteArray result = env->NewByteArray(templateData.size());
    if (result == nullptr) {
        LOGE("Failed to create byte array");
        return nullptr;
    }

    env->SetByteArrayRegion(result, 0, templateData.size(), 
                           reinterpret_cast<const jbyte*>(templateData.data()));
    
    LOGI("Enrollment completed: %zu bytes", templateData.size());
    return result;
}

/**
 * Match two templates.
 */
JNIEXPORT jfloat JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_match(
    JNIEnv* env, jclass clazz, jbyteArray template1, jbyteArray template2) {
    
    if (g_capture == nullptr) {
        LOGE("Device not opened");
        return 0.0f;
    }

    // Get stored template (template2 is the stored template from database)
    jsize len2 = env->GetArrayLength(template2);
    jbyte* data2 = env->GetByteArrayElements(template2, nullptr);

    std::vector<uint8_t> storedTemplate(reinterpret_cast<uint8_t*>(data2), 
                                        reinterpret_cast<uint8_t*>(data2) + len2);

    // Match using libfprint
    // matchWithTemplate will capture current fingerprint and match with stored template
    // Signature: matchWithTemplate(storedTemplate, matched, score)
    bool matched = false;
    int score = 0;
    if (!g_capture->matchWithTemplate(storedTemplate, matched, score)) {
        LOGE("matchWithTemplate failed - check device status and template format");
        env->ReleaseByteArrayElements(template2, data2, JNI_ABORT);
        return -1.0f; // Return -1.0f to indicate error (not just no match)
    }

    env->ReleaseByteArrayElements(template2, data2, JNI_ABORT);

    // Convert score (0-100) to float (0.0-1.0)
    // Return score even if not matched, so SDK can distinguish between error and no match
    return score / 100.0f;
}

/**
 * Identify user from multiple templates (1:N matching).
 */
JNIEXPORT jint JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_nativeIdentify(
    JNIEnv* env, jclass clazz, jintArray userIds, jobjectArray fingers, jobjectArray templates, 
    jintArray scoreOut, jintArray matchedIndexOut) {
    
    if (g_capture == nullptr) {
        LOGE("Device not opened");
        return -1;
    }

    // Get arrays length
    jsize count = env->GetArrayLength(userIds);
    if (count != env->GetArrayLength(templates) || count != env->GetArrayLength(fingers)) {
        LOGE("User IDs, fingers, and templates arrays must have same length");
        return -1;
    }
    
    // Get user IDs array
    jint* userIdPtr = env->GetIntArrayElements(userIds, nullptr);
    
    // Build template list (one entry per template, not per user - supports multiple fingers per user)
    std::vector<std::pair<int, std::vector<uint8_t>>> templateList;
    for (jsize i = 0; i < count; i++) {
        jint userId = userIdPtr[i];
        
        // Get finger name (for logging)
        jstring fingerStr = (jstring)env->GetObjectArrayElement(fingers, i);
        const char* fingerCStr = env->GetStringUTFChars(fingerStr, nullptr);
        std::string finger(fingerCStr);
        env->ReleaseStringUTFChars(fingerStr, fingerCStr);
        env->DeleteLocalRef(fingerStr);
        
        // Get template data
        jbyteArray templateArray = (jbyteArray)env->GetObjectArrayElement(templates, i);
        jsize templateLen = env->GetArrayLength(templateArray);
        jbyte* templateBytes = env->GetByteArrayElements(templateArray, nullptr);
        std::vector<uint8_t> templateData(reinterpret_cast<uint8_t*>(templateBytes), 
                                          reinterpret_cast<uint8_t*>(templateBytes) + templateLen);
        env->ReleaseByteArrayElements(templateArray, templateBytes, JNI_ABORT);
        env->DeleteLocalRef(templateArray);
        
        LOGI("Template %zu: User %d, Finger %s, Size %zu bytes", i, userId, finger.c_str(), templateData.size());
        templateList.push_back(std::make_pair(userId, templateData));
    }
    
    env->ReleaseIntArrayElements(userIds, userIdPtr, JNI_ABORT);
    
    // Identify using libfprint (identifyUser will capture and match in one step)
    int matchedIndex = -1;
    int score = 0;
    if (!g_capture->identifyUser(templateList, matchedIndex, score)) {
        LOGE("Identification failed - check device status and template format");
        return -1; // Return -1 to indicate error (not just no match)
    }
    
    // Return score
    if (scoreOut && env->GetArrayLength(scoreOut) > 0) {
        jint* scorePtr = env->GetIntArrayElements(scoreOut, nullptr);
        scorePtr[0] = score;
        env->ReleaseIntArrayElements(scoreOut, scorePtr, 0);
    }
    
    // Return matched index
    if (matchedIndexOut && env->GetArrayLength(matchedIndexOut) > 0) {
        jint* indexPtr = env->GetIntArrayElements(matchedIndexOut, nullptr);
        indexPtr[0] = matchedIndex;
        env->ReleaseIntArrayElements(matchedIndexOut, indexPtr, 0);
    }
    
    // Return matchedIndex (not userId) so Kotlin can get full template info by index
    return matchedIndex;
}

} // extern "C"

