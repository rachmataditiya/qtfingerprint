# Install APK ke Tablet Android

## IP Host Server
- **Server IP**: `192.168.1.2:3000`
- **Rust API**: http://192.168.1.2:3000

## Cara Install ke Tablet

### Option 1: Via ADB (USB)
```bash
# Connect tablet via USB, enable USB debugging
adb devices  # Verify tablet detected

# Install APK
cd android-poc
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Launch app
adb shell am start -n com.arkana.fingerprintpoc/.MainActivity
```

### Option 2: Via File Transfer
1. Copy `app/build/outputs/apk/debug/app-debug.apk` ke tablet
2. Install APK dari file manager tablet
3. Allow installation from unknown sources if needed

### Option 3: Via ADB Wireless
```bash
# Enable wireless debugging on tablet
# Connect via WiFi ADB
adb connect TABLET_IP:5555

# Then install as usual
adb install -r app/build/outputs/apk/debug/app-debug.apk
```

## Verifikasi Koneksi

### 1. Pastikan Server Running
Server Rust harus running di host (192.168.1.2:3000):
```bash
curl http://192.168.1.2:3000/health
```

### 2. Pastikan Tablet & Host di Network yang Sama
- Tablet dan komputer host harus di WiFi/network yang sama
- Test koneksi dari browser tablet: `http://192.168.1.2:3000/health`

### 3. Firewall Settings
Pastikan firewall di host mengizinkan koneksi port 3000:
```bash
# macOS
sudo pfctl -d  # Disable firewall sementara untuk testing
# atau tambahkan rule untuk allow port 3000
```

## Testing

Setelah install:
1. Buka app di tablet
2. Tap "Initialize Reader"
3. Tap "Refresh" di User List
4. Jika berhasil, akan muncul list users dari database

## Troubleshooting

**"Failed to connect to server"**
- Pastikan tablet & host di network yang sama
- Cek firewall di host
- Test di browser tablet: `http://192.168.1.2:3000/health`

**"No users found"**
- Server mungkin belum running
- Check log server: `tail -f /tmp/rust_api.log`

**App crash**
- Check logcat: `adb logcat | grep fingerprintpoc`
- Pastikan API_BASE_URL sudah benar di build.gradle.kts

