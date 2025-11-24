# Fingerprint Backend API

Backend API untuk Arkana Fingerprint SDK menggunakan Rust Axum.

## Overview

Backend API menyediakan endpoints untuk:
- Menyimpan fingerprint templates
- Mengambil templates untuk verification/identification
- Logging authentication events

## Features

- **Rust Axum**: Fast, async web framework
- **PostgreSQL/MySQL Support**: Pilih database sesuai kebutuhan
- **Migration System**: Automatic database migrations
- **Base64 Encoding**: Template encoding/decoding
- **CORS Support**: Cross-origin requests
- **Error Handling**: Comprehensive error responses
- **Unit Tests**: Complete test coverage

## Prerequisites

- Rust 1.70+
- PostgreSQL 12+ atau MySQL 8.0+
- Cargo

## Installation

1. **Install dependencies**:
   ```bash
   cargo build
   ```

2. **Setup database**:
   
   **Option 1: Automatic script (Linux/macOS):**
   ```bash
   ./setup_db.sh
   ```
   
   **Option 2: Automatic script (Windows PowerShell):**
   ```powershell
   .\setup_db.ps1
   ```
   
   **Option 3: Manual SQL script:**
   ```bash
   # Run SQL script
   psql -U <your_admin_user> -f setup_db.sql
   
   # Or run commands directly
   psql -U <your_admin_user> -c "CREATE USER finger WITH PASSWORD 'finger';"
   psql -U <your_admin_user> -c "CREATE DATABASE fingerprint_db OWNER finger;"
   ```
   
   Script akan:
   - Membuat user `finger` dengan password `finger`
   - Membuat database `fingerprint_db` jika belum ada
   - Grant privileges yang diperlukan
   - Test koneksi
   
   **Note:** Jika script gagal, gunakan manual setup dengan user admin PostgreSQL Anda.

3. **Configure environment**:
   ```bash
   cp .env.example .env
   # .env sudah dikonfigurasi dengan default: postgresql://finger:finger@localhost:5432/fingerprint_db
   ```

4. **Run migrations** (automatic on first run)

## Configuration

Create `.env` file:

**PostgreSQL:**
```env
DATABASE_URL=postgresql://user:password@localhost/fingerprint_db
RUST_LOG=info
```

**MySQL:**
```env
DATABASE_URL=mysql://user:password@localhost/fingerprint_db
RUST_LOG=info
```

## Running

```bash
cargo run
```

Server akan berjalan di `http://0.0.0.0:3000`

## Database Migrations

Migrations dijalankan otomatis saat aplikasi start. File migration ada di:
- `migrations/postgresql/` - Untuk PostgreSQL
- `migrations/mysql/` - Untuk MySQL

### Manual Migration

Jika perlu menjalankan migration manual:

```bash
# Set DATABASE_URL
export DATABASE_URL=postgresql://user:password@localhost/fingerprint_db

# Run migrations (akan dijalankan otomatis saat init)
cargo run
```

## API Endpoints

### POST /users/{userId}/fingerprint

Store fingerprint template.

**Request:**
```json
{
  "template": "base64_encoded_template",
  "finger": "LEFT_INDEX"
}
```

### GET /users/{userId}/fingerprint

Load fingerprint template.

**Response:**
```json
{
  "template": "base64_encoded_template",
  "finger": "LEFT_INDEX",
  "created_at": "2024-01-01T00:00:00Z"
}
```

### GET /templates?scope={scope}

Load all templates (for identification).

**Response:**
```json
[
  {
    "user_id": 123,
    "user_name": "John Doe",
    "user_email": "john@example.com",
    "template": "base64_encoded_template",
    "finger": "LEFT_INDEX"
  }
]
```

**Note:** Response includes all fingers for all users. If a user has multiple fingers enrolled, each finger will be a separate entry in the array.

### POST /log_auth

Log authentication event.

**Request:**
```json
{
  "user_id": 123,
  "success": true,
  "score": 0.85
}
```

## Testing

### Setup Test Database

```bash
# PostgreSQL
createdb test_fingerprint_db

# MySQL
mysql -u root -p -e "CREATE DATABASE test_fingerprint_db;"
```

### Run Tests

```bash
# Set test database URL
export TEST_DATABASE_URL=postgresql://postgres:postgres@localhost/test_fingerprint_db

# Run tests
cargo test
```

### Test Coverage

Tests mencakup:
- ✅ Store template
- ✅ Load template
- ✅ Load template (not found)
- ✅ Load all templates
- ✅ Log authentication
- ✅ Invalid request handling

## Database Schema

### users

**PostgreSQL:**
```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP
);
```

**MySQL:**
```sql
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
);
```

### fingerprints

**PostgreSQL:**
```sql
CREATE TABLE fingerprints (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    template BYTEA NOT NULL,
    finger VARCHAR(50) NOT NULL,
    created_at TIMESTAMPTZ DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(user_id, finger),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

**MySQL:**
```sql
CREATE TABLE fingerprints (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    template BLOB NOT NULL,
    finger VARCHAR(50) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE KEY unique_user_finger (user_id, finger),
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);
```

**Note:** 
- `template` is stored as binary data (BYTEA in PostgreSQL, BLOB in MySQL)
- One user can have multiple fingers (one-to-many relationship)
- Each finger is identified by the `finger` field (e.g., "LEFT_INDEX", "RIGHT_THUMB")

### auth_logs

**PostgreSQL:**
```sql
CREATE TABLE auth_logs (
    id SERIAL PRIMARY KEY,
    user_id INTEGER NOT NULL,
    success BOOLEAN NOT NULL,
    score FLOAT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

**MySQL:**
```sql
CREATE TABLE auth_logs (
    id INT AUTO_INCREMENT PRIMARY KEY,
    user_id INT NOT NULL,
    success BOOLEAN NOT NULL,
    score FLOAT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## Development

```bash
# Run with auto-reload
cargo watch -x run

# Run tests
cargo test

# Format code
cargo fmt

# Lint
cargo clippy
```

## Production Deployment

1. Build release:
   ```bash
   cargo build --release
   ```

2. Run:
   ```bash
   ./target/release/fingerprint-backend
   ```

3. Use process manager (systemd, supervisor, etc.)

## See Also

- [API Specification](../docs/BACKEND_API.md)
- [SDK Integration](../docs/INTEGRATION.md)
