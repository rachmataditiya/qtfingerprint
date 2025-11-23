# Android Fingerprint PoC Application

Proof of Concept Android tablet application similar to the Qt desktop application, using REST API for database operations.

## Features

- ✅ Reader initialization
- ✅ Fingerprint enrollment (5 stages)
- ✅ Fingerprint verification (1:1 match)
- ✅ Fingerprint identification (1:N match)
- ✅ User list management
- ✅ REST API integration with PostgreSQL backend
- ✅ Modern Material Design UI

## Architecture

```
┌─────────────────────────────────────┐
│   Android PoC App                   │
│   ├── MainActivity                  │
│   ├── FingerprintService (local)    │
│   └── ApiClient (REST API)          │
├─────────────────────────────────────┤
│   REST API Middleware (Rust)        │
│   └── PostgreSQL Database           │
└─────────────────────────────────────┘
```

## Prerequisites

- Android Studio (latest)
- Android SDK (API 29+)
- Fingerprint hardware or emulator with biometric support
- REST API server running (see middleware/rust-api)

## Setup

### 1. Configure API Endpoint

Edit `app/src/main/res/values/config.xml`:

```xml
<string name="api_base_url">http://YOUR_SERVER_IP:3000</string>
```

Or use build configuration for different environments.

### 2. Build and Run

```bash
./gradlew assembleDebug
adb install app/build/outputs/apk/debug/app-debug.apk
```

## API Integration

The app uses Retrofit for REST API calls. All API calls are asynchronous and handled with Kotlin coroutines.

## Usage

1. **Initialize Reader**: Tap "Initialize Reader" to check fingerprint hardware availability
2. **Enroll User**: Enter name/email, tap "Start Enrollment", then scan finger 5 times
3. **Verify**: Select user from list, tap "Start Verification", scan finger
4. **Identify**: Tap "Identify User", scan finger to match against all enrolled users
5. **Delete User**: Select user and tap "Delete User"

## Notes

- Fingerprint templates are stored in Android KeyStore (secure hardware)
- Template identifiers (key aliases) are synced to PostgreSQL via REST API
- The app requires internet connection for database operations

