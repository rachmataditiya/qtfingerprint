// Image comparison utilities for fingerprint matching
// Uses histogram and structural similarity for image comparison

use std::cmp;

/// Calculate histogram for grayscale image
pub fn calculate_histogram(image: &[u8], _width: usize, _height: usize) -> Vec<usize> {
    let mut histogram = vec![0; 256];
    
    for &pixel in image.iter() {
        histogram[pixel as usize] += 1;
    }
    
    histogram
}

/// Calculate normalized histogram (0.0 to 1.0)
pub fn calculate_normalized_histogram(image: &[u8]) -> Vec<f64> {
    let histogram = calculate_histogram(image, 0, 0);
    let total_pixels = image.len() as f64;
    
    histogram.iter().map(|&count| count as f64 / total_pixels).collect()
}

/// Calculate histogram intersection similarity (0.0 to 1.0)
pub fn histogram_intersection(hist1: &[f64], hist2: &[f64]) -> f64 {
    let mut intersection = 0.0;
    let mut total = 0.0;
    
    for i in 0..256 {
        let min_val = hist1[i].min(hist2[i]);
        intersection += min_val;
        total += hist1[i].max(hist2[i]);
    }
    
    if total > 0.0 {
        intersection / total
    } else {
        0.0
    }
}

/// Calculate correlation coefficient between two histograms
pub fn histogram_correlation(hist1: &[f64], hist2: &[f64]) -> f64 {
    let mean1: f64 = hist1.iter().sum::<f64>() / hist1.len() as f64;
    let mean2: f64 = hist2.iter().sum::<f64>() / hist2.len() as f64;
    
    let mut numerator = 0.0;
    let mut denom1 = 0.0;
    let mut denom2 = 0.0;
    
    for i in 0..hist1.len() {
        let diff1 = hist1[i] - mean1;
        let diff2 = hist2[i] - mean2;
        numerator += diff1 * diff2;
        denom1 += diff1 * diff1;
        denom2 += diff2 * diff2;
    }
    
    let denominator = (denom1 * denom2).sqrt();
    if denominator > 0.0 {
        numerator / denominator
    } else {
        0.0
    }
}

/// Calculate mean squared error between two images
pub fn mean_squared_error(img1: &[u8], img2: &[u8]) -> f64 {
    if img1.len() != img2.len() {
        return f64::MAX;
    }
    
    let mut mse = 0.0;
    for i in 0..img1.len() {
        let diff = img1[i] as i32 - img2[i] as i32;
        mse += (diff * diff) as f64;
    }
    
    mse / img1.len() as f64
}

/// Calculate structural similarity index (SSIM) - simplified version
pub fn calculate_ssim_simple(img1: &[u8], img2: &[u8], width: usize, height: usize) -> f64 {
    if img1.len() != img2.len() || img1.len() != width * height {
        return 0.0;
    }
    
    // Calculate mean
    let mean1: f64 = img1.iter().map(|&x| x as f64).sum::<f64>() / img1.len() as f64;
    let mean2: f64 = img2.iter().map(|&x| x as f64).sum::<f64>() / img2.len() as f64;
    
    // Calculate variance and covariance
    let mut var1 = 0.0;
    let mut var2 = 0.0;
    let mut cov = 0.0;
    
    for i in 0..img1.len() {
        let diff1 = img1[i] as f64 - mean1;
        let diff2 = img2[i] as f64 - mean2;
        var1 += diff1 * diff1;
        var2 += diff2 * diff2;
        cov += diff1 * diff2;
    }
    
    let n = img1.len() as f64;
    var1 /= n;
    var2 /= n;
    cov /= n;
    
    // SSIM constants
    let c1 = 0.01;
    let c2 = 0.03;
    
    // SSIM formula
    let ssim = ((2.0 * mean1 * mean2 + c1) * (2.0 * cov + c2)) / 
               ((mean1 * mean1 + mean2 * mean2 + c1) * (var1 + var2 + c2));
    
    ssim.max(0.0).min(1.0)
}

