#include <jni.h>
#include <string>
#include <map>
#include "fingerprint_manager_android.h"
#include "fingerprint_capture.h"
#include <android/log.h>

#define LOG_TAG "NativeLib"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

// Global instances
static FingerprintManagerAndroid* g_manager = nullptr;
static FingerprintCapture* g_capture = nullptr;

extern "C" {

JNIEXPORT jlong JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_createNativeInstance(JNIEnv* env, jclass clazz, jobject context) {
    LOGI("Creating FingerprintManagerAndroid instance");
    
    g_manager = new FingerprintManagerAndroid();
    if (!g_manager->initialize(env, context)) {
        LOGE("Failed to initialize FingerprintManagerAndroid");
        delete g_manager;
        g_manager = nullptr;
        return 0;
    }
    
    // Set as global instance for callbacks
    extern void setGlobalFingerprintManager(FingerprintManagerAndroid* manager);
    setGlobalFingerprintManager(g_manager);
    
    LOGI("FingerprintManagerAndroid instance created successfully");
    return reinterpret_cast<jlong>(g_manager);
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_destroyNativeInstance(JNIEnv* env, jclass clazz, jlong nativePtr) {
    if (nativePtr) {
        FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
        manager->cleanup();
        delete manager;
        if (g_manager == manager) {
            g_manager = nullptr;
        }
    }
}

JNIEXPORT jboolean JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeIsAvailable(JNIEnv* env, jclass clazz, jlong nativePtr) {
    if (!nativePtr) return JNI_FALSE;
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    return manager->isAvailable() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeGetDeviceCount(JNIEnv* env, jclass clazz, jlong nativePtr) {
    if (!nativePtr) return 0;
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    return manager->getDeviceCount();
}

JNIEXPORT jboolean JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeOpenReader(JNIEnv* env, jclass clazz, jlong nativePtr, jobject activity) {
    if (!nativePtr || !activity) return JNI_FALSE;
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    return manager->openReader(activity) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeStartEnrollment(JNIEnv* env, jclass clazz, jlong nativePtr, jint userId) {
    if (!nativePtr) return JNI_FALSE;
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    return manager->startEnrollment(userId) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeVerifyFingerprint(JNIEnv* env, jclass clazz, jlong nativePtr, jint userId, jintArray scoreOut) {
    if (!nativePtr) return JNI_FALSE;
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    int score = 0;
    bool result = manager->verifyFingerprint(userId, score);
    
    if (scoreOut && env->GetArrayLength(scoreOut) > 0) {
        jint* scorePtr = env->GetIntArrayElements(scoreOut, nullptr);
        scorePtr[0] = score;
        env->ReleaseIntArrayElements(scoreOut, scorePtr, 0);
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jint JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeIdentifyUser(JNIEnv* env, jclass clazz, jlong nativePtr, jintArray userIds, jintArray scoreOut) {
    if (!nativePtr || !userIds) return -1;
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    
    jsize len = env->GetArrayLength(userIds);
    jint* ids = env->GetIntArrayElements(userIds, nullptr);
    std::vector<int> idVec(ids, ids + len);
    env->ReleaseIntArrayElements(userIds, ids, JNI_ABORT);
    
    int score = 0;
    int matchedUserId = manager->identifyUser(idVec, score);
    
    if (scoreOut && env->GetArrayLength(scoreOut) > 0) {
        jint* scorePtr = env->GetIntArrayElements(scoreOut, nullptr);
        scorePtr[0] = score;
        env->ReleaseIntArrayElements(scoreOut, scorePtr, 0);
    }
    
    return matchedUserId;
}

JNIEXPORT void JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeCancel(JNIEnv* env, jclass clazz, jlong nativePtr) {
    if (nativePtr) {
        FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
        manager->cancel();
    }
}

JNIEXPORT jstring JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeGetLastError(JNIEnv* env, jclass clazz, jlong nativePtr) {
    if (!nativePtr) return env->NewStringUTF("");
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    std::string error = manager->getLastError();
    return env->NewStringUTF(error.c_str());
}

JNIEXPORT jbyteArray JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeCaptureFingerprint(JNIEnv* env, jclass clazz, jlong nativePtr) {
    if (!nativePtr) {
        LOGE("Native instance is null");
        return nullptr;
    }
    
    // Initialize capture if not already done
    if (!g_capture) {
        g_capture = new FingerprintCapture();
        if (!g_capture->initialize()) {
            LOGE("Failed to initialize FingerprintCapture: %s", g_capture->getLastError().c_str());
            delete g_capture;
            g_capture = nullptr;
            return nullptr;
        }
    }
    
    // Check if device is available
    int deviceCount = g_capture->getDeviceCount();
    if (deviceCount == 0) {
        LOGE("No fingerprint devices available");
        return nullptr;
    }
    
    // Open first device
    if (!g_capture->openDevice(0)) {
        LOGE("Failed to open device: %s", g_capture->getLastError().c_str());
        return nullptr;
    }
    
    // Capture template
    std::vector<uint8_t> templateData;
    if (!g_capture->captureTemplate(templateData)) {
        LOGE("Failed to capture template: %s", g_capture->getLastError().c_str());
        g_capture->closeDevice();
        return nullptr;
    }
    
    // Close device
    g_capture->closeDevice();
    
    // Convert to Java byte array
    if (templateData.empty()) {
        LOGE("Captured template is empty");
        return nullptr;
    }
    
    jbyteArray result = env->NewByteArray(templateData.size());
    if (!result) {
        LOGE("Failed to create byte array");
        return nullptr;
    }
    
    env->SetByteArrayRegion(result, 0, templateData.size(), reinterpret_cast<const jbyte*>(templateData.data()));
    LOGI("Fingerprint captured successfully: %zu bytes", templateData.size());
    
    return result;
}

JNIEXPORT jboolean JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeMatchWithTemplate(JNIEnv* env, jclass clazz, jlong nativePtr, jbyteArray templateData, jintArray resultOut) {
    if (!nativePtr) {
        LOGE("Native instance is null");
        return JNI_FALSE;
    }
    
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    
    // Get template data from Java
    jsize templateLen = env->GetArrayLength(templateData);
    jbyte* templateBytes = env->GetByteArrayElements(templateData, nullptr);
    std::vector<uint8_t> storedTemplate(templateBytes, templateBytes + templateLen);
    env->ReleaseByteArrayElements(templateData, templateBytes, JNI_ABORT);
    
    // Match using libfprint
    bool matched = false;
    int score = 0;
    bool result = manager->verifyFingerprint(storedTemplate, matched, score);
    
    if (!result) {
        LOGE("Matching failed: %s", manager->getLastError().c_str());
        return JNI_FALSE;
    }
    
    // Return result
    if (resultOut && env->GetArrayLength(resultOut) >= 2) {
        jint* resultPtr = env->GetIntArrayElements(resultOut, nullptr);
        resultPtr[0] = matched ? 1 : 0; // matched
        resultPtr[1] = score; // score
        env->ReleaseIntArrayElements(resultOut, resultPtr, 0);
    }
    
    return JNI_TRUE;
}

JNIEXPORT jint JNICALL
Java_com_arkana_libdigitalpersona_FingerprintJNI_nativeIdentifyWithTemplates(JNIEnv* env, jclass clazz, jlong nativePtr, jobjectArray userIds, jobjectArray templates, jintArray scoreOut) {
    if (!nativePtr) {
        LOGE("Native instance is null");
        return -1;
    }
    
    FingerprintManagerAndroid* manager = reinterpret_cast<FingerprintManagerAndroid*>(nativePtr);
    
    // Get arrays length
    jsize count = env->GetArrayLength(userIds);
    if (count != env->GetArrayLength(templates)) {
        LOGE("User IDs and templates arrays must have same length");
        return -1;
    }
    
    // Build template map
    std::map<int, std::vector<uint8_t>> templateMap;
    for (jsize i = 0; i < count; i++) {
        // Get user ID (userIds is array of IntArray, each containing one userId)
        jobject userIdObj = env->GetObjectArrayElement(userIds, i);
        jintArray userIdArray = (jintArray)userIdObj;
        jint* userIdPtr = env->GetIntArrayElements(userIdArray, nullptr);
        jint userId = userIdPtr[0];
        env->ReleaseIntArrayElements(userIdArray, userIdPtr, JNI_ABORT);
        env->DeleteLocalRef(userIdObj);
        
        // Get template data
        jbyteArray templateArray = (jbyteArray)env->GetObjectArrayElement(templates, i);
        jsize templateLen = env->GetArrayLength(templateArray);
        jbyte* templateBytes = env->GetByteArrayElements(templateArray, nullptr);
        std::vector<uint8_t> templateData(templateBytes, templateBytes + templateLen);
        env->ReleaseByteArrayElements(templateArray, templateBytes, JNI_ABORT);
        env->DeleteLocalRef(templateArray);
        
        templateMap[userId] = templateData;
    }
    
    // Identify using libfprint
    int score = 0;
    int matchedUserId = manager->identifyUser(templateMap, score);
    
    // Return score
    if (scoreOut && env->GetArrayLength(scoreOut) > 0) {
        jint* scorePtr = env->GetIntArrayElements(scoreOut, nullptr);
        scorePtr[0] = score;
        env->ReleaseIntArrayElements(scoreOut, scorePtr, 0);
    }
    
    return matchedUserId;
}

} // extern "C"
