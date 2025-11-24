//! Fingerprint SDK Service
//! 
//! Rust service layer for fingerprint operations.
//! This module provides the core functionality that is exposed via JNI.

use jni::JNIEnv;
use jni::objects::{JClass, JString};
use jni::sys::{jint, jboolean, jstring};
use std::sync::Arc;
use tokio::runtime::Runtime;

mod fingerprint;
mod template;
mod database;
mod usb;
mod error;

use error::{SDKError, SDKResult};
use fingerprint::FingerprintService;

/// Global service instance
static mut SERVICE: Option<Arc<FingerprintService>> = None;
static mut RUNTIME: Option<Runtime> = None;

/// Initialize the fingerprint service
/// 
/// This must be called before any other operations.
/// 
/// # Arguments
/// * `env` - JNI environment
/// * `_class` - Java class (unused)
/// * `backend_url` - Backend API URL
/// 
/// # Returns
/// `true` if initialization successful, `false` otherwise
#[no_mangle]
pub extern "system" fn Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeInitialize(
    env: JNIEnv,
    _class: JClass,
    backend_url: JString,
) -> jboolean {
    let backend_url_str: String = match env.get_string(backend_url) {
        Ok(s) => s.into(),
        Err(_) => return false,
    };

    // Create async runtime
    let rt = match Runtime::new() {
        Ok(r) => r,
        Err(e) => {
            let _ = env.throw_new("java/lang/RuntimeException", &format!("Failed to create runtime: {}", e));
            return false;
        }
    };

    // Initialize service
    let service = match rt.block_on(FingerprintService::new(&backend_url_str)) {
        Ok(s) => s,
        Err(e) => {
            let _ = env.throw_new("java/lang/RuntimeException", &format!("Failed to initialize service: {}", e));
            return false;
        }
    };

    unsafe {
        RUNTIME = Some(rt);
        SERVICE = Some(Arc::new(service));
    }

    true
}

/// Enroll fingerprint for a user
/// 
/// # Arguments
/// * `env` - JNI environment
/// * `_class` - Java class (unused)
/// * `user_id` - User ID to enroll
/// 
/// # Returns
/// `true` if enrollment successful, `false` otherwise
#[no_mangle]
pub extern "system" fn Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeEnroll(
    env: JNIEnv,
    _class: JClass,
    user_id: jint,
) -> jboolean {
    let service = unsafe {
        match &SERVICE {
            Some(s) => s.clone(),
            None => {
                let _ = env.throw_new("java/lang/IllegalStateException", "Service not initialized");
                return false;
            }
        }
    };

    let rt = unsafe {
        match &RUNTIME {
            Some(r) => r,
            None => {
                let _ = env.throw_new("java/lang/IllegalStateException", "Runtime not initialized");
                return false;
            }
        }
    };

    match rt.block_on(service.enroll_fingerprint(user_id)) {
        Ok(_) => true,
        Err(e) => {
            let _ = env.throw_new("java/lang/Exception", &format!("Enrollment failed: {}", e));
            false
        }
    }
}

/// Verify fingerprint for a user
/// 
/// # Arguments
/// * `env` - JNI environment
/// * `_class` - Java class (unused)
/// * `user_id` - User ID to verify
/// 
/// # Returns
/// Match score (0-100) if successful, -1 on error
#[no_mangle]
pub extern "system" fn Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeVerify(
    env: JNIEnv,
    _class: JClass,
    user_id: jint,
) -> jint {
    let service = unsafe {
        match &SERVICE {
            Some(s) => s.clone(),
            None => {
                let _ = env.throw_new("java/lang/IllegalStateException", "Service not initialized");
                return -1;
            }
        }
    };

    let rt = unsafe {
        match &RUNTIME {
            Some(r) => r,
            None => {
                let _ = env.throw_new("java/lang/IllegalStateException", "Runtime not initialized");
                return -1;
            }
        }
    };

    match rt.block_on(service.verify_fingerprint(user_id)) {
        Ok(score) => score as jint,
        Err(e) => {
            let _ = env.throw_new("java/lang/Exception", &format!("Verification failed: {}", e));
            -1
        }
    }
}

/// Identify user from fingerprint (1:N matching)
/// 
/// # Arguments
/// * `env` - JNI environment
/// * `_class` - Java class (unused)
/// 
/// # Returns
/// User ID if match found, -1 if no match or error
#[no_mangle]
pub extern "system" fn Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeIdentify(
    env: JNIEnv,
    _class: JClass,
) -> jint {
    let service = unsafe {
        match &SERVICE {
            Some(s) => s.clone(),
            None => {
                let _ = env.throw_new("java/lang/IllegalStateException", "Service not initialized");
                return -1;
            }
        }
    };

    let rt = unsafe {
        match &RUNTIME {
            Some(r) => r,
            None => {
                let _ = env.throw_new("java/lang/IllegalStateException", "Runtime not initialized");
                return -1;
            }
        }
    };

    match rt.block_on(service.identify_fingerprint()) {
        Ok(Some(user_id)) => user_id as jint,
        Ok(None) => -1, // No match found
        Err(e) => {
            let _ = env.throw_new("java/lang/Exception", &format!("Identification failed: {}", e));
            -1
        }
    }
}

/// Cleanup service resources
/// 
/// # Arguments
/// * `env` - JNI environment
/// * `_class` - Java class (unused)
#[no_mangle]
pub extern "system" fn Java_com_arkana_fingerprintsdk_FingerprintSDK_nativeCleanup(
    _env: JNIEnv,
    _class: JClass,
) {
    unsafe {
        SERVICE = None;
        RUNTIME = None;
    }
}

