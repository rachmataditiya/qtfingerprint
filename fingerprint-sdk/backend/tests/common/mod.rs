use axum::Router;
use std::sync::Once;
use sqlx::postgres::PgPoolOptions;
use sqlx::PgPool;

static INIT: Once = Once::new();

/// Initialize test database
async fn init_test_db() -> PgPool {
    INIT.call_once(|| {
        std::env::set_var("RUST_LOG", "debug");
        tracing_subscriber::fmt::init();
    });

    let database_url = std::env::var("TEST_DATABASE_URL")
        .unwrap_or_else(|_| "postgresql://postgres:postgres@localhost/test_fingerprint_db".to_string());

    let pool = PgPoolOptions::new()
        .max_connections(5)
        .connect(&database_url)
        .await
        .expect("Failed to connect to test database");

    // Clean up test database
    sqlx::query("DROP TABLE IF EXISTS fingerprints CASCADE")
        .execute(&pool)
        .await
        .ok();
    sqlx::query("DROP TABLE IF EXISTS auth_logs CASCADE")
        .execute(&pool)
        .await
        .ok();
    sqlx::query("DROP TABLE IF EXISTS migrations CASCADE")
        .execute(&pool)
        .await
        .ok();

    // Run migrations
    fingerprint_backend::database::run_migrations_postgres(&pool).await
        .expect("Failed to run migrations");

    pool
}

/// Create test app with test database
pub async fn create_test_app() -> Router {
    use fingerprint_backend::handlers;
    use axum::routing::{get, post};
    use tower_http::cors::CorsLayer;

    // Setup test database
    let _pool = init_test_db().await;
    
    // Set environment variable for database module
    std::env::set_var("DATABASE_URL", 
        std::env::var("TEST_DATABASE_URL")
            .unwrap_or_else(|_| "postgresql://postgres:postgres@localhost/test_fingerprint_db".to_string())
    );

    // Note: database::init() uses OnceLock which can only be set once
    // For tests, we need to ensure it's not already initialized
    // If it fails, it means it was already initialized (which is OK for tests)
    let _ = fingerprint_backend::database::init().await;
    
    Router::new()
        .route("/users/:user_id/fingerprint", post(handlers::store_template))
        .route("/users/:user_id/fingerprint", get(handlers::load_template))
        .route("/templates", get(handlers::load_templates))
        .route("/log_auth", post(handlers::log_auth))
        .layer(CorsLayer::permissive())
}
