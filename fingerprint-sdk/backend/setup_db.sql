-- Manual database setup script
-- Run with: psql -U <your_admin_user> -f setup_db.sql
-- Or copy-paste commands to psql prompt

-- Create user
CREATE USER finger WITH PASSWORD 'finger';

-- Create database
CREATE DATABASE fingerprint_db OWNER finger;

-- Grant privileges
GRANT ALL PRIVILEGES ON DATABASE fingerprint_db TO finger;

-- Connect to database and grant schema privileges
\c fingerprint_db
GRANT ALL ON SCHEMA public TO finger;

-- Verify
\du finger
\l fingerprint_db

