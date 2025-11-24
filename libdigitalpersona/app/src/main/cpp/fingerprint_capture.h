#ifndef FINGERPRINT_CAPTURE_H
#define FINGERPRINT_CAPTURE_H

#include <vector>
#include <string>
#include <map>

// Forward declarations for libfprint types (using actual typedef names)
extern "C" {
    typedef struct _FpContext FpContext;
    typedef struct _FpDevice FpDevice;
}

/**
 * @brief C++ wrapper for libfprint fingerprint capture (Android version)
 * 
 * This class provides fingerprint capture functionality using libfprint
 * without Qt dependencies, suitable for Android JNI integration.
 */
class FingerprintCapture {
public:
    FingerprintCapture();
    ~FingerprintCapture();
    
    /**
     * @brief Initialize libfprint context
     * @return true on success
     */
    bool initialize();
    
    /**
     * @brief Cleanup resources
     */
    void cleanup();
    
    /**
     * @brief Get number of available devices
     */
    int getDeviceCount();
    
    /**
     * @brief Open fingerprint device
     * @param deviceIndex Device index (0-based)
     * @return true on success
     */
    bool openDevice(int deviceIndex = 0);
    
    /**
     * @brief Set USB file descriptor from Android USB Host API
     * This allows libusb to access USB device through Android's file descriptor
     * @param fd File descriptor from UsbDeviceConnection.getFileDescriptor()
     * @return true on success
     */
    bool setUsbFileDescriptor(int fd);
    
    /**
     * @brief Close device
     */
    void closeDevice();
    
    /**
     * @brief Check if device is open
     */
    bool isDeviceOpen() const;
    
    /**
     * @brief Capture fingerprint template from device
     * @param[out] templateData Captured template data (raw image)
     * @return true on success
     */
    bool captureTemplate(std::vector<uint8_t>& templateData);
    
    /**
     * @brief Match captured fingerprint with stored template (local matching using libfprint)
     * @param storedTemplate Template data from database (FP3 format)
     * @param[out] matched True if match found
     * @param[out] score Match score (0-100)
     * @return true on success (even if no match)
     */
    bool matchWithTemplate(const std::vector<uint8_t>& storedTemplate, bool& matched, int& score);
    
    /**
     * @brief Match captured fingerprint with multiple templates (identification)
     * @param templates Map of userId -> template data
     * @param[out] matchedUserId User ID of match, or -1 if no match
     * @param[out] score Match score (0-100)
     * @return true on success
     */
    bool identifyUser(const std::map<int, std::vector<uint8_t>>& templates, int& matchedUserId, int& score);
    
    /**
     * @brief Get last error message
     */
    std::string getLastError() const { return m_lastError; }
    
private:
    FpContext* m_context;
    FpDevice* m_device;
    std::string m_lastError;
    int m_usbFd; // USB file descriptor from Android
    
    void setError(const std::string& error);
};

#endif // FINGERPRINT_CAPTURE_H

