#include "fingerprint_capture.h"
#include <android/log.h>
#include <cstring>

// Include libfprint headers
extern "C" {
#include <glib.h>
#include <glib-object.h>
#include <libfprint-2/fprint.h>
}

#define LOG_TAG "FingerprintCapture"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

FingerprintCapture::FingerprintCapture()
    : m_context(nullptr)
    , m_device(nullptr)
{
}

FingerprintCapture::~FingerprintCapture()
{
    cleanup();
}

bool FingerprintCapture::initialize()
{
    if (m_context != nullptr) {
        LOGI("Already initialized");
        return true;
    }
    
    // Initialize GLib type system (required for GObject)
    // Note: g_type_init() is deprecated in GLib 2.36+, but we call it for compatibility
    #if GLIB_CHECK_VERSION(2, 36, 0)
    // Type system is automatically initialized in GLib 2.36+
    #else
    g_type_init();
    #endif
    
    // Create libfprint context
    m_context = fp_context_new();
    if (!m_context) {
        setError("Failed to create libfprint context");
        return false;
    }
    
    LOGI("libfprint context initialized successfully");
    return true;
}

void FingerprintCapture::cleanup()
{
    closeDevice();
    
    if (m_context) {
        g_object_unref(m_context);
        m_context = nullptr;
    }
}

int FingerprintCapture::getDeviceCount()
{
    if (!m_context) {
        if (!initialize()) {
            return 0;
        }
    }
    
    GPtrArray* devices = fp_context_get_devices(m_context);
    if (!devices) {
        return 0;
    }
    
    int count = devices->len;
    g_ptr_array_unref(devices);
    
    return count;
}

bool FingerprintCapture::openDevice(int deviceIndex)
{
    if (!m_context) {
        if (!initialize()) {
            return false;
        }
    }
    
    // Close existing device if any
    closeDevice();
    
    // Get available devices
    GPtrArray* devices = fp_context_get_devices(m_context);
    if (!devices || devices->len == 0) {
        setError("No fingerprint devices found");
        if (devices) {
            g_ptr_array_unref(devices);
        }
        return false;
    }
    
    if (deviceIndex < 0 || deviceIndex >= (int)devices->len) {
        setError("Invalid device index");
        g_ptr_array_unref(devices);
        return false;
    }
    
    FpDevice* device = FP_DEVICE(g_ptr_array_index(devices, deviceIndex));
    if (!device) {
        setError("Failed to get device");
        g_ptr_array_unref(devices);
        return false;
    }
    
    // Take reference to device
    m_device = FP_DEVICE(g_object_ref(device));
    g_ptr_array_unref(devices);
    
    // Open device synchronously
    GError* error = nullptr;
    gboolean result = fp_device_open_sync(m_device, nullptr, &error);
    
    if (!result) {
        std::string errorMsg = "Failed to open device";
        if (error) {
            errorMsg += ": ";
            errorMsg += error->message;
            g_error_free(error);
        }
        setError(errorMsg);
        g_object_unref(m_device);
        m_device = nullptr;
        return false;
    }
    
    const char* deviceName = fp_device_get_name(m_device);
    LOGI("Device opened successfully: %s", deviceName ? deviceName : "unknown");
    
    return true;
}

void FingerprintCapture::closeDevice()
{
    if (m_device) {
        if (fp_device_is_open(m_device)) {
            GError* error = nullptr;
            fp_device_close_sync(m_device, nullptr, &error);
            if (error) {
                LOGE("Error closing device: %s", error->message);
                g_error_free(error);
            }
        }
        g_object_unref(m_device);
        m_device = nullptr;
    }
}

bool FingerprintCapture::captureTemplate(std::vector<uint8_t>& templateData)
{
    templateData.clear();
    
    if (!m_device) {
        setError("Device not open. Call openDevice() first.");
        return false;
    }
    
    if (!fp_device_is_open(m_device)) {
        setError("Device is not open");
        return false;
    }
    
    // Capture image synchronously
    GError* error = nullptr;
    FpImage* image = fp_device_capture_sync(m_device, TRUE, nullptr, &error);
    
    if (!image) {
        std::string errorMsg = "Failed to capture fingerprint image";
        if (error) {
            errorMsg += ": ";
            errorMsg += error->message;
            g_error_free(error);
        }
        setError(errorMsg);
        return false;
    }
    
    LOGI("Fingerprint image captured successfully");
    
    // Create print from image
    FpPrint* print = fp_print_new(m_device);
    if (!print) {
        setError("Failed to create print object");
        g_object_unref(image);
        return false;
    }
    
    // Set print properties (optional, but recommended)
    fp_print_set_finger(print, FP_FINGER_ANY);
    
    // Create print data from image
    // Note: fp_print_data_from_image is an internal API, we need to use enrollment or verify
    // For now, we'll serialize the image data directly
    // In a real implementation, we'd use fp_device_enroll_sync or create print data properly
    
    // Get image data
    gsize dataLen = 0;
    const guchar* imageData = fp_image_get_data(image, &dataLen);
    
    if (!imageData || dataLen == 0) {
        setError("Image data is empty");
        g_object_unref(print);
        g_object_unref(image);
        return false;
    }
    
    // For now, serialize the print object (this creates a template)
    // Note: This requires the print to have been enrolled or created properly
    // We'll use a simpler approach: store image data as template
    // In production, use fp_print_data_serialize or proper enrollment flow
    
    // Copy image data to template
    templateData.resize(dataLen);
    memcpy(templateData.data(), imageData, dataLen);
    
    // Cleanup
    g_object_unref(print);
    g_object_unref(image);
    
    LOGI("Template created successfully, size: %zu bytes", templateData.size());
    
    return true;
}

