#include "fingerprint_manager.h"
#include <QDebug>
#include <cstring>

// Undefine GLib macros that conflict with Qt
#ifdef signals
#undef signals
#endif
#ifdef slots  
#undef slots
#endif
#ifdef emit
#undef emit
#endif

// Include GLib/libfprint after Qt
extern "C" {
#include <glib.h>
#include <libfprint-2/fprint.h>
}

FingerprintManager::FingerprintManager()
    : m_context(nullptr)
    , m_device(nullptr)
    , m_enrollPrint(nullptr)
    , m_enrollmentCount(0)
    , m_enrollmentInProgress(false)
{
}

FingerprintManager::~FingerprintManager()
{
    cleanup();
}

bool FingerprintManager::initialize()
{
    m_context = fp_context_new();
    if (!m_context) {
        setError("Failed to create libfprint context");
        return false;
    }
    
    return true;
}

void FingerprintManager::cleanup()
{
    closeReader();
    
    if (m_enrollPrint) {
        g_object_unref(m_enrollPrint);
        m_enrollPrint = nullptr;
    }
    
    if (m_context) {
        g_object_unref(m_context);
        m_context = nullptr;
    }
}

int FingerprintManager::getReaderCount()
{
    if (!m_context) {
        return 0;
    }
    
    GPtrArray* devices = fp_context_get_devices(m_context);
    int count = devices->len;
    g_ptr_array_unref(devices);
    
    return count;
}

bool FingerprintManager::openReader()
{
    if (m_device) {
        return true; // Already open
    }
    
    if (!m_context) {
        setError("Context not initialized");
        return false;
    }
    
    GPtrArray* devices = fp_context_get_devices(m_context);
    if (devices->len == 0) {
        g_ptr_array_unref(devices);
        setError("No fingerprint readers found");
        return false;
    }
    
    // Use first device
    m_device = FP_DEVICE(g_object_ref(g_ptr_array_index(devices, 0)));
    g_ptr_array_unref(devices);
    
    // Open device
    GError* error = nullptr;
    if (!fp_device_open_sync(m_device, nullptr, &error)) {
        setError(QString("Failed to open device: %1").arg(error->message));
        g_error_free(error);
        g_object_unref(m_device);
        m_device = nullptr;
        return false;
    }
    
    qDebug() << "Device opened:" << fp_device_get_name(m_device);
    return true;
}

void FingerprintManager::closeReader()
{
    if (m_device) {
        // Cancel any ongoing enrollment
        if (m_enrollmentInProgress) {
            m_enrollmentInProgress = false;
        }
        
        GError* error = nullptr;
        if (!fp_device_close_sync(m_device, nullptr, &error)) {
            qWarning() << "Failed to close device:" << error->message;
            g_error_free(error);
        }
        
        g_object_unref(m_device);
        m_device = nullptr;
        qDebug() << "Device closed";
    }
}

// Callback for enrollment progress
static void enroll_progress_cb(FpDevice* device, gint completed_stages, FpPrint* print, 
                                gpointer user_data, GError* error)
{
    (void)device;  // Unused
    (void)print;   // Unused
    (void)error;   // Unused
    
    int* count = (int*)user_data;
    if (count) {
        *count = completed_stages;
        
        // User-friendly progress messages
        switch(completed_stages) {
            case 1:
                qDebug() << "✓ SCAN 1/5 Complete - Lift finger and place again...";
                break;
            case 2:
                qDebug() << "✓ SCAN 2/5 Complete - Lift finger and place again...";
                break;
            case 3:
                qDebug() << "✓ SCAN 3/5 Complete - Lift finger and place again...";
                break;
            case 4:
                qDebug() << "✓ SCAN 4/5 Complete - Lift finger and place again...";
                break;
            case 5:
                qDebug() << "✓ SCAN 5/5 Complete - Processing fingerprint template...";
                break;
            default:
                qDebug() << "Enrollment progress:" << completed_stages << "stages completed";
        }
    }
}

bool FingerprintManager::startEnrollment()
{
    cancelEnrollment();
    
    if (!m_device) {
        setError("Device not open");
        return false;
    }
    
    m_enrollmentCount = 0;
    m_enrollmentInProgress = true;
    
    qDebug() << "Enrollment started - device ready for capture";
    qDebug() << "Please scan your finger 5 times when prompted";
    
    return true;
}

