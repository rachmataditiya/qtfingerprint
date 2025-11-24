use serde::{Deserialize, Serialize};
use chrono::NaiveDateTime;
use sqlx::FromRow;

#[derive(Debug, Serialize, Deserialize, Clone, FromRow)]
pub struct User {
    pub id: i32,
    pub name: String,
    pub email: Option<String>,
    #[serde(skip_serializing)]
    pub fingerprint_template: Option<Vec<u8>>,
    #[serde(skip_serializing)]
    pub fingerprint_image: Option<Vec<u8>>,
    pub created_at: NaiveDateTime,
    pub updated_at: Option<NaiveDateTime>,
}

#[derive(Debug, Serialize, Deserialize)]
pub struct UserResponse {
    pub id: i32,
    pub name: String,
    pub email: Option<String>,
    pub has_fingerprint: bool,
    pub created_at: NaiveDateTime,
    pub updated_at: Option<NaiveDateTime>,
}

impl From<User> for UserResponse {
    fn from(user: User) -> Self {
        UserResponse {
            id: user.id,
            name: user.name,
            email: user.email,
            has_fingerprint: user.fingerprint_template.is_some(),
            created_at: user.created_at,
            updated_at: user.updated_at,
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct CreateUserRequest {
    pub name: String,
    pub email: Option<String>,
}

#[derive(Debug, Deserialize)]
pub struct EnrollFingerprintRequest {
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>,
}

#[derive(Debug, Deserialize)]
pub struct VerifyFingerprintRequest {
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>,
}

#[derive(Debug, Deserialize)]
pub struct IdentifyFingerprintRequest {
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>,
}

#[derive(Debug, Serialize)]
pub struct VerifyResponse {
    pub matched: bool,
    pub score: Option<i32>,
}

#[derive(Debug, Serialize)]
pub struct IdentifyResponse {
    pub user_id: Option<i32>,
    pub score: Option<i32>,
}

#[derive(Debug, Serialize)]
pub struct FingerprintResponse {
    #[serde(with = "base64_serde")]
    pub template: Vec<u8>,
}

// Helper module for base64 serialization
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

