//! Template management

use crate::error::{SDKError, SDKResult};
use reqwest::Client;
use serde::{Deserialize, Serialize};
use std::collections::HashMap;

/// Template manager
pub struct TemplateManager {
    client: Client,
    backend_url: String,
}

#[derive(Serialize, Deserialize)]
struct FingerprintResponse {
    template: String, // Base64 encoded
}

impl TemplateManager {
    /// Create new template manager
    pub async fn new(backend_url: &str) -> SDKResult<Self> {
        Ok(Self {
            client: Client::new(),
            backend_url: backend_url.to_string(),
        })
    }

    /// Store template for a user
    pub async fn store_template(&self, user_id: i32, template: &[u8]) -> SDKResult<()> {
        let template_base64 = base64::encode(template);
        
        let url = format!("{}/users/{}/fingerprint", self.backend_url, user_id);
        let body = serde_json::json!({
            "template": template_base64
        });

        let response = self.client
            .post(&url)
            .json(&body)
            .send()
            .await?;

        if response.status().is_success() {
            Ok(())
        } else {
            Err(SDKError::NetworkError(format!("Failed to store template: {}", response.status())))
        }
    }

    /// Load template for a user
    pub async fn load_template(&self, user_id: i32) -> SDKResult<Vec<u8>> {
        let url = format!("{}/users/{}/fingerprint", self.backend_url, user_id);
        
        let response = self.client
            .get(&url)
            .send()
            .await?;

        if !response.status().is_success() {
            return Err(SDKError::TemplateNotFound(user_id));
        }

        let fingerprint: FingerprintResponse = response.json().await?;
        let template = base64::decode(&fingerprint.template)
            .map_err(|e| SDKError::NetworkError(format!("Failed to decode template: {}", e)))?;

        Ok(template)
    }

    /// Load all templates
    pub async fn load_all_templates(&self) -> SDKResult<HashMap<i32, Vec<u8>>> {
        // First, get list of users
        let users_url = format!("{}/users", self.backend_url);
        let users_response = self.client
            .get(&users_url)
            .send()
            .await?;

        if !users_response.status().is_success() {
            return Err(SDKError::NetworkError("Failed to load users".to_string()));
        }

        #[derive(Deserialize)]
        struct User {
            id: i32,
            has_fingerprint: bool,
        }

        let users: Vec<User> = users_response.json().await?;

        // Load templates for users with fingerprints
        let mut templates = HashMap::new();
        for user in users {
            if user.has_fingerprint {
                match self.load_template(user.id).await {
                    Ok(template) => {
                        templates.insert(user.id, template);
                    }
                    Err(_) => continue, // Skip users with failed template loading
                }
            }
        }

        Ok(templates)
    }
}

