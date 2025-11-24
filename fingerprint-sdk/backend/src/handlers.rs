use axum::{
    extract::{Path, Query},
    http::StatusCode,
    response::Json,
};
use serde::Deserialize;

use crate::models::*;
use crate::database;

/// Create user
pub async fn create_user(
    Json(request): Json<CreateUserRequest>,
) -> Result<Json<UserResponse>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!("Creating user: {} ({:?})", request.name, request.email);

    // Validate request
    if request.name.trim().is_empty() {
        return Err((
            StatusCode::BAD_REQUEST,
            Json(ErrorResponse {
                error: "Name cannot be empty".to_string(),
                code: "INVALID_REQUEST".to_string(),
            }),
        ));
    }

    match database::create_user(&request.name.trim(), request.email.as_deref().map(|e| e.trim()).filter(|e| !e.is_empty())).await {
        Ok(user) => Ok(Json(UserResponse {
            id: user.id,
            name: user.name,
            email: user.email,
            finger_count: 0,
            created_at: user.created_at,
        })),
        Err(e) => {
            let error_msg = e.to_string();
            tracing::error!("Failed to create user: {}", error_msg);
            
            // Check for duplicate email error
            if error_msg.contains("duplicate key") || error_msg.contains("unique constraint") || error_msg.contains("users_email_key") {
                return Err((
                    StatusCode::CONFLICT,
                    Json(ErrorResponse {
                        error: "Email already exists".to_string(),
                        code: "DUPLICATE_EMAIL".to_string(),
                    }),
                ));
            }
            
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to create user: {}", error_msg),
                    code: "CREATE_ERROR".to_string(),
                }),
            ))
        }
    }
}

/// List all users
pub async fn list_users() -> Result<Json<Vec<UserResponse>>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!("Listing users");

    match database::list_users().await {
        Ok(users) => {
            let response: Vec<UserResponse> = users
                .into_iter()
                .map(|(user, finger_count)| UserResponse {
                    id: user.id,
                    name: user.name,
                    email: user.email,
                    finger_count,
                    created_at: user.created_at,
                })
                .collect();
            Ok(Json(response))
        }
        Err(e) => {
            tracing::error!("Failed to list users: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to list users: {}", e),
                    code: "LIST_ERROR".to_string(),
                }),
            ))
        }
    }
}

/// Get user by ID
pub async fn get_user(
    Path(user_id): Path<i32>,
) -> Result<Json<UserResponse>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!("Getting user: {}", user_id);

    match database::get_user(user_id).await {
        Ok(Some(user)) => {
            // Get finger count
            let fingers = database::get_user_fingers(user_id).await.unwrap_or_default();
            Ok(Json(UserResponse {
                id: user.id,
                name: user.name,
                email: user.email,
                finger_count: fingers.len() as i32,
                created_at: user.created_at,
            }))
        }
        Ok(None) => Err((
            StatusCode::NOT_FOUND,
            Json(ErrorResponse {
                error: format!("User {} not found", user_id),
                code: "USER_NOT_FOUND".to_string(),
            }),
        )),
        Err(e) => {
            tracing::error!("Failed to get user: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to get user: {}", e),
                    code: "GET_ERROR".to_string(),
                }),
            ))
        }
    }
}

/// Get user's fingers
pub async fn get_user_fingers(
    Path(user_id): Path<i32>,
) -> Result<Json<Vec<String>>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!("Getting fingers for user: {}", user_id);

    match database::get_user_fingers(user_id).await {
        Ok(fingers) => Ok(Json(fingers)),
        Err(e) => {
            tracing::error!("Failed to get user fingers: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to get user fingers: {}", e),
                    code: "GET_FINGERS_ERROR".to_string(),
                }),
            ))
        }
    }
}

/// Store fingerprint template for user
pub async fn store_template(
    Path(user_id): Path<i32>,
    Json(request): Json<StoreTemplateRequest>,
) -> Result<Json<StoreTemplateResponse>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!("Storing template for user {}", user_id);

    // Validate template (basic check)
    if request.template.is_empty() {
        return Err((
            StatusCode::BAD_REQUEST,
            Json(ErrorResponse {
                error: "Template cannot be empty".to_string(),
                code: "EMPTY_TEMPLATE".to_string(),
            }),
        ));
    }

    // Store in database (template is Vec<u8> from base64_serde)
    match database::store_template(user_id, &request.template, &request.finger).await {
        Ok(_) => Ok(Json(StoreTemplateResponse {
            success: true,
            message: "Template stored successfully".to_string(),
        })),
        Err(e) => {
            tracing::error!("Failed to store template: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to store template: {}", e),
                    code: "STORAGE_ERROR".to_string(),
                }),
            ))
        }
    }
}

/// Load fingerprint template for user and finger
pub async fn load_template(
    Path(user_id): Path<i32>,
    Query(params): Query<std::collections::HashMap<String, String>>,
) -> Result<Json<TemplateResponse>, (StatusCode, Json<ErrorResponse>)> {
    let finger = params.get("finger").map(|s| s.as_str());
    tracing::info!("Loading template for user {} finger {:?}", user_id, finger);

    match database::load_template(user_id, finger).await {
        Ok(Some(template)) => Ok(Json(TemplateResponse {
            template: template.template,
            finger: template.finger,
            created_at: template.created_at,
        })),
        Ok(None) => Err((
            StatusCode::NOT_FOUND,
            Json(ErrorResponse {
                error: format!("Template not found for user {}", user_id),
                code: "TEMPLATE_NOT_FOUND".to_string(),
            }),
        )),
        Err(e) => {
            tracing::error!("Failed to load template: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to load template: {}", e),
                    code: "LOAD_ERROR".to_string(),
                }),
            ))
        }
    }
}

#[derive(Debug, Deserialize)]
pub struct TemplatesQuery {
    pub scope: Option<String>,
}

/// Load all templates (for identification)
pub async fn load_templates(
    Query(query): Query<TemplatesQuery>,
) -> Result<Json<Vec<TemplatesResponse>>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!("Loading templates (scope: {:?})", query.scope);

    match database::load_templates(query.scope.as_deref()).await {
        Ok(templates) => {
            let response: Vec<TemplatesResponse> = templates
                .into_iter()
                .map(|t| TemplatesResponse {
                    user_id: t.user_id,
                    user_name: t.user_name.unwrap_or_else(|| {
                        tracing::warn!("Template for user_id {} has NULL user_name, this should not happen", t.user_id);
                        format!("User {}", t.user_id)
                    }),
                    user_email: t.user_email,
                    template: t.template,
                    finger: t.finger,
                })
                .collect();
            Ok(Json(response))
        }
        Err(e) => {
            tracing::error!("Failed to load templates: {}", e);
            Err((
                StatusCode::INTERNAL_SERVER_ERROR,
                Json(ErrorResponse {
                    error: format!("Failed to load templates: {}", e),
                    code: "LOAD_ERROR".to_string(),
                }),
            ))
        }
    }
}

/// Log authentication event
pub async fn log_auth(
    Json(request): Json<LogAuthRequest>,
) -> Result<Json<LogAuthResponse>, (StatusCode, Json<ErrorResponse>)> {
    tracing::info!(
        "Logging auth: user_id={}, success={}, score={}",
        request.user_id,
        request.success,
        request.score
    );

    // Store log in database (fire and forget)
    let _ = database::log_auth(request.user_id, request.success, request.score).await;

    Ok(Json(LogAuthResponse { success: true }))
}

