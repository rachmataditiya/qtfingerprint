use axum::{
    routing::{get, post},
    Router,
};
use tower_http::cors::CorsLayer;

use fingerprint_backend::handlers::{
    create_user, get_user, get_user_fingers, list_users,
    load_template, load_templates, log_auth, store_template,
};
use fingerprint_backend::database;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    // Initialize tracing
    tracing_subscriber::fmt::init();

    // Initialize database
    database::init().await?;

    // Build application
    let app = Router::new()
        // User endpoints
        .route("/users", post(create_user))
        .route("/users", get(list_users))
        .route("/users/:user_id", get(get_user))
        .route("/users/:user_id/fingers", get(get_user_fingers))
        // Fingerprint endpoints
        .route("/users/:user_id/fingerprint", post(store_template))
        .route("/users/:user_id/fingerprint", get(load_template))
        .route("/templates", get(load_templates))
        .route("/log_auth", post(log_auth))
        .layer(CorsLayer::permissive());

    // Run server
    let listener = tokio::net::TcpListener::bind("0.0.0.0:3000").await?;
    tracing::info!("Server running on http://0.0.0.0:3000");
    
    axum::serve(listener, app).await?;

    Ok(())
}

