use axum::{
    extract::{Path, State},
    http::StatusCode,
    response::Json,
};
use crate::models::*;
use crate::AppState;
use tracing::{info, error, warn};

pub async fn list_users(
    State(state): State<AppState>,
) -> Result<Json<Vec<UserResponse>>, StatusCode> {
    match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, fingerprint_image, created_at, updated_at FROM users ORDER BY created_at DESC")
        .fetch_all(state.db.as_ref().pool())
        .await
    {
        Ok(users) => {
            let responses: Vec<UserResponse> = users.into_iter().map(UserResponse::from).collect();
            Ok(Json(responses))
        }
        Err(e) => {
            error!("Failed to list users: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_user(
    State(state): State<AppState>,
    Path(id): Path<i32>,
) -> Result<Json<UserResponse>, StatusCode> {
                match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, fingerprint_image, created_at, updated_at FROM users WHERE id = $1")
        .bind(id)
        .fetch_optional(state.db.pool())
        .await
    {
        Ok(Some(user)) => Ok(Json(UserResponse::from(user))),
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(e) => {
            error!("Failed to get user: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn create_user(
    State(state): State<AppState>,
    Json(request): Json<CreateUserRequest>,
) -> Result<Json<UserResponse>, StatusCode> {
    info!("Creating user: {} ({:?})", request.name, request.email);

    match sqlx::query_as::<_, User>(
        "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id, name, email, fingerprint_template, fingerprint_image, created_at, updated_at",
    )
    .bind(&request.name)
    .bind(&request.email)
    .fetch_one(state.db.pool())
    .await
    {
        Ok(user) => {
            info!("User created successfully with ID: {}", user.id);
            Ok(Json(UserResponse::from(user)))
        }
        Err(e) => {
            error!("Failed to create user: {}", e);
            if e.to_string().contains("duplicate key") {
                Err(StatusCode::CONFLICT)
            } else {
                Err(StatusCode::INTERNAL_SERVER_ERROR)
            }
        }
    }
}

pub async fn delete_user(
    State(state): State<AppState>,
    Path(id): Path<i32>,
) -> Result<StatusCode, StatusCode> {
    info!("Deleting user: {}", id);

    match sqlx::query("DELETE FROM users WHERE id = $1")
        .bind(id)
        .execute(state.db.as_ref().pool())
        .await
    {
        Ok(result) => {
            if result.rows_affected() > 0 {
                info!("User {} deleted successfully", id);
                Ok(StatusCode::NO_CONTENT)
            } else {
                Err(StatusCode::NOT_FOUND)
            }
        }
        Err(e) => {
            error!("Failed to delete user: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn enroll_fingerprint(
    State(state): State<AppState>,
    Path(id): Path<i32>,
    Json(request): Json<EnrollFingerprintRequest>,
) -> Result<Json<UserResponse>, StatusCode> {
    info!("Enrolling fingerprint for user: {}", id);

    // Store template (could be FP3 format) and also store as image if it's raw image format
    let image_data = if request.template.len() == 111360 {
        // This is likely a raw image (384x290 = 111360 bytes)
        Some(request.template.clone())
    } else {
        None
    };

    match sqlx::query(
        "UPDATE users SET fingerprint_template = $1, fingerprint_image = $2, updated_at = CURRENT_TIMESTAMP WHERE id = $3",
    )
    .bind(&request.template)
    .bind(&image_data)
    .bind(id)
    .execute(state.db.pool())
    .await
    {
        Ok(result) => {
            if result.rows_affected() > 0 {
                // Fetch updated user
                match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, fingerprint_image, created_at, updated_at FROM users WHERE id = $1")
                    .bind(id)
                    .fetch_one(state.db.pool())
                    .await
                {
                    Ok(user) => {
                        info!("Fingerprint enrolled successfully for user {}", id);
                        Ok(Json(UserResponse::from(user)))
                    }
                    Err(e) => {
                        error!("Failed to fetch updated user: {}", e);
                        Err(StatusCode::INTERNAL_SERVER_ERROR)
                    }
                }
            } else {
                Err(StatusCode::NOT_FOUND)
            }
        }
        Err(e) => {
            error!("Failed to enroll fingerprint: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn get_fingerprint(
    State(state): State<AppState>,
    Path(id): Path<i32>,
) -> Result<Json<FingerprintResponse>, StatusCode> {
    match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, fingerprint_image, created_at, updated_at FROM users WHERE id = $1")
        .bind(id)
        .fetch_optional(state.db.pool())
        .await
    {
        Ok(Some(user)) => {
            // For verify/identify, we need FP3 template, not raw image
            // fingerprint_template contains FP3 format, fingerprint_image is raw image (111360 bytes)
            let template = user.fingerprint_template
                .or(user.fingerprint_image) // Fallback to image if template not available (shouldn't happen)
                .ok_or(StatusCode::NOT_FOUND)?;
            info!("Returning fingerprint for user {}: template size = {} bytes", id, template.len());
            Ok(Json(FingerprintResponse { template }))
        }
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(e) => {
            error!("Failed to get fingerprint: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

// Removed: verify_fingerprint and identify_user
// Matching is now done on client side (Android) using libfprint, just like Qt application
// Middleware only provides CRUD operations for database
