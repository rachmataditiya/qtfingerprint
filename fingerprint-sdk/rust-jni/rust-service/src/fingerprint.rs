//! Fingerprint operations

use crate::error::{SDKError, SDKResult};
use crate::template::TemplateManager;
use crate::usb::UsbManager;
use std::sync::Arc;
use tokio::sync::Mutex;

/// Fingerprint service
pub struct FingerprintService {
    usb_manager: Arc<Mutex<UsbManager>>,
    template_manager: Arc<TemplateManager>,
    enrollment_scans: u32,
    match_threshold: i32,
}

impl FingerprintService {
    /// Create new fingerprint service
    pub async fn new(backend_url: &str) -> SDKResult<Self> {
        let usb_manager = Arc::new(Mutex::new(UsbManager::new()?));
        let template_manager = Arc::new(TemplateManager::new(backend_url).await?);
        
        Ok(Self {
            usb_manager,
            template_manager,
            enrollment_scans: 5,
            match_threshold: 60,
        })
    }

    /// Enroll fingerprint for a user
    pub async fn enroll_fingerprint(&self, user_id: i32) -> SDKResult<()> {
        // Ensure device is initialized
        let mut usb = self.usb_manager.lock().await;
        if !usb.is_initialized() {
            usb.initialize().await?;
        }

        // Capture multiple scans for enrollment
        let mut scans = Vec::new();
        for i in 0..self.enrollment_scans {
            // Capture scan
            let scan = usb.capture_fingerprint().await?;
            scans.push(scan);
            
            // TODO: Add progress callback
        }

        // Create template from scans
        let template = self.create_template_from_scans(&scans)?;

        // Store template in database
        self.template_manager.store_template(user_id, &template).await?;

        Ok(())
    }

    /// Verify fingerprint for a user
    pub async fn verify_fingerprint(&self, user_id: i32) -> SDKResult<i32> {
        // Ensure device is initialized
        let mut usb = self.usb_manager.lock().await;
        if !usb.is_initialized() {
            usb.initialize().await?;
        }

        // Load stored template
        let stored_template = self.template_manager.load_template(user_id).await?;

        // Capture current fingerprint
        let current_scan = usb.capture_fingerprint().await?;

        // Match current scan with stored template
        let score = usb.match_template(&current_scan, &stored_template).await?;

        if score >= self.match_threshold {
            Ok(score)
        } else {
            Err(SDKError::VerificationFailed(format!("Score {} below threshold {}", score, self.match_threshold)))
        }
    }

    /// Identify user from fingerprint (1:N matching)
    pub async fn identify_fingerprint(&self) -> SDKResult<Option<i32>> {
        // Ensure device is initialized
        let mut usb = self.usb_manager.lock().await;
        if !usb.is_initialized() {
            usb.initialize().await?;
        }

        // Load all templates
        let templates = self.template_manager.load_all_templates().await?;

        if templates.is_empty() {
            return Ok(None);
        }

        // Capture current fingerprint
        let current_scan = usb.capture_fingerprint().await?;

        // Match against all templates
        let mut best_match: Option<(i32, i32)> = None; // (user_id, score)

        for (user_id, template) in templates {
            match usb.match_template(&current_scan, &template).await {
                Ok(score) => {
                    if score >= self.match_threshold {
                        match best_match {
                            Some((_, best_score)) if score > best_score => {
                                best_match = Some((user_id, score));
                            }
                            None => {
                                best_match = Some((user_id, score));
                            }
                            _ => {}
                        }
                    }
                }
                Err(_) => continue, // Skip failed matches
            }
        }

        Ok(best_match.map(|(user_id, _)| user_id))
    }

    /// Create template from multiple scans
    fn create_template_from_scans(&self, scans: &[Vec<u8>]) -> SDKResult<Vec<u8>> {
        // TODO: Implement template creation from multiple scans
        // For now, use the first scan as template
        if scans.is_empty() {
            return Err(SDKError::EnrollmentFailed("No scans provided".to_string()));
        }
        
        // In real implementation, this would combine multiple scans
        // using libfprint's template creation
        Ok(scans[0].clone())
    }
}

