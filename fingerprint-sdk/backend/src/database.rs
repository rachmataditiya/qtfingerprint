use sqlx::{Row, Executor};
use sqlx::postgres::PgPool;
use sqlx::mysql::MySqlPool;
use std::env;
use anyhow::Context;

pub enum DatabaseType {
    Postgres,
    MySql,
}

#[derive(Debug)]
pub enum DatabasePool {
    Postgres(PgPool),
    MySql(MySqlPool),
}

pub struct UserRecord {
    pub id: i32,
    pub name: String,
    pub email: Option<String>,
    pub created_at: Option<String>,
    pub updated_at: Option<String>,
}

pub struct TemplateRecord {
    pub user_id: i32,
    pub template: Vec<u8>, // Binary data (BYTEA)
    pub finger: String,
    pub created_at: Option<String>,
    pub user_name: Option<String>,
    pub user_email: Option<String>,
}

use std::sync::OnceLock;

static POOL: OnceLock<DatabasePool> = OnceLock::new();

/// Detect database type from DATABASE_URL
fn detect_database_type(database_url: &str) -> DatabaseType {
    if database_url.starts_with("mysql://") || database_url.starts_with("mariadb://") {
        DatabaseType::MySql
    } else {
        DatabaseType::Postgres
    }
}

/// Initialize database connection
pub async fn init() -> anyhow::Result<()> {
    let database_url = env::var("DATABASE_URL")
        .unwrap_or_else(|_| "postgresql://user:password@localhost/fingerprint_db".to_string());

    let db_type = detect_database_type(&database_url);
    
    match db_type {
        DatabaseType::Postgres => {
            let pool = PgPool::connect(&database_url).await
                .context("Failed to connect to PostgreSQL database")?;
            run_migrations_postgres(&pool).await?;
            if POOL.set(DatabasePool::Postgres(pool)).is_err() {
                anyhow::bail!("Database pool already initialized");
            }
        }
        DatabaseType::MySql => {
            let pool = MySqlPool::connect(&database_url).await
                .context("Failed to connect to MySQL database")?;
            run_migrations_mysql(&pool).await?;
            if POOL.set(DatabasePool::MySql(pool)).is_err() {
                anyhow::bail!("Database pool already initialized");
            }
        }
    }

    tracing::info!("Database initialized");
    Ok(())
}

