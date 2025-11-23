use axum::{
    extract::{Path, State},
    http::StatusCode,
    response::Json,
};
use crate::models::*;
use crate::AppState;
use tracing::{info, error};

pub async fn list_users(
    State(state): State<AppState>,
) -> Result<Json<Vec<UserResponse>>, StatusCode> {
    match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users ORDER BY created_at DESC")
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
    match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE id = $1")
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
        "INSERT INTO users (name, email) VALUES ($1, $2) RETURNING id, name, email, fingerprint_template, created_at, updated_at",
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

    match sqlx::query(
        "UPDATE users SET fingerprint_template = $1, updated_at = CURRENT_TIMESTAMP WHERE id = $2",
    )
    .bind(&request.template)
    .bind(id)
    .execute(state.db.pool())
    .await
    {
        Ok(result) => {
            if result.rows_affected() > 0 {
                // Fetch updated user
                match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE id = $1")
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
    match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE id = $1")
        .bind(id)
        .fetch_optional(state.db.pool())
        .await
    {
        Ok(Some(user)) => {
            if let Some(template) = user.fingerprint_template {
                Ok(Json(FingerprintResponse { template }))
            } else {
                Err(StatusCode::NOT_FOUND)
            }
        }
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(e) => {
            error!("Failed to get fingerprint: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn verify_fingerprint(
    State(state): State<AppState>,
    Path(id): Path<i32>,
    Json(request): Json<VerifyFingerprintRequest>,
) -> Result<Json<VerifyResponse>, StatusCode> {
    info!("Verifying fingerprint for user: {}", id);

    // Get stored template
    match sqlx::query_as::<_, User>("SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE id = $1")
        .bind(id)
        .fetch_optional(state.db.pool())
        .await
    {
        Ok(Some(user)) => {
            if let Some(stored_template) = user.fingerprint_template {
                // For PoC, simple byte comparison
                // In production, use proper fingerprint matching algorithm
                let matched = stored_template == request.template;
                let score = if matched { Some(95) } else { Some(30) };

                Ok(Json(VerifyResponse {
                    matched,
                    score: if matched { score } else { None },
                }))
            } else {
                Err(StatusCode::NOT_FOUND)
            }
        }
        Ok(None) => Err(StatusCode::NOT_FOUND),
        Err(e) => {
            error!("Failed to verify fingerprint: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}

pub async fn identify_user(
    State(state): State<AppState>,
    Json(request): Json<IdentifyFingerprintRequest>,
) -> Result<Json<IdentifyResponse>, StatusCode> {
    info!("Identifying user from fingerprint");

    // Get all users with fingerprints
    match sqlx::query_as::<_, User>(
        "SELECT id, name, email, fingerprint_template, created_at, updated_at FROM users WHERE fingerprint_template IS NOT NULL",
    )
        .fetch_all(state.db.as_ref().pool())
    .await
    {
        Ok(users) => {
            let mut best_match: Option<(i32, i32)> = None;

            for user in users {
                if let Some(stored_template) = user.fingerprint_template {
                    // Simple byte comparison for PoC
                    // In production, use proper fingerprint matching algorithm
                    if stored_template == request.template {
                        let score = 95;
                        if best_match.is_none() || best_match.unwrap().1 < score {
                            best_match = Some((user.id, score));
                        }
                    }
                }
            }

            if let Some((user_id, score)) = best_match {
                info!("User identified: {} with score: {}", user_id, score);
                Ok(Json(IdentifyResponse {
                    user_id: Some(user_id),
                    score: Some(score),
                }))
            } else {
                Ok(Json(IdentifyResponse {
                    user_id: None,
                    score: None,
                }))
            }
        }
        Err(e) => {
            error!("Failed to identify user: {}", e);
            Err(StatusCode::INTERNAL_SERVER_ERROR)
        }
    }
}
