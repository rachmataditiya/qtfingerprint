use serde::{Deserialize, Serialize};

/// User model
#[derive(Debug, Serialize, Deserialize)]
pub struct User {
    pub id: i32,
    pub name: String,
    pub email: Option<String>,
    pub created_at: Option<String>,
    pub updated_at: Option<String>,
}

/// Request to create user
#[derive(Debug, Deserialize)]
pub struct CreateUserRequest {
    pub name: String,
    pub email: Option<String>,
}

/// Response for user list
#[derive(Debug, Serialize)]
pub struct UserResponse {
    pub id: i32,
    pub name: String,
    pub email: Option<String>,
    pub finger_count: i32,
    pub created_at: Option<String>,
}

/// Request to store fingerprint template
#[derive(Debug, Deserialize)]
pub struct StoreTemplateRequest {
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>, // FP3 template (Base64 decoded from JSON)
    pub finger: String,   // Finger type (e.g., "LEFT_INDEX")
}

/// Response for template storage
#[derive(Debug, Serialize)]
pub struct StoreTemplateResponse {
    pub success: bool,
    pub message: String,
}

/// Response for template retrieval
#[derive(Debug, Serialize)]
pub struct TemplateResponse {
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>, // FP3 template (Base64 encoded in JSON)
    pub finger: String,
    #[serde(skip_serializing_if = "Option::is_none")]
    pub created_at: Option<String>,
}

/// Response for multiple templates
#[derive(Debug, Serialize)]
pub struct TemplatesResponse {
    pub user_id: i32,
    pub user_name: String,
    pub user_email: Option<String>,
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>, // FP3 template (Base64 encoded in JSON)
    pub finger: String,
}

// Helper module for base64 serialization (same as middleware POC)
mod base64_serde {
    use serde::{Deserializer, Serializer, Deserialize};
    use base64::{Engine as _, engine::general_purpose};
    
    pub fn serialize<S>(bytes: &[u8], serializer: S) -> Result<S::Ok, S::Error>
    where
        S: Serializer,
    {
        let encoded = general_purpose::STANDARD.encode(bytes);
        serializer.serialize_str(&encoded)
    }

    pub fn deserialize<'de, D>(deserializer: D) -> Result<Vec<u8>, D::Error>
    where
        D: Deserializer<'de>,
    {
        let s = String::deserialize(deserializer)?;
        general_purpose::STANDARD.decode(&s)
            .map_err(|e| serde::de::Error::custom(format!("Invalid base64 string: {}", e)))
    }
}

/// Request to log authentication
#[derive(Debug, Deserialize)]
pub struct LogAuthRequest {
    pub user_id: i32,
    pub success: bool,
    pub score: f32,
}

/// Response for log authentication
#[derive(Debug, Serialize)]
pub struct LogAuthResponse {
    pub success: bool,
}

/// Error response
#[derive(Debug, Serialize)]
pub struct ErrorResponse {
    pub error: String,
    pub code: String,
}

