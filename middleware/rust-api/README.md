# Fingerprint REST API Middleware

Simple REST API middleware written in Rust using Axum and PostgreSQL for fingerprint management.

## Features

- ✅ User management (CRUD operations)
- ✅ Fingerprint enrollment
- ✅ Fingerprint verification (1:1 match)
- ✅ Fingerprint identification (1:N match)
- ✅ PostgreSQL backend
- ✅ CORS enabled for mobile apps
- ✅ JSON API responses

## Prerequisites

- Rust 1.70+ (install from https://rustup.rs/)
- PostgreSQL 12+
- Cargo (comes with Rust)

## Setup

### 1. Create PostgreSQL Database

```bash
createdb fingerprint_db
```

Or via psql:

```sql
CREATE DATABASE fingerprint_db;
```

### 2. Configure Environment

Copy `.env.example` to `.env` and update database connection:

```bash
cp .env.example .env
```

Edit `.env`:

```
DATABASE_URL=postgresql://username:password@localhost:5432/fingerprint_db
SERVER_ADDR=0.0.0.0:3000
RUST_LOG=info
```

### 3. Build and Run

```bash
# Install dependencies
cargo build

# Run in development mode
cargo run

# Or run in release mode
cargo run --release
```

The server will start on `http://0.0.0.0:3000`

## API Endpoints

### Health Check

```
GET /health
```

Response:
```json
{
  "status": "ok",
  "service": "fingerprint-api"
}
```

### List Users

```
GET /users
```

Response:
```json
[
  {
    "id": 1,
    "name": "John Doe",
    "email": "john@example.com",
    "has_fingerprint": true,
    "created_at": "2024-01-01T00:00:00Z",
    "updated_at": "2024-01-01T00:00:00Z"
  }
]
```

### Get User

```
GET /users/:id
```

### Create User

```
POST /users
Content-Type: application/json

{
  "name": "John Doe",
  "email": "john@example.com"
}
```

### Delete User

```
DELETE /users/:id
```

### Enroll Fingerprint

```
POST /users/:id/fingerprint
Content-Type: application/json

{
  "template": "base64_encoded_template_data"
}
```

### Get Fingerprint Template

```
GET /users/:id/fingerprint
```

Response:
```json
{
  "template": "base64_encoded_template_data"
}
```

### Verify Fingerprint (1:1)

```
POST /users/:id/verify
Content-Type: application/json

{
  "template": "base64_encoded_template_data"
}
```

Response:
```json
{
  "matched": true,
  "score": 95
}
```

### Identify User (1:N)

```
POST /identify
Content-Type: application/json

{
  "template": "base64_encoded_template_data"
}
```

Response:
```json
{
  "user_id": 1,
  "score": 95
}
```

Or if no match:
```json
{
  "user_id": null,
  "score": null
}
```

## Database Schema

The API automatically creates the following table:

```sql
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    name VARCHAR(255) NOT NULL,
    email VARCHAR(255) UNIQUE,
    fingerprint_template BYTEA,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

## Development

### Run with hot reload

```bash
cargo watch -x run
```

### Run tests

```bash
cargo test
```

### Check code

```bash
cargo clippy
cargo fmt
```

## Production Deployment

1. Build release binary:
   ```bash
   cargo build --release
   ```

2. Run the binary:
   ```bash
   ./target/release/fingerprint-api
   ```

3. Use a process manager like systemd or supervisor to keep it running.

## Notes

- Template matching in this PoC uses simple byte comparison. In production, implement proper fingerprint matching algorithms.
- The API uses CORS to allow requests from mobile apps. Adjust CORS settings in production.
- Database migrations run automatically on startup.