/// Extract image from libfprint FP3 template (if template contains image data)
/// Returns None if template doesn't contain image or can't be parsed
pub fn extract_image_from_template(template: &[u8]) -> Option<Vec<u8>> {
    // Check if it's FP3 format (starts with "FP3")
    if template.len() >= 3 && &template[0..3] == b"FP3" {
        // FP3 format: 3-byte header "FP3" + GVariant data
        // The template contains serialized minutiae, not raw image
        // Cannot extract image from FP3 - it only contains minutiae data
        // For matching FP3 templates, we would need to:
        // 1. Extract minutiae from incoming raw image
        // 2. Compare minutiae using bozorth3 algorithm
        // For now, return None to indicate no image available
        None
    } else if template.len() == 111360 {
        // This is a raw image (384x290 = 111360 bytes)
        Some(template.to_vec())
    } else if template.len() > 1000 && template.len() < 5000 {
        // This might be FP3 template (typical size 2-4KB)
        // FP3 templates don't contain raw image, only minutiae
        None
    } else {
        // Unknown format, try as image if size matches
        if template.len() == 384 * 290 {
            Some(template.to_vec())
        } else {
            None
        }
    }
}

/// Match FP3 template (minutiae) against raw image
/// Uses simplified minutiae extraction and matching
/// Returns similarity score and match status
pub fn match_fp3_template_with_image(fp3_template: &[u8], image: &[u8], threshold: f64) -> (f64, bool) {
    use crate::minutiae_match::match_image_with_fp3_template;
    match_image_with_fp3_template(image, fp3_template, threshold)
}

/// Compare two fingerprint images using multiple metrics
pub fn compare_fingerprint_images(img1: &[u8], img2: &[u8], width: usize, height: usize) -> f64 {
    if img1.len() != img2.len() || img1.len() != width * height {
        return 0.0;
    }
    
    // Normalize images if needed (ensure same size)
    let img1_norm = if img1.len() == width * height {
        img1
    } else {
        return 0.0;
    };
    
    let img2_norm = if img2.len() == width * height {
        img2
    } else {
        return 0.0;
    };
    
    // Calculate multiple similarity metrics
    let hist1 = calculate_normalized_histogram(img1_norm);
    let hist2 = calculate_normalized_histogram(img2_norm);
    
    let hist_intersection = histogram_intersection(&hist1, &hist2);
    let hist_correlation = histogram_correlation(&hist1, &hist2);
    let ssim = calculate_ssim_simple(img1_norm, img2_norm, width, height);
    let mse = mean_squared_error(img1_norm, img2_norm);
    
    // Convert MSE to similarity (lower MSE = higher similarity)
    let mse_similarity = 1.0 / (1.0 + mse / 10000.0);
    
    // Weighted combination of metrics
    // SSIM is most important, then histogram, then correlation
    let similarity = (ssim * 0.5) + (hist_intersection * 0.3) + (hist_correlation * 0.1) + (mse_similarity * 0.1);
    
    similarity
}

/// Match fingerprint image against template
/// Returns similarity score (0.0 to 1.0) and match status
pub fn match_fingerprint(image: &[u8], template: &[u8], threshold: f64) -> (f64, bool) {
    const FINGERPRINT_WIDTH: usize = 384;
    const FINGERPRINT_HEIGHT: usize = 290;
    const EXPECTED_IMAGE_SIZE: usize = FINGERPRINT_WIDTH * FINGERPRINT_HEIGHT; // 111360 bytes
    
    // If template is FP3 format (contains serialized minutiae, not image)
    // Use minutiae-based matching
    if template.len() >= 3 && &template[0..3] == b"FP3" {
        // FP3 template contains minutiae data, not raw image
        // Use FP3 parser and minutiae matching
        return match_fp3_template_with_image(image, template, threshold);
    }
    
    // Try to extract image from template
    let template_image = match extract_image_from_template(template) {
        Some(img) => img,
        None => {
            // Template is not in image format and not extractable
            // Return no match
            return (0.0, false);
        }
    };
    
    // Ensure both images are the correct size
    if image.len() != EXPECTED_IMAGE_SIZE {
        return (0.0, false);
    }
    
    if template_image.len() != EXPECTED_IMAGE_SIZE {
        return (0.0, false);
    }
    
    // Compare images using multiple metrics
    let similarity = compare_fingerprint_images(
        image,
        &template_image,
        FINGERPRINT_WIDTH,
        FINGERPRINT_HEIGHT,
    );
    
    let matched = similarity >= threshold;
    
    (similarity, matched)
}

