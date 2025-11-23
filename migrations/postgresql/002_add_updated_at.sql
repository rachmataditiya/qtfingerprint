ALTER TABLE users ADD COLUMN updated_at TIMESTAMP;
-- separator
UPDATE users SET updated_at = CURRENT_TIMESTAMP WHERE updated_at IS NULL;

