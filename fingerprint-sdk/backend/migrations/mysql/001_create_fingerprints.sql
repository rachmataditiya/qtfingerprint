-- Create fingerprints table
-- Use BLOB for template (binary data) like middleware POC
-- One user can have multiple fingers (one-to-many relationship)
CREATE TABLE IF NOT EXISTS fingerprints (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    template BLOB NOT NULL,
    finger VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_user_finger (user_id, finger),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE INDEX idx_fingerprints_user_id ON fingerprints(user_id);

