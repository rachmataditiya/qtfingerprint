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
    JNIEnv* env, jclass clazz, jobject usbDevice) {
    
    if (g_capture == nullptr) {
        g_capture = new FingerprintCapture();
        if (!g_capture->initialize()) {
            LOGE("Failed to initialize FingerprintCapture");
            delete g_capture;
            g_capture = nullptr;
            return JNI_FALSE;
        }
    }

    // Get USB file descriptor from Android USB device
    // TODO: Extract file descriptor from UsbDevice
    // For now, this is a placeholder
    
    // Open device
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
 * Capture fingerprint.
 */
JNIEXPORT jbyteArray JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_nativeCapture(
    JNIEnv* env, jclass clazz, jlong timeoutMs) {
    
    if (g_capture == nullptr) {
        LOGE("Device not opened");
        return nullptr;
    }

    // Capture template
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
 * Match two templates.
 */
JNIEXPORT jfloat JNICALL
Java_com_arkana_fingerprint_sdk_capture_native_LibfprintNative_match(
    JNIEnv* env, jclass clazz, jbyteArray template1, jbyteArray template2) {
    
    if (g_capture == nullptr) {
        LOGE("Device not opened");
        return 0.0f;
    }

    // Get template data
    jsize len1 = env->GetArrayLength(template1);
    jsize len2 = env->GetArrayLength(template2);
    
    jbyte* data1 = env->GetByteArrayElements(template1, nullptr);
    jbyte* data2 = env->GetByteArrayElements(template2, nullptr);

    std::vector<uint8_t> t1(reinterpret_cast<uint8_t*>(data1), 
                           reinterpret_cast<uint8_t*>(data1) + len1);
    std::vector<uint8_t> t2(reinterpret_cast<uint8_t*>(data2), 
                           reinterpret_cast<uint8_t*>(data2) + len2);

    // Match using libfprint
    bool matched = false;
    int score = 0;
    if (!g_capture->matchWithTemplate(t1, t2, matched, score)) {
        env->ReleaseByteArrayElements(template1, data1, JNI_ABORT);
        env->ReleaseByteArrayElements(template2, data2, JNI_ABORT);
        return 0.0f;
    }

    env->ReleaseByteArrayElements(template1, data1, JNI_ABORT);
    env->ReleaseByteArrayElements(template2, data2, JNI_ABORT);

    // Convert score (0-100) to float (0.0-1.0)
    return matched ? (score / 100.0f) : 0.0f;
}

} // extern "C"