int FingerprintManager::addEnrollmentSample(QString& message, int& quality)
{
    if (!m_device || !m_enrollmentInProgress) {
        setError("Enrollment not started");
        return -1;
    }
    
    GError* error = nullptr;
    
    // First time - create template
    if (m_enrollmentCount == 0) {
        qDebug() << "=== ENROLLMENT STARTED ===";
        qDebug() << "You will need to scan your finger 5 times";
        qDebug() << "Please place your finger on the reader now...";
        message = "SCAN 1/5: Place your finger on the reader and hold...";
    }
    
    quality = 50;
    
    // Create template print for first scan with metadata
    FpPrint* template_print = fp_print_new(m_device);
    
    // Set required metadata to avoid NULL assertions during serialization
    fp_print_set_username(template_print, "user");
    fp_print_set_finger(template_print, FP_FINGER_UNKNOWN);
    fp_print_set_description(template_print, "enrolled");
    
    qDebug() << "Capturing enrollment samples...";
    qDebug() << "Keep finger steady on the reader...";
    
    // Enroll - this will capture all required samples
    FpPrint* enrolled_print = fp_device_enroll_sync(
        m_device, 
        template_print, 
        nullptr, 
        enroll_progress_cb,
        &m_enrollmentCount,
        &error
    );
    
    // Don't unref template_print yet - check if enrolled_print is the same object
    
    if (error) {
        QString errorMsg = QString("Enrollment failed: %1").arg(error->message);
        setError(errorMsg);
        qWarning() << errorMsg;
        g_error_free(error);
        g_object_unref(template_print);
        m_enrollmentInProgress = false;
        return -1;
    }
    
    if (!enrolled_print) {
        setError("Enrollment failed - no print returned");
        qWarning() << "Enrollment failed - no print returned";
        g_object_unref(template_print);
        m_enrollmentInProgress = false;
        return -1;
    }
    
    qDebug() << "Enrolled print received, checking validity...";
    qDebug() << "template_print ptr:" << (void*)template_print;
    qDebug() << "enrolled_print ptr:" << (void*)enrolled_print;
    
    // Clean up old enrollment if any
    if (m_enrollPrint) {
        g_object_unref(m_enrollPrint);
        m_enrollPrint = nullptr;
    }
    
    // Store the enrolled print (take ownership)
    m_enrollPrint = enrolled_print;
    
    // Unref template only if it's different from enrolled_print
    if (template_print != enrolled_print) {
        qDebug() << "Template and enrolled prints are different, unreffing template";
        g_object_unref(template_print);
    } else {
        qDebug() << "Template and enrolled prints are same object";
    }
    
    // Enrollment complete
    message = QString("✓ ENROLLMENT COMPLETE! Successfully captured %1 scans.").arg(m_enrollmentCount);
    quality = 100;
    m_enrollmentInProgress = false;
    
    qDebug() << "=== ENROLLMENT COMPLETED SUCCESSFULLY ===";
    qDebug() << "Total scans completed:" << m_enrollmentCount;
    
    return 1;
}

bool FingerprintManager::createEnrollmentTemplate(QByteArray& templateData)
{
    if (!m_enrollPrint) {
        setError("No enrollment data");
        qWarning() << "No enrollment print available";
        return false;
    }
    
    qDebug() << "Creating enrollment template...";
    qDebug() << "m_enrollPrint ptr:" << (void*)m_enrollPrint;
    
    // Check if print object is valid
    if (!FP_IS_PRINT(m_enrollPrint)) {
        setError("Invalid print object - corrupted or already freed");
        qWarning() << "ERROR: m_enrollPrint is not a valid FpPrint object!";
        return false;
    }
    
    qDebug() << "Print object is valid, checking metadata...";
    
    // Ensure metadata is set before serialization
    const gchar* existing_username = fp_print_get_username(m_enrollPrint);
    qDebug() << "Current username:" << (existing_username ? existing_username : "(null)");
    
    if (!existing_username || strlen(existing_username) == 0) {
        qDebug() << "Setting default metadata for serialization";
        fp_print_set_username(m_enrollPrint, "enrolled_user");
    }
    
    const gchar* existing_desc = fp_print_get_description(m_enrollPrint);
    qDebug() << "Current description:" << (existing_desc ? existing_desc : "(null)");
    
    if (!existing_desc || strlen(existing_desc) == 0) {
        fp_print_set_description(m_enrollPrint, "fingerprint");
    }
    
    // Serialize print
    guchar* data = nullptr;
    gsize size = 0;
    GError* error = nullptr;
    
    qDebug() << "Serializing fingerprint data...";
    
    gboolean result = fp_print_serialize(m_enrollPrint, &data, &size, &error);
    if (!result || error) {
        QString errorMsg = QString("Failed to serialize print: %1").arg(error ? error->message : "Unknown error");
        setError(errorMsg);
        qWarning() << errorMsg;
        if (error) g_error_free(error);
        return false;
    }
    
    if (!data || size == 0) {
        setError("Serialization returned empty data");
        qWarning() << "Serialization returned empty data";
        return false;
    }
    
    templateData = QByteArray((const char*)data, size);
    g_free(data);
    
    qDebug() << "Template created successfully, size:" << templateData.size() << "bytes";
    
    return true;
}

