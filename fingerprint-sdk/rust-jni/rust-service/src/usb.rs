//! USB device management

use crate::error::{SDKError, SDKResult};
use std::ffi::CString;
use std::os::raw::c_int;

// External functions from libfprint (via JNI or direct linking)
extern "C" {
    fn fp_context_new() -> *mut std::ffi::c_void;
    fn fp_context_set_android_usb_fd(context: *mut std::ffi::c_void, fd: c_int) -> c_int;
    fn fp_device_capture_sync(device: *mut std::ffi::c_void, data: *mut *mut u8, len: *mut usize) -> c_int;
    fn fp_device_verify_sync(device: *mut std::ffi::c_void, template: *const u8, template_len: usize, matched: *mut c_int, score: *mut c_int) -> c_int;
    fn fp_device_identify_sync(device: *mut std::ffi::c_void, gallery: *const u8, gallery_len: usize, matched_user_id: *mut c_int, score: *mut c_int) -> c_int;
}

/// USB manager
pub struct UsbManager {
    context: Option<*mut std::ffi::c_void>,
    device: Option<*mut std::ffi::c_void>,
    usb_fd: Option<c_int>,
    initialized: bool,
}

impl UsbManager {
    /// Create new USB manager
    pub fn new() -> SDKResult<Self> {
        Ok(Self {
            context: None,
            device: None,
            usb_fd: None,
            initialized: false,
        })
    }

    /// Initialize USB device
    pub async fn initialize(&mut self) -> SDKResult<()> {
        // TODO: Get USB file descriptor from Android via JNI callback
        // For now, this is a placeholder
        
        unsafe {
            let context = fp_context_new();
            if context.is_null() {
                return Err(SDKError::DeviceInitFailed("Failed to create libfprint context".to_string()));
            }

            // TODO: Set Android USB file descriptor
            // if let Some(fd) = self.usb_fd {
            //     if fp_context_set_android_usb_fd(context, fd) == 0 {
            //         return Err(SDKError::DeviceInitFailed("Failed to set USB file descriptor".to_string()));
            //     }
            // }

            self.context = Some(context);
            self.initialized = true;
        }

        Ok(())
    }

    /// Check if device is initialized
    pub fn is_initialized(&self) -> bool {
        self.initialized
    }

    /// Capture fingerprint
    pub async fn capture_fingerprint(&mut self) -> SDKResult<Vec<u8>> {
        if !self.initialized {
            return Err(SDKError::DeviceInitFailed("Device not initialized".to_string()));
        }

        unsafe {
            let device = match self.device {
                Some(d) => d,
                None => {
                    // TODO: Get device from context
                    return Err(SDKError::DeviceInitFailed("Device not available".to_string()));
                }
            };

            let mut data: *mut u8 = std::ptr::null_mut();
            let mut len: usize = 0;

            let result = fp_device_capture_sync(device, &mut data, &mut len);
            if result != 0 {
                return Err(SDKError::CaptureFailed(format!("Capture failed with code {}", result)));
            }

            if data.is_null() || len == 0 {
                return Err(SDKError::CaptureFailed("Invalid capture data".to_string()));
            }

            let slice = std::slice::from_raw_parts(data, len);
            let fingerprint_data = slice.to_vec();

            // Free the data (if needed by libfprint)
            // libc::free(data as *mut std::ffi::c_void);

            Ok(fingerprint_data)
        }
    }

    /// Match template
    pub async fn match_template(&self, scan: &[u8], template: &[u8]) -> SDKResult<i32> {
        if !self.initialized {
            return Err(SDKError::DeviceInitFailed("Device not initialized".to_string()));
        }

        unsafe {
            let device = match self.device {
                Some(d) => d,
                None => {
                    return Err(SDKError::DeviceInitFailed("Device not available".to_string()));
                }
            };

            let mut matched: c_int = 0;
            let mut score: c_int = 0;

            let result = fp_device_verify_sync(
                device,
                template.as_ptr(),
                template.len(),
                &mut matched,
                &mut score,
            );

            if result != 0 {
                return Err(SDKError::VerificationFailed(format!("Match failed with code {}", result)));
            }

            if matched != 0 {
                Ok(score)
            } else {
                Ok(0) // No match
            }
        }
    }

    /// Set USB file descriptor (called from JNI)
    pub fn set_usb_fd(&mut self, fd: c_int) {
        self.usb_fd = Some(fd);
    }
}

impl Drop for UsbManager {
    fn drop(&mut self) {
        // Cleanup resources
        // TODO: Close device and context
    }
}

