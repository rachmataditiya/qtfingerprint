-- Create indexes for better performance
CREATE INDEX idx_fingerprints_user_id ON fingerprints(user_id);
CREATE INDEX idx_auth_logs_user_id ON auth_logs(user_id);
CREATE INDEX idx_auth_logs_created_at ON auth_logs(created_at);