void FingerprintManager::cancelEnrollment()
{
    m_enrollmentCount = 0;
    m_enrollmentInProgress = false;
    
    if (m_enrollPrint) {
        qDebug() << "Cleaning up enrollment print";
        g_object_unref(m_enrollPrint);
        m_enrollPrint = nullptr;
    }
}

bool FingerprintManager::verifyFingerprint(const QByteArray& templateData, int& score)
{
    if (!m_device) {
        setError("Device not open");
        return false;
    }
    
    // Deserialize stored print
    GError* error = nullptr;
    FpPrint* storedPrint = fp_print_deserialize((const guchar*)templateData.constData(), templateData.size(), &error);
    if (error) {
        setError(QString("Failed to deserialize print: %1").arg(error->message));
        g_error_free(error);
        return false;
    }
    
    // Capture and match
    bool matched = false;
    bool result = captureAndMatch(storedPrint, matched, score);
    
    g_object_unref(storedPrint);
    
    return result && matched;
}

bool FingerprintManager::captureAndMatch(FpPrint* storedPrint, bool& matched, int& score)
{
    GError* error = nullptr;
    FpPrint* newPrint = nullptr;
    gboolean match = FALSE;
    
    qDebug() << "=== VERIFICATION STARTED ===";
    qDebug() << "Please place your finger on the reader...";
    qDebug() << "Waiting for finger scan...";
    
    // Verify - capture and match in one step
    gboolean result = fp_device_verify_sync(
        m_device,
        storedPrint,
        nullptr,
        nullptr,
        nullptr,
        &match,
        &newPrint,
        &error
    );
    
    if (error) {
        if (error->domain == FP_DEVICE_ERROR && error->code == FP_DEVICE_ERROR_DATA_NOT_FOUND) {
            // No match - this is normal for failed verification
            qDebug() << "Fingerprint scanned but NO MATCH";
            matched = false;
            score = 0;
            g_error_free(error);
            qDebug() << "=== VERIFICATION COMPLETED: NO MATCH ===";
            return true;
        }
        
        QString errorMsg = QString("Verification failed: %1").arg(error->message);
        setError(errorMsg);
        qWarning() << errorMsg;
        g_error_free(error);
        if (newPrint) {
            g_object_unref(newPrint);
        }
        return false;
    }
    
    if (!result) {
        setError("Verification failed - no result returned");
        qWarning() << "Verification failed - no result";
        if (newPrint) {
            g_object_unref(newPrint);
        }
        return false;
    }
    
    matched = (match == TRUE);
    
    // Calculate score based on match
    if (matched) {
        score = 95; // libfprint doesn't provide detailed score, just match/no-match
        qDebug() << "✓ FINGERPRINT MATCHED!";
    } else {
        score = 30; // Low score for no match
        qDebug() << "✗ Fingerprint does not match";
    }
    
    if (newPrint) {
        g_object_unref(newPrint);
    }
    
    qDebug() << "=== VERIFICATION COMPLETED ===";
    qDebug() << "Result: matched=" << (matched ? "YES" : "NO") << ", score=" << score << "%";
    
    return true;
}

void FingerprintManager::setError(const QString& error)
{
    m_lastError = error;
    qWarning() << "FingerprintManager Error:" << error;
}

