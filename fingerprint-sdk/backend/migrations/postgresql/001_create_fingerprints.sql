-- Create fingerprints table
-- Use BYTEA for template (binary data) like middleware POC
-- One user can have multiple fingers (one-to-many relationship)
CREATE TABLE IF NOT EXISTS fingerprints (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,
    template BYTEA NOT NULL,
    finger VARCHAR(50) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(user_id, finger)
);

CREATE INDEX IF NOT EXISTS idx_fingerprints_user_id ON fingerprints(user_id);

