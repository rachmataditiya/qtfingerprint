#include <android/log.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

// Include libfprint headers
extern "C" {
#include <glib.h>
#include <glib-object.h>
#include <libfprint-2/fprint.h>
#include <libusb-1.0/libusb.h>
}

// Include internal header untuk akses ke FpContextPrivate
// Note: Ini adalah internal API, tapi kita perlu untuk mengakses GUsbContext
#include "../libfprint_repo/libfprint/fpi-context.h"

#define LOG_TAG "TestLibfprint"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

// Forward declaration untuk fungsi yang akan kita test
extern "C" {
    // Fungsi untuk mendapatkan GUsbContext dari FpContext
    // Menggunakan internal API fp_context_get_instance_private()
    GUsbContext* get_gusb_context_from_fp_context(FpContext* context);
    
    // Fungsi untuk mendapatkan libusb_context dari GUsbContext
    // Ini memerlukan akses ke internal structure GUsbContext
    libusb_context* get_libusb_context_from_gusb_context(GUsbContext* gusb_ctx);
    
    // Fungsi untuk wrap file descriptor ke libusb device
    bool wrap_file_descriptor_to_libusb(libusb_context* ctx, int fd);
}

// Test function untuk mengintegrasikan file descriptor dengan libfprint
bool test_integrate_file_descriptor(int fd) {
    LOGI("=== TEST: Integrate File Descriptor with libfprint ===");
    LOGI("File descriptor: %d", fd);
    
    if (fd < 0) {
        LOGE("Invalid file descriptor: %d", fd);
        return false;
    }
    
    // Step 1: Create FpContext
    LOGI("Step 1: Creating FpContext...");
    FpContext* context = fp_context_new();
    if (!context) {
        LOGE("Failed to create FpContext");
        return false;
    }
    LOGI("✓ FpContext created successfully");
    
    // Step 2: Get FpContextPrivate untuk mengakses GUsbContext
    LOGI("Step 2: Accessing FpContextPrivate...");
    FpContextPrivate* priv = fp_context_get_instance_private(context);
    if (!priv) {
        LOGE("Failed to get FpContextPrivate");
        g_object_unref(context);
        return false;
    }
    LOGI("✓ FpContextPrivate accessed");
    
    // Step 3: Get GUsbContext dari private data
    LOGI("Step 3: Getting GUsbContext from private data...");
    GUsbContext* gusb_ctx = priv->usb_ctx;
    if (!gusb_ctx) {
        LOGE("GUsbContext is NULL in FpContextPrivate");
        g_object_unref(context);
        return false;
    }
    LOGI("✓ GUsbContext obtained: %p", gusb_ctx);
    
    // Step 4: Get libusb_context dari GUsbContext
    // Note: Ini memerlukan akses ke internal structure GUsbContext
    // Kita perlu menggunakan GObject introspection atau direct structure access
    LOGI("Step 4: Getting libusb_context from GUsbContext...");
    
    // Coba akses langsung ke structure GUsbContext
    // GUsbContext adalah GObject, kita perlu mendapatkan private data-nya
    // Tapi ini memerlukan header internal GUsbContext yang tidak tersedia
    
    // Alternatif: Gunakan libusb_wrap_sys_device() dengan libusb context yang sudah ada
    // Tapi kita perlu mendapatkan libusb_context dari GUsbContext terlebih dahulu
    
    // Untuk sekarang, kita coba pendekatan lain:
    // 1. Set environment variable LIBUSB_FD
    // 2. Trigger enumeration ulang
    // 3. Lihat apakah device terdeteksi
    
    LOGI("Step 5: Setting LIBUSB_FD environment variable...");
    char fd_str[32];
    snprintf(fd_str, sizeof(fd_str), "%d", fd);
    g_setenv("LIBUSB_FD", fd_str, TRUE);
    LOGI("✓ LIBUSB_FD set to: %s", fd_str);
    
    // Step 6: Enumerate devices
    LOGI("Step 6: Enumerating devices...");
    fp_context_enumerate(context);
    LOGI("✓ Device enumeration completed");
    
    // Step 7: Check device count
    LOGI("Step 7: Checking device count...");
    GPtrArray* devices = fp_context_get_devices(context);
    if (!devices) {
        LOGE("Failed to get devices array");
        g_object_unref(context);
        return false;
    }
    
    int count = (int)devices->len;
    LOGI("Found %d fingerprint device(s)", count);
    
    if (count > 0) {
        LOGI("✓ SUCCESS: Device detected!");
        for (int i = 0; i < count; i++) {
            FpDevice* device = (FpDevice*)g_ptr_array_index(devices, i);
            if (device) {
                const char* name = fp_device_get_name(device);
                LOGI("  Device %d: %s", i, name ? name : "(unknown)");
            }
        }
    } else {
        LOGW("⚠ No devices found");
    }
    
    g_ptr_array_unref(devices);
    g_object_unref(context);
    
    return count > 0;
}

// Main test function
extern "C" JNIEXPORT jboolean JNICALL
Java_com_arkana_libdigitalpersona_TestLibfprint_nativeTestIntegration(
    JNIEnv *env,
    jobject /* this */,
    jint fd) {
    
    LOGI("=== Starting libfprint integration test ===");
    
    bool result = test_integrate_file_descriptor((int)fd);
    
    if (result) {
        LOGI("=== TEST PASSED ===");
    } else {
        LOGE("=== TEST FAILED ===");
    }
    
    return result ? JNI_TRUE : JNI_FALSE;
}

