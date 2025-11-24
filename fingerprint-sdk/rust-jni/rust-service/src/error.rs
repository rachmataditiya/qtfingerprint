//! Error types for the SDK service

use thiserror::Error;

/// SDK Result type
pub type SDKResult<T> = Result<T, SDKError>;

/// SDK Error types
#[derive(Error, Debug)]
pub enum SDKError {
    #[error("USB device not found or not connected")]
    UsbDeviceNotFound,
    
    #[error("USB permission denied")]
    UsbPermissionDenied,
    
    #[error("Device initialization failed: {0}")]
    DeviceInitFailed(String),
    
    #[error("Fingerprint capture failed: {0}")]
    CaptureFailed(String),
    
    #[error("Template not found for user {0}")]
    TemplateNotFound(i32),
    
    #[error("Database error: {0}")]
    DatabaseError(String),
    
    #[error("Network error: {0}")]
    NetworkError(String),
    
    #[error("Enrollment failed: {0}")]
    EnrollmentFailed(String),
    
    #[error("Verification failed: {0}")]
    VerificationFailed(String),
    
    #[error("Identification failed: {0}")]
    IdentificationFailed(String),
    
    #[error("Operation timeout")]
    Timeout,
    
    #[error("Service not initialized")]
    NotInitialized,
    
    #[error("Unknown error: {0}")]
    Unknown(String),
}

impl From<sqlx::Error> for SDKError {
    fn from(err: sqlx::Error) -> Self {
        SDKError::DatabaseError(err.to_string())
    }
}

impl From<reqwest::Error> for SDKError {
    fn from(err: reqwest::Error) -> Self {
        SDKError::NetworkError(err.to_string())
    }
}