/// Run migrations for PostgreSQL (public for testing)
pub async fn run_migrations_postgres(pool: &PgPool) -> anyhow::Result<()> {
    // Create migrations table if not exists
    sqlx::query(
        r#"
        CREATE TABLE IF NOT EXISTS migrations (
            id SERIAL PRIMARY KEY,
            name VARCHAR(255) NOT NULL UNIQUE,
            applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        "#,
    )
    .execute(pool)
    .await?;

    // Run migrations
    let migrations = get_postgres_migrations();
    for migration in migrations {
        // Check if migration already applied
        let exists: bool = sqlx::query_scalar(
            "SELECT EXISTS(SELECT 1 FROM migrations WHERE name = $1)"
        )
        .bind(migration.name)
        .fetch_one(pool)
        .await?;

        if !exists {
            tracing::info!("Applying migration: {}", migration.name);
            pool.execute(migration.sql).await?;
            
            sqlx::query("INSERT INTO migrations (name) VALUES ($1)")
                .bind(migration.name)
                .execute(pool)
                .await?;
        }
    }

    Ok(())
}

/// Run migrations for MySQL
async fn run_migrations_mysql(pool: &MySqlPool) -> anyhow::Result<()> {
    // Create migrations table if not exists
    sqlx::query(
        r#"
        CREATE TABLE IF NOT EXISTS migrations (
            id INT AUTO_INCREMENT PRIMARY KEY,
            name VARCHAR(255) NOT NULL UNIQUE,
            applied_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        "#,
    )
    .execute(pool)
    .await?;

    // Run migrations
    let migrations = get_mysql_migrations();
    for migration in migrations {
        // Check if migration already applied
        let exists: bool = sqlx::query_scalar(
            "SELECT EXISTS(SELECT 1 FROM migrations WHERE name = ?)"
        )
        .bind(migration.name)
        .fetch_one(pool)
        .await?;

        if !exists {
            tracing::info!("Applying migration: {}", migration.name);
            pool.execute(migration.sql).await?;
            
            sqlx::query("INSERT INTO migrations (name) VALUES (?)")
                .bind(migration.name)
                .execute(pool)
                .await?;
        }
    }

    Ok(())
}

struct Migration {
    name: &'static str,
    sql: &'static str,
}

fn get_postgres_migrations() -> Vec<Migration> {
    vec![
        Migration {
            name: "000_create_users",
            sql: include_str!("../migrations/postgresql/000_create_users.sql"),
        },
        Migration {
            name: "001_create_fingerprints",
            sql: include_str!("../migrations/postgresql/001_create_fingerprints.sql"),
        },
        Migration {
            name: "002_create_auth_logs",
            sql: include_str!("../migrations/postgresql/002_create_auth_logs.sql"),
        },
        Migration {
            name: "003_create_indexes",
            sql: include_str!("../migrations/postgresql/003_create_indexes.sql"),
        },
    ]
}

fn get_mysql_migrations() -> Vec<Migration> {
    vec![
        Migration {
            name: "000_create_users",
            sql: include_str!("../migrations/mysql/000_create_users.sql"),
        },
        Migration {
            name: "001_create_fingerprints",
            sql: include_str!("../migrations/mysql/001_create_fingerprints.sql"),
        },
        Migration {
            name: "002_create_auth_logs",
            sql: include_str!("../migrations/mysql/002_create_auth_logs.sql"),
        },
        Migration {
            name: "003_create_indexes",
            sql: include_str!("../migrations/mysql/003_create_indexes.sql"),
        },
    ]
}

fn get_pool() -> &'static DatabasePool {
    POOL.get().expect("Database not initialized")
}

/// Store template in database
pub async fn store_template(
    user_id: i32,
    template: &[u8], // Binary data (BYTEA)
    finger: &str,
) -> anyhow::Result<()> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            sqlx::query(
                r#"
                INSERT INTO fingerprints (user_id, template, finger)
                VALUES ($1, $2, $3)
                ON CONFLICT (user_id, finger) 
                DO UPDATE SET template = $2, created_at = CURRENT_TIMESTAMP
                "#,
            )
            .bind(user_id)
            .bind(template)
            .bind(finger)
            .execute(p)
            .await?;
        }
        DatabasePool::MySql(p) => {
            sqlx::query(
                r#"
                INSERT INTO fingerprints (user_id, template, finger)
                VALUES (?, ?, ?)
                ON DUPLICATE KEY UPDATE template = ?, created_at = CURRENT_TIMESTAMP
                "#,
            )
            .bind(user_id)
            .bind(template)
            .bind(finger)
            .bind(template)
            .execute(p)
            .await?;
        }
    }

    Ok(())
}

/// Load template from database
pub async fn load_template(user_id: i32, finger: Option<&str>) -> anyhow::Result<Option<TemplateRecord>> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            let row = if let Some(finger) = finger {
                sqlx::query(
                    r#"
                    SELECT f.user_id, f.template, f.finger, f.created_at,
                           u.name as user_name, u.email as user_email
                    FROM fingerprints f
                    JOIN users u ON f.user_id = u.id
                    WHERE f.user_id = $1 AND f.finger = $2
                    LIMIT 1
                    "#,
                )
                .bind(user_id)
                .bind(finger)
                .fetch_optional(p)
                .await?
            } else {
                sqlx::query(
                    r#"
                    SELECT f.user_id, f.template, f.finger, f.created_at,
                           u.name as user_name, u.email as user_email
                    FROM fingerprints f
                    JOIN users u ON f.user_id = u.id
                    WHERE f.user_id = $1
                    ORDER BY f.created_at DESC
                    LIMIT 1
                    "#,
                )
                .bind(user_id)
                .fetch_optional(p)
                .await?
            };

            if let Some(row) = row {
                Ok(Some(TemplateRecord {
                    user_id: row.get("user_id"),
                    template: row.get("template"),
                    finger: row.get("finger"),
                    created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                        .map(|dt| dt.to_rfc3339()),
                    user_name: row.get::<Option<String>, _>("user_name"),
                    user_email: row.get::<Option<String>, _>("user_email"),
                }))
            } else {
                Ok(None)
            }
        }
        DatabasePool::MySql(p) => {
            let row = if let Some(finger) = finger {
                sqlx::query(
                    r#"
                    SELECT f.user_id, f.template, f.finger, f.created_at,
                           u.name as user_name, u.email as user_email
                    FROM fingerprints f
                    JOIN users u ON f.user_id = u.id
                    WHERE f.user_id = ? AND f.finger = ?
                    LIMIT 1
                    "#,
                )
                .bind(user_id)
                .bind(finger)
                .fetch_optional(p)
                .await?
            } else {
                sqlx::query(
                    r#"
                    SELECT f.user_id, f.template, f.finger, f.created_at,
                           u.name as user_name, u.email as user_email
                    FROM fingerprints f
                    JOIN users u ON f.user_id = u.id
                    WHERE f.user_id = ?
                    ORDER BY f.created_at DESC
                    LIMIT 1
                    "#,
                )
                .bind(user_id)
                .fetch_optional(p)
                .await?
            };

            if let Some(row) = row {
                Ok(Some(TemplateRecord {
                    user_id: row.get("user_id"),
                    template: row.get("template"),
                    finger: row.get("finger"),
                    created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                        .map(|dt| dt.to_rfc3339()),
                    user_name: row.get::<Option<String>, _>("user_name"),
                    user_email: row.get::<Option<String>, _>("user_email"),
                }))
            } else {
                Ok(None)
            }
        }
    }
}

