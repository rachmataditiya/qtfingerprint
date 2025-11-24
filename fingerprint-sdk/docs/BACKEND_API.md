# Backend API Specification

Spesifikasi API backend yang diperlukan oleh Arkana Fingerprint SDK.

## Overview

SDK membutuhkan backend API untuk:
- Menyimpan fingerprint templates
- Mengambil templates untuk verification/identification
- Logging authentication events

## Base URL

```
http://your-backend-api.com
```

## Authentication

API menggunakan standard HTTP authentication (optional, tergantung implementasi).

## Endpoints

### 1. Store Template

**POST** `/users/{userId}/fingerprint`

Menyimpan fingerprint template untuk user.

**Path Parameters:**
- `userId` (integer): User ID

**Request Body:**
```json
{
  "template": "base64_encoded_template_string",
  "finger": "LEFT_INDEX"
}
```

**Response:**
- `200 OK`: Template stored successfully
- `400 Bad Request`: Invalid request
- `500 Internal Server Error`: Server error

**Example:**
```bash
curl -X POST http://api.example.com/users/123/fingerprint \
  -H "Content-Type: application/json" \
  -d '{
    "template": "FP3_BASE64_ENCODED_STRING",
    "finger": "LEFT_INDEX"
  }'
```

### 2. Load Template

**GET** `/users/{userId}/fingerprint`

Mengambil fingerprint template untuk user.

**Path Parameters:**
- `userId` (integer): User ID

**Response:**
```json
{
  "template": "base64_encoded_template_string",
  "finger": "LEFT_INDEX",
  "created_at": "2024-01-01T00:00:00Z"
}
```

**Status Codes:**
- `200 OK`: Template found
- `404 Not Found`: Template not found
- `500 Internal Server Error`: Server error

**Example:**
```bash
curl http://api.example.com/users/123/fingerprint
```

### 3. Load All Templates

**GET** `/templates?scope={scope}`

Mengambil semua templates (untuk identification).

**Query Parameters:**
- `scope` (string, optional): Filter by scope (e.g., branch ID)

**Response:**
```json
[
  {
    "user_id": 123,
    "template": "base64_encoded_template_string",
    "finger": "LEFT_INDEX"
  },
  {
    "user_id": 456,
    "template": "base64_encoded_template_string",
    "finger": "RIGHT_INDEX"
  }
]
```

**Status Codes:**
- `200 OK`: Templates retrieved
- `500 Internal Server Error`: Server error

**Example:**
```bash
# Get all templates
curl http://api.example.com/templates

# Get templates for specific scope
curl http://api.example.com/templates?scope=branch_001
```

### 4. Log Authentication

**POST** `/log_auth`

Log authentication events (verification/identification).

**Request Body:**
```json
{
  "user_id": 123,
  "success": true,
  "score": 0.85
}
```

**Response:**
- `200 OK`: Logged successfully
- `400 Bad Request`: Invalid request

**Example:**
```bash
curl -X POST http://api.example.com/log_auth \
  -H "Content-Type: application/json" \
  -d '{
    "user_id": 123,
    "success": true,
    "score": 0.85
  }'
```

## Data Format

### Template Format

- **Format**: FP3 (libfprint format)
- **Encoding**: Base64
- **Size**: Variable (typically 1-10 KB)

### Finger Type

```json
{
  "UNKNOWN": "Unknown finger",
  "LEFT_THUMB": "Left thumb",
  "LEFT_INDEX": "Left index finger",
  "LEFT_MIDDLE": "Left middle finger",
  "LEFT_RING": "Left ring finger",
  "LEFT_PINKY": "Left pinky",
  "RIGHT_THUMB": "Right thumb",
  "RIGHT_INDEX": "Right index finger",
  "RIGHT_MIDDLE": "Right middle finger",
  "RIGHT_RING": "Right ring finger",
  "RIGHT_PINKY": "Right pinky"
}
```

## Error Responses

All endpoints may return error responses:

```json
{
  "error": "Error message",
  "code": "ERROR_CODE"
}
```

## Implementation Notes

1. **Template Storage**: Store templates securely (encrypted at rest)
2. **Performance**: `/templates` endpoint should be optimized for large datasets
3. **Caching**: Consider caching templates for frequently accessed users
4. **Logging**: Authentication logs should be stored for audit purposes

## Example Backend Implementation

Lihat folder `backend/` untuk contoh implementasi Rust Axum.

