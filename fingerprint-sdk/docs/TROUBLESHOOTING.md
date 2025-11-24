# Troubleshooting

Common issues and solutions when using Arkana Fingerprint SDK.

## Initialization Issues

### SDK Not Initialized

**Error:** `IllegalStateException: SDK not initialized`

**Solution:**
- Ensure `FingerprintSdk.init()` is called before any operations
- Call in `Application.onCreate()` or `Activity.onCreate()`

### Device Not Found

**Error:** `FingerError.DEVICE_NOT_FOUND`

**Solutions:**
1. Check USB device is connected
2. Verify device is DigitalPersona U.are.U 4000
3. Check USB permissions in AndroidManifest.xml
4. Restart app after connecting device

## USB Issues

### USB Permission Denied

**Error:** `FingerError.USB_PERMISSION_DENIED`

**Solutions:**
1. Ensure USB permission is requested
2. Check `device_filter.xml` is correct
3. Grant permission when prompted

### USB Device Not Found

**Error:** `FingerError.USB_DEVICE_NOT_FOUND`

**Solutions:**
1. Reconnect USB device
2. Check USB cable
3. Try different USB port
4. Restart device

## Capture Issues

### Capture Timeout

**Error:** `FingerError.CAPTURE_TIMEOUT`

**Solutions:**
1. Increase timeout in config:
   ```kotlin
   val config = FingerprintSdkConfig(
       timeoutMs = 60000 // 60 seconds
   )
   ```
2. Ensure finger is placed correctly
3. Clean fingerprint reader surface

### Capture Failed

**Error:** `FingerError.CAPTURE_FAILED`

**Solutions:**
1. Check device is properly initialized
2. Ensure device is not busy with another operation
3. Try reconnecting device

## Matching Issues

### Verification Failed

**Error:** `FingerError.VERIFICATION_FAILED`

**Solutions:**
1. Ensure same finger is used for enrollment and verification
2. Lower match threshold:
   ```kotlin
   val config = FingerprintSdkConfig(
       matchThreshold = 0.5f // Lower threshold
   )
   ```
3. Try re-enrolling fingerprint
4. Clean fingerprint reader

### Identification Failed

**Error:** `FingerError.IDENTIFICATION_FAILED`

**Solutions:**
1. Ensure users have enrolled fingerprints
2. Check backend API is accessible
3. Verify templates are loaded correctly
4. Lower match threshold

### No Templates

**Error:** `FingerError.NO_TEMPLATES`

**Solutions:**
1. Ensure users have enrolled fingerprints
2. Check backend API returns templates
3. Verify scope parameter (if used)

## Backend Issues

### Network Error

**Error:** `FingerError.NETWORK_ERROR`

**Solutions:**
1. Check network connectivity
2. Verify backend URL is correct
3. Check backend API is running
4. Check firewall/security settings

### Backend Error

**Error:** `FingerError.BACKEND_ERROR`

**Solutions:**
1. Check backend API logs
2. Verify API endpoints are correct
3. Check database connectivity
4. Verify request format

### Template Not Found

**Error:** `FingerError.TEMPLATE_NOT_FOUND`

**Solutions:**
1. Ensure user has enrolled fingerprint
2. Check backend database
3. Verify user ID is correct

## Build Issues

### Native Library Not Found

**Error:** `UnsatisfiedLinkError`

**Solutions:**
1. Ensure AAR includes native libraries
2. Check `abiFilters` includes `arm64-v8a`
3. Clean and rebuild project

### Compilation Errors

**Solutions:**
1. Ensure minSdk = 28
2. Check all dependencies are resolved
3. Clean and rebuild
4. Invalidate caches in Android Studio

## Performance Issues

### Slow Operations

**Solutions:**
1. Check network latency to backend
2. Ensure device is properly initialized
3. Reduce enrollment scans if needed
4. Use appropriate scope for identification

### Timeout Errors

**Solutions:**
1. Increase timeout in config
2. Check device responsiveness
3. Verify USB connection stability

## Debugging

### Enable Logging

SDK uses Android Logcat. Filter logs:

```bash
adb logcat | grep -E "FingerprintSdk|ArkanaFprint|FingerprintCapture"
```

### Check Device Status

```kotlin
// Check if device is connected
val usbManager = getSystemService(Context.USB_SERVICE) as UsbManager
val deviceList = usbManager.deviceList
// Check for DigitalPersona device (VID: 0x05BA)
```

### Test Backend API

```bash
# Test store template
curl -X POST http://api.example.com/users/123/fingerprint \
  -H "Content-Type: application/json" \
  -d '{"template": "test", "finger": "LEFT_INDEX"}'

# Test load template
curl http://api.example.com/users/123/fingerprint
```

## Getting Help

1. Check logs with `adb logcat`
2. Verify device connection
3. Test with simple example first
4. Review [USAGE.md](USAGE.md) for usage patterns
5. Open issue in repository

