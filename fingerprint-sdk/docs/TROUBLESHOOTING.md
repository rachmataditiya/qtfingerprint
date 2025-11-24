# Troubleshooting

Common issues and solutions when using the Fingerprint SDK.

## Initialization Issues

### Issue: "SDK not initialized"

**Symptoms:**
- `SDKException: SDK not initialized`

**Solutions:**
1. Ensure `FingerprintSDK.initialize()` is called before any operations
2. Check that initialization completed successfully
3. Verify USB device is connected

### Issue: "USB device not found"

**Symptoms:**
- `USBDeviceNotFoundException`
- Device not detected

**Solutions:**
1. Check USB cable connection
2. Verify device is DigitalPersona U.are.U 4000
3. Check USB permissions in AndroidManifest.xml
4. Restart app after connecting device

## Operation Issues

### Issue: Enrollment fails

**Symptoms:**
- `onEnrollError` called
- "Enrollment failed" message

**Solutions:**
1. Ensure device is ready: `sdk.isReady()`
2. Check USB connection
3. Verify backend API is accessible
4. Check network connectivity
5. Review logs for detailed error

### Issue: Verification always fails

**Symptoms:**
- Low scores or no match
- "Verification failed" even with correct finger

**Solutions:**
1. Ensure same finger is used for enrollment and verification
2. Check match threshold in config (default: 60)
3. Try re-enrolling the fingerprint
4. Clean fingerprint reader surface
5. Ensure finger is placed correctly

### Issue: Identification returns no match

**Symptoms:**
- `onIdentifyError("No matching user found")`
- No user identified

**Solutions:**
1. Verify users have enrolled fingerprints
2. Check database connectivity
3. Ensure templates are loaded correctly
4. Try verification first to test device
5. Check match threshold

## Performance Issues

### Issue: Operations are slow

**Solutions:**
1. Check network latency to backend
2. Ensure device is properly initialized
3. Reduce enrollment scans if needed (config)
4. Use background threads (SDK handles this automatically)

### Issue: Timeout errors

**Solutions:**
1. Increase timeout in config:
   ```kotlin
   val config = FingerprintSDK.Config(timeoutSeconds = 60)
   ```
2. Check device responsiveness
3. Verify USB connection stability

## Integration Issues

### Issue: Native library not found

**Symptoms:**
- `UnsatisfiedLinkError`
- Library loading fails

**Solutions:**
1. Verify `.so` files are in `jniLibs/arm64-v8a/`
2. Check `build.gradle` includes correct ABI:
   ```gradle
   android {
       defaultConfig {
           ndk {
               abiFilters 'arm64-v8a'
           }
       }
   }
   ```
3. Clean and rebuild project

### Issue: Build errors

**Solutions:**
1. Ensure all dependencies are built
2. Check NDK version compatibility
3. Verify Rust toolchain is installed
4. Check CMake configuration

## Debugging

### Enable Logging

```kotlin
val config = FingerprintSDK.Config(enableLogging = true)
val sdk = FingerprintSDK.initialize(this, config)
```

### Check Logs

```bash
adb logcat | grep -E "FingerprintSDK|fingerprint"
```

### Status Monitoring

```kotlin
when (sdk.getStatus()) {
    FingerprintSDK.Status.READY -> { /* Ready */ }
    FingerprintSDK.Status.ERROR -> { /* Error */ }
    // ...
}
```

## Common Error Messages

| Error | Cause | Solution |
|-------|-------|----------|
| `USB_PERMISSION_DENIED` | USB permission not granted | Grant USB permission |
| `USB_DEVICE_NOT_FOUND` | Device not connected | Connect device |
| `DEVICE_INITIALIZATION_FAILED` | Device init error | Check USB connection, restart app |
| `TEMPLATE_NOT_FOUND` | Template missing | Enroll fingerprint first |
| `NETWORK_ERROR` | Backend unreachable | Check network, verify backend URL |
| `TIMEOUT` | Operation timed out | Increase timeout, check device |

## Getting Help

1. Check logs with `enableLogging = true`
2. Verify device connection
3. Test with simple example first
4. Review [EXAMPLES.md](EXAMPLES.md) for usage patterns

