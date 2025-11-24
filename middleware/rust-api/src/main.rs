use axum::{
    extract::Path,
    response::Json,
    routing::{get, post, delete},
    Router,
};
use std::sync::Arc;
use tower_http::cors::{Any, CorsLayer};
use tracing::info;
mod database;
mod models;
mod handlers;

use database::Database;

#[derive(Clone)]
struct AppState {
    db: Arc<Database>,
}

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    // Initialize tracing
    tracing_subscriber::fmt()
        .with_env_filter(tracing_subscriber::EnvFilter::from_default_env())
        .init();

    // Load environment variables
    dotenvy::dotenv().ok();

    // Get database URL from environment
    let database_url = std::env::var("DATABASE_URL")
        .unwrap_or_else(|_| "postgresql://postgres:postgres@localhost:5432/fingerprint_db".to_string());

    info!("Connecting to database...");
    let db = Database::new(&database_url).await?;
    
    info!("Running migrations...");
    db.run_migrations().await?;
    
    info!("Database initialized successfully");

    // Create app state
    let app_state = AppState { db: Arc::new(db) };

    // Configure CORS
    let cors = CorsLayer::new()
        .allow_origin(Any)
        .allow_methods(Any)
        .allow_headers(Any);

    // Build application routes - Simple CRUD only, no fingerprint matching
    let app = Router::new()
        .route("/health", get(health_check))
        .route("/users", get(handlers::list_users))
        .route("/users", post(handlers::create_user))
        .route("/users/:id", get(handlers::get_user))
        .route("/users/:id", delete(handlers::delete_user))
        .route("/users/:id/fingerprint", post(handlers::enroll_fingerprint))
        .route("/users/:id/fingerprint", get(handlers::get_fingerprint))
        .layer(cors)
        .with_state(app_state);

    // Start server
    let addr = std::env::var("SERVER_ADDR")
        .unwrap_or_else(|_| "0.0.0.0:3000".to_string());
    
    info!("Starting server on {}", addr);
    let listener = tokio::net::TcpListener::bind(&addr).await?;
    axum::serve(listener, app).await?;

    Ok(())
}

async fn health_check() -> Json<serde_json::Value> {
    Json(serde_json::json!({
        "status": "ok",
        "service": "fingerprint-api"
    }))
}

