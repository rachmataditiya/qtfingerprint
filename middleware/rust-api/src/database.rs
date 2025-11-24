use sqlx::{PgPool, Postgres, Pool, postgres::PgPoolOptions};
use std::time::Duration;
use tracing::{info, error};

pub struct Database {
    pool: PgPool,
}

impl Database {
    pub async fn new(database_url: &str) -> anyhow::Result<Self> {
        let pool = PgPoolOptions::new()
            .max_connections(10)
            .acquire_timeout(Duration::from_secs(30))
            .connect(database_url)
            .await?;

        Ok(Self { pool })
    }

    pub fn pool(&self) -> &PgPool {
        &self.pool
    }

    pub async fn run_migrations(&self) -> anyhow::Result<()> {
        info!("Running database migrations...");

        // Create users table if not exists
        sqlx::query(
            r#"
            CREATE TABLE IF NOT EXISTS users (
                id SERIAL PRIMARY KEY,
                name VARCHAR(255) NOT NULL,
                email VARCHAR(255) UNIQUE,
                fingerprint_template BYTEA,
                created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
                updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
            )
            "#,
        )
        .execute(&self.pool)
        .await?;

        // Add fingerprint_image column if it doesn't exist (migration)
        sqlx::query(
            r#"
            ALTER TABLE users ADD COLUMN IF NOT EXISTS fingerprint_image BYTEA
            "#,
        )
        .execute(&self.pool)
        .await
        .ok(); // Ignore error if column already exists

        // Create index on email for faster lookups
        sqlx::query(
            r#"
            CREATE INDEX IF NOT EXISTS idx_users_email ON users(email)
            "#,
        )
        .execute(&self.pool)
        .await?;

        // Create index on name for search
        sqlx::query(
            r#"
            CREATE INDEX IF NOT EXISTS idx_users_name ON users(name)
            "#,
        )
        .execute(&self.pool)
        .await?;

        info!("Migrations completed successfully");
        Ok(())
    }
}

