//! libfprint wrapper FFI bindings for Rust
//! 
//! This module provides safe Rust bindings for the C wrapper functions
//! that interface with libfprint.

use std::ffi::{CStr, CString};
use std::os::raw::{c_char, c_int, c_uint, c_void};
use std::ptr;

// Opaque handle type
#[repr(transparent)]
#[derive(Clone, Copy)]
pub struct FpPrintHandle(*mut c_void);

// Error codes
#[repr(i32)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum LibfprintError {
    Success = 0,
    InvalidArg = -1,
    ParseFailed = -2,
    MatchFailed = -3,
    OutOfMemory = -4,
}

impl LibfprintError {
    fn from_code(code: c_int) -> Self {
        match code {
            0 => LibfprintError::Success,
            -1 => LibfprintError::InvalidArg,
            -2 => LibfprintError::ParseFailed,
            -3 => LibfprintError::MatchFailed,
            -4 => LibfprintError::OutOfMemory,
            _ => LibfprintError::InvalidArg,
        }
    }
}

// FFI function declarations
extern "C" {
    fn libfprint_deserialize_template(
        data: *const u8,
        len: usize,
        print_out: *mut FpPrintHandle,
        error_out: *mut *mut c_char,
    ) -> c_int;

    fn libfprint_create_print_from_image(
        image_data: *const u8,
        image_len: usize,
        width: c_uint,
        height: c_uint,
        print_out: *mut FpPrintHandle,
        error_out: *mut *mut c_char,
    ) -> c_int;

    fn libfprint_match_prints(
        template_print: FpPrintHandle,
        probe_print: FpPrintHandle,
        threshold: c_int,
        score_out: *mut c_int,
        matched_out: *mut bool,
        error_out: *mut *mut c_char,
    ) -> c_int;

    fn libfprint_free_print(print: FpPrintHandle);
    fn libfprint_free_error(error: *mut c_char);
}

/// Deserialize FP3 template data into FpPrint object
pub fn deserialize_template(data: &[u8]) -> Result<FpPrint, String> {
    let mut print_handle = FpPrintHandle(ptr::null_mut());
    let mut error_ptr: *mut c_char = ptr::null_mut();

    let result = unsafe {
        libfprint_deserialize_template(
            data.as_ptr(),
            data.len(),
            &mut print_handle,
            &mut error_ptr,
        )
    };

    if result == LibfprintError::Success as c_int {
        Ok(FpPrint::from_handle(print_handle))
    } else {
        let error_msg = if !error_ptr.is_null() {
            unsafe {
                let msg = CStr::from_ptr(error_ptr).to_string_lossy().to_string();
                libfprint_free_error(error_ptr);
                msg
            }
        } else {
            format!("Deserialization failed with code: {}", result)
        };
        Err(error_msg)
    }
}

/// Create FpPrint from raw fingerprint image
pub fn create_print_from_image(
    image_data: &[u8],
    width: u32,
    height: u32,
) -> Result<FpPrint, String> {
    let mut print_handle = FpPrintHandle(ptr::null_mut());
    let mut error_ptr: *mut c_char = ptr::null_mut();

    let result = unsafe {
        libfprint_create_print_from_image(
            image_data.as_ptr(),
            image_data.len(),
            width,
            height,
            &mut print_handle,
            &mut error_ptr,
        )
    };

    if result == LibfprintError::Success as c_int {
        Ok(FpPrint::from_handle(print_handle))
    } else {
        let error_msg = if !error_ptr.is_null() {
            unsafe {
                let msg = CStr::from_ptr(error_ptr).to_string_lossy().to_string();
                libfprint_free_error(error_ptr);
                msg
            }
        } else {
            format!("Create print from image failed with code: {}", result)
        };
        Err(error_msg)
    }
}

/// Match two FpPrint objects using bozorth3 algorithm
pub fn match_prints(
    template: &FpPrint,
    probe: &FpPrint,
    threshold: i32,
) -> Result<(bool, i32), String> {
    let mut score: c_int = 0;
    let mut matched: bool = false;
    let mut error_ptr: *mut c_char = ptr::null_mut();

    let result = unsafe {
        libfprint_match_prints(
            template.handle(),
            probe.handle(),
            threshold,
            &mut score,
            &mut matched,
            &mut error_ptr,
        )
    };

    if result == LibfprintError::Success as c_int {
        Ok((matched, score))
    } else {
        let error_msg = if !error_ptr.is_null() {
            unsafe {
                let msg = CStr::from_ptr(error_ptr).to_string_lossy().to_string();
                libfprint_free_error(error_ptr);
                msg
            }
        } else {
            format!("Match failed with code: {}", result)
        };
        Err(error_msg)
    }
}

/// Safe wrapper for FpPrintHandle with automatic cleanup
pub struct FpPrint {
    handle: FpPrintHandle,
}

impl FpPrint {
    fn from_handle(handle: FpPrintHandle) -> Self {
        Self { handle }
    }
    
    fn handle(&self) -> FpPrintHandle {
        self.handle
    }
}

impl Drop for FpPrint {
    fn drop(&mut self) {
        if !self.handle.0.is_null() {
            unsafe {
                libfprint_free_print(self.handle);
            }
        }
    }
}

