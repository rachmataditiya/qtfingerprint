# Android Migration Plan - DigitalPersona Library

## Overview

Migrating `digitalpersonalib` from Linux (libfprint) to Android tablet requires a different approach because Android's fingerprint API works differently.

## Key Differences

| Aspect | Desktop (libfprint) | Android |
|--------|---------------------|---------|
| **Template Storage** | Raw template bytes (BLOB in DB) | Android KeyStore (secure hardware) |
| **Template Access** | Can extract and compare templates | Templates stored securely, cannot extract |
| **Verification** | Compare template bytes | Authenticate against KeyStore key |
| **Identification (1:N)** | Loop compare against gallery | Authenticate against multiple keys with different aliases |
| **API** | C library (libfprint-2) | Java/Kotlin (BiometricPrompt) |

## Implementation Strategy

### Option 1: Android KeyStore Approach (Recommended)

**Pros:**
- Uses Android's secure hardware
- Templates never leave secure hardware
- Most secure approach

**Cons:**
- Cannot extract raw templates for cross-platform use
- Identification requires multiple authentication attempts (slower for 1:N)

**Implementation:**
1. **Enrollment:** Create Android KeyStore key, require biometric auth to use
2. **Verification (1:1):** Authenticate against specific user's key
3. **Identification (1:N):** Loop through keys (each user has unique key alias)

### Option 2: Hybrid Approach (Store template metadata + use KeyStore)

**Pros:**
- Can store template metadata/user info in database
- Uses Android secure hardware for authentication
- Faster identification (can filter users first)

**Cons:**
- Still need multiple auth attempts for 1:N

### Option 3: Custom Template Storage (If we can extract)

**Note:** This is NOT possible with standard Android API. Templates are stored in secure hardware and cannot be extracted.

## Architecture Plan

```
┌─────────────────────────────────────┐
│   Qt Android App                    │
│   (C++ FingerprintManager API)      │
├─────────────────────────────────────┤
│   JNI Bridge (C++ ↔ Java)           │
├─────────────────────────────────────┤
│   FingerprintService.kt             │
│   - Uses Android BiometricPrompt    │
│   - Manages Android KeyStore        │
├─────────────────────────────────────┤
│   Android KeyStore                  │
│   (Secure Hardware)                 │
└─────────────────────────────────────┘
```

## Implementation Steps

### Phase 1: Android FingerprintService (Kotlin)
- [x] Create `FingerprintService.kt` with BiometricPrompt
- [ ] Implement enrollment using KeyStore
- [ ] Implement verification (1:1)
- [ ] Implement identification (1:N) using key aliases

### Phase 2: JNI Bridge (C++)
- [ ] Create JNI wrapper functions
- [ ] Map Android callbacks to C++ callbacks
- [ ] Implement same API signature as desktop `FingerprintManager`

### Phase 3: C++ Wrapper
- [ ] Create `fingerprint_manager_android.cpp`
- [ ] Implement same methods as desktop version
- [ ] Bridge JNI calls to maintain API compatibility

## Current Status

✅ Project structure created
✅ Basic FingerprintService.kt skeleton created
⏳ Need to decide on template storage strategy
⏳ Need to implement JNI bridge
⏳ Need to implement C++ wrapper

## Questions to Resolve

1. **Template Storage:** Do we need cross-platform template compatibility, or is Android-only OK?
2. **Identification Performance:** Is it acceptable to do multiple authentication attempts for 1:N?
3. **Database:** Keep SQLite or use Android Room/SQLite?

## Next Steps

1. Fix FingerprintService to use Android KeyStore properly
2. Create JNI bridge
3. Create C++ wrapper that matches desktop API
4. Test on Android tablet