/// Load all templates (optionally filtered by scope)
pub async fn load_templates(_scope: Option<&str>) -> anyhow::Result<Vec<TemplateRecord>> {
    let pool = get_pool();

    // For now, scope is ignored (can be implemented with user metadata table)
    match pool {
        DatabasePool::Postgres(p) => {
            let rows = sqlx::query(
                r#"
                SELECT f.user_id, f.template, f.finger, f.created_at,
                       u.name as user_name, u.email as user_email
                FROM fingerprints f
                JOIN users u ON f.user_id = u.id
                ORDER BY f.user_id, f.finger
                "#,
            )
            .fetch_all(p)
            .await?;

            let templates: Vec<TemplateRecord> = rows
                .into_iter()
                .map(|row| TemplateRecord {
                    user_id: row.get("user_id"),
                    template: row.get::<Vec<u8>, _>("template"), // BYTEA -> Vec<u8>
                    finger: row.get("finger"),
                    created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                        .map(|dt| dt.to_rfc3339()),
                    user_name: row.get::<Option<String>, _>("user_name"),
                    user_email: row.get::<Option<String>, _>("user_email"),
                })
                .collect();

            Ok(templates)
        }
        DatabasePool::MySql(p) => {
            let rows = sqlx::query(
                r#"
                SELECT f.user_id, f.template, f.finger, f.created_at,
                       u.name as user_name, u.email as user_email
                FROM fingerprints f
                JOIN users u ON f.user_id = u.id
                ORDER BY f.user_id, f.finger
                "#,
            )
            .fetch_all(p)
            .await?;

            let templates: Vec<TemplateRecord> = rows
                .into_iter()
                .map(|row| TemplateRecord {
                    user_id: row.get("user_id"),
                    template: row.get::<Vec<u8>, _>("template"), // BLOB -> Vec<u8>
                    finger: row.get("finger"),
                    created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                        .map(|dt| dt.to_rfc3339()),
                    user_name: row.get::<Option<String>, _>("user_name"),
                    user_email: row.get::<Option<String>, _>("user_email"),
                })
                .collect();

            Ok(templates)
        }
    }
}

/// Create user
pub async fn create_user(name: &str, email: Option<&str>) -> anyhow::Result<UserRecord> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            let row = sqlx::query(
                r#"
                INSERT INTO users (name, email)
                VALUES ($1, $2)
                RETURNING id, name, email, created_at, updated_at
                "#,
            )
            .bind(name)
            .bind(email)
            .fetch_one(p)
            .await?;

            Ok(UserRecord {
                id: row.get("id"),
                name: row.get("name"),
                email: row.get("email"),
                created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                    .map(|dt| dt.to_rfc3339()),
                updated_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("updated_at")
                    .map(|dt| dt.to_rfc3339()),
            })
        }
        DatabasePool::MySql(p) => {
            let row = sqlx::query(
                r#"
                INSERT INTO users (name, email)
                VALUES (?, ?)
                "#,
            )
            .bind(name)
            .bind(email)
            .execute(p)
            .await?;

            let id = row.last_insert_id() as i32;

            let row = sqlx::query(
                r#"
                SELECT id, name, email, created_at, updated_at
                FROM users
                WHERE id = ?
                "#,
            )
            .bind(id)
            .fetch_one(p)
            .await?;

            Ok(UserRecord {
                id: row.get("id"),
                name: row.get("name"),
                email: row.get("email"),
                created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                    .map(|dt| dt.to_rfc3339()),
                updated_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("updated_at")
                    .map(|dt| dt.to_rfc3339()),
            })
        }
    }
}

