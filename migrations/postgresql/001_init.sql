CREATE TABLE IF NOT EXISTS users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255),
    fingerprint_template BYTEA,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