bool FingerprintCapture::matchWithTemplate(const std::vector<uint8_t>& storedTemplate, bool& matched, int& score)
{
    matched = false;
    score = 0;
    
    if (!m_device) {
        setError("Device not open. Call openDevice() first.");
        return false;
    }
    
    if (!fp_device_is_open(m_device)) {
        setError("Device is not open");
        return false;
    }
    
    // Deserialize stored template (FP3 format)
    GError* error = nullptr;
    FpPrint* storedPrint = fp_print_deserialize(storedTemplate.data(), storedTemplate.size(), &error);
    if (error) {
        std::string errorMsg = "Failed to deserialize template: ";
        errorMsg += error->message;
        setError(errorMsg);
        g_error_free(error);
        return false;
    }
    
    LOGI("Template deserialized successfully, starting verification...");
    
    // Verify - capture and match in one step (like Qt does)
    FpPrint* newPrint = nullptr;
    gboolean match = FALSE;
    
    gboolean result = fp_device_verify_sync(
        m_device,
        storedPrint,
        nullptr, // cancellable
        nullptr, // verify_cb
        nullptr, // verify_data
        &match,
        &newPrint,
        &error
    );
    
    if (error) {
        if (error->domain == FP_DEVICE_ERROR && error->code == FP_DEVICE_ERROR_DATA_NOT_FOUND) {
            // No match - this is normal for failed verification
            LOGI("Fingerprint scanned but NO MATCH");
            matched = false;
            score = 0;
            g_error_free(error);
            g_object_unref(storedPrint);
            if (newPrint) {
                g_object_unref(newPrint);
            }
            return true; // Successfully completed, just no match
        }
        
        std::string errorMsg = "Verification failed: ";
        errorMsg += error->message;
        setError(errorMsg);
        g_error_free(error);
        g_object_unref(storedPrint);
        if (newPrint) {
            g_object_unref(newPrint);
        }
        return false;
    }
    
    if (!result) {
        setError("Verification failed - no result returned");
        g_object_unref(storedPrint);
        if (newPrint) {
            g_object_unref(newPrint);
        }
        return false;
    }
    
    matched = (match == TRUE);
    
    // Calculate score based on match
    if (matched) {
        score = 95; // libfprint doesn't provide detailed score, just match/no-match
        LOGI("✓ FINGERPRINT MATCHED!");
    } else {
        score = 30; // Low score for no match
        LOGI("✗ Fingerprint does not match");
    }
    
    g_object_unref(storedPrint);
    if (newPrint) {
        g_object_unref(newPrint);
    }
    
    LOGI("Verification completed: matched=%s, score=%d", matched ? "YES" : "NO", score);
    
    return true;
}

bool FingerprintCapture::identifyUser(const std::map<int, std::vector<uint8_t>>& templates, int& matchedUserId, int& score)
{
    matchedUserId = -1;
    score = 0;
    
    if (!m_device) {
        setError("Device not open. Call openDevice() first.");
        return false;
    }
    
    if (!fp_device_is_open(m_device)) {
        setError("Device is not open");
        return false;
    }
    
    if (templates.empty()) {
        setError("No templates provided");
        return false;
    }
    
    LOGI("Preparing gallery for %zu users...", templates.size());
    
    // Create gallery (like Qt does)
    GPtrArray* gallery = g_ptr_array_new_with_free_func(g_object_unref);
    std::map<FpPrint*, int> printToIdMap;
    
    // Deserialize all templates
    for (const auto& pair : templates) {
        int userId = pair.first;
        const std::vector<uint8_t>& templateData = pair.second;
        
        GError* error = nullptr;
        FpPrint* print = fp_print_deserialize(templateData.data(), templateData.size(), &error);
        if (error) {
            LOGE("Skipping invalid template for user %d: %s", userId, error->message);
            g_error_free(error);
            continue;
        }
        
        g_ptr_array_add(gallery, print); // print is owned by gallery now
        printToIdMap[print] = userId;
    }
    
    if (gallery->len == 0) {
        setError("No valid templates loaded");
        g_ptr_array_unref(gallery);
        return false;
    }
    
    LOGI("Gallery prepared. Size: %u", gallery->len);
    LOGI("Starting identification scan...");
    
    // Identify - capture and match against gallery (like Qt does)
    GError* error = nullptr;
    FpPrint* matchPrint = nullptr;
    FpPrint* newPrint = nullptr;
    
    gboolean result = fp_device_identify_sync(
        m_device,
        gallery,
        nullptr, // cancellable
        nullptr, // match_cb
        nullptr, // match_data
        &matchPrint, // return matching print
        &newPrint,   // return new print
        &error
    );
    
    if (error) {
        if (error->domain == FP_DEVICE_ERROR && error->code == FP_DEVICE_ERROR_DATA_NOT_FOUND) {
            LOGI("Identify: No match found (DATA_NOT_FOUND)");
        } else {
            std::string errorMsg = "Identification failed: ";
            errorMsg += error->message;
            setError(errorMsg);
            LOGE("%s", errorMsg.c_str());
        }
        g_error_free(error);
    } else if (matchPrint) {
        // Found a match!
        auto it = printToIdMap.find(matchPrint);
        if (it != printToIdMap.end()) {
            matchedUserId = it->second;
            score = 95; // High confidence match
            LOGI("✓ IDENTIFICATION MATCH: User ID %d", matchedUserId);
        } else {
            LOGE("Match returned but not found in map!");
        }
    } else {
        LOGI("Identification completed: No match found.");
    }
    
    // Cleanup
    g_ptr_array_unref(gallery);
    if (newPrint) {
        g_object_unref(newPrint);
    }
    
    return true;
}

void FingerprintCapture::setError(const std::string& error)
{
    m_lastError = error;
    LOGE("%s", error.c_str());
}