/// List all users with finger count
pub async fn list_users() -> anyhow::Result<Vec<(UserRecord, i32)>> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            let rows = sqlx::query(
                r#"
                SELECT 
                    u.id, u.name, u.email, u.created_at, u.updated_at,
                    CAST(COALESCE(COUNT(f.id), 0) AS INTEGER) as finger_count
                FROM users u
                LEFT JOIN fingerprints f ON u.id = f.user_id
                GROUP BY u.id, u.name, u.email, u.created_at, u.updated_at
                ORDER BY u.created_at DESC
                "#,
            )
            .fetch_all(p)
            .await?;

            Ok(rows
                .into_iter()
                .map(|row| {
                    (
                        UserRecord {
                            id: row.get("id"),
                            name: row.get("name"),
                            email: row.get("email"),
                            created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                                .map(|dt| dt.to_rfc3339()),
                            updated_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("updated_at")
                                .map(|dt| dt.to_rfc3339()),
                        },
                        row.get::<i32, _>("finger_count"),
                    )
                })
                .collect())
        }
        DatabasePool::MySql(p) => {
            let rows = sqlx::query(
                r#"
                SELECT 
                    u.id, u.name, u.email, u.created_at, u.updated_at,
                    COALESCE(COUNT(f.id), 0) as finger_count
                FROM users u
                LEFT JOIN fingerprints f ON u.id = f.user_id
                GROUP BY u.id, u.name, u.email, u.created_at, u.updated_at
                ORDER BY u.created_at DESC
                "#,
            )
            .fetch_all(p)
            .await?;

            Ok(rows
                .into_iter()
                .map(|row| {
                    (
                        UserRecord {
                            id: row.get("id"),
                            name: row.get("name"),
                            email: row.get("email"),
                            created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                                .map(|dt| dt.to_rfc3339()),
                            updated_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("updated_at")
                                .map(|dt| dt.to_rfc3339()),
                        },
                        row.get::<i32, _>("finger_count"),
                    )
                })
                .collect())
        }
    }
}

/// Get user by ID
pub async fn get_user(user_id: i32) -> anyhow::Result<Option<UserRecord>> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            let row = sqlx::query(
                r#"
                SELECT id, name, email, created_at, updated_at
                FROM users
                WHERE id = $1
                "#,
            )
            .bind(user_id)
            .fetch_optional(p)
            .await?;

            Ok(row.map(|row| UserRecord {
                id: row.get("id"),
                name: row.get("name"),
                email: row.get("email"),
                created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                    .map(|dt| dt.to_rfc3339()),
                updated_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("updated_at")
                    .map(|dt| dt.to_rfc3339()),
            }))
        }
        DatabasePool::MySql(p) => {
            let row = sqlx::query(
                r#"
                SELECT id, name, email, created_at, updated_at
                FROM users
                WHERE id = ?
                "#,
            )
            .bind(user_id)
            .fetch_optional(p)
            .await?;

            Ok(row.map(|row| UserRecord {
                id: row.get("id"),
                name: row.get("name"),
                email: row.get("email"),
                created_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("created_at")
                    .map(|dt| dt.to_rfc3339()),
                updated_at: row.get::<Option<chrono::DateTime<chrono::Utc>>, _>("updated_at")
                    .map(|dt| dt.to_rfc3339()),
            }))
        }
    }
}

/// Get user's fingers
pub async fn get_user_fingers(user_id: i32) -> anyhow::Result<Vec<String>> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            let rows = sqlx::query(
                r#"
                SELECT finger
                FROM fingerprints
                WHERE user_id = $1
                ORDER BY created_at
                "#,
            )
            .bind(user_id)
            .fetch_all(p)
            .await?;

            Ok(rows.into_iter().map(|row| row.get("finger")).collect())
        }
        DatabasePool::MySql(p) => {
            let rows = sqlx::query(
                r#"
                SELECT finger
                FROM fingerprints
                WHERE user_id = ?
                ORDER BY created_at
                "#,
            )
            .bind(user_id)
            .fetch_all(p)
            .await?;

            Ok(rows.into_iter().map(|row| row.get("finger")).collect())
        }
    }
}

/// Log authentication event
pub async fn log_auth(user_id: i32, success: bool, score: f32) -> anyhow::Result<()> {
    let pool = get_pool();

    match pool {
        DatabasePool::Postgres(p) => {
            sqlx::query(
                r#"
                INSERT INTO auth_logs (user_id, success, score)
                VALUES ($1, $2, $3)
                "#,
            )
            .bind(user_id)
            .bind(success)
            .bind(score)
            .execute(p)
            .await?;
        }
        DatabasePool::MySql(p) => {
            sqlx::query(
                r#"
                INSERT INTO auth_logs (user_id, success, score)
                VALUES (?, ?, ?)
                "#,
            )
            .bind(user_id)
            .bind(success)
            .bind(score)
            .execute(p)
            .await?;
        }
    }

    Ok(())
}
