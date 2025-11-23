# Quick Start - Lihat UI di Emulator

## Status: ✅ APK Sudah Berhasil Dibuild!

**Lokasi APK:** `android-poc/app/build/outputs/apk/debug/app-debug.apk`

## Langkah Cepat

### 1. Start Emulator
```bash
# Via Android Studio: Tools > Device Manager > Play button
# Atau via command line:
emulator -list-avds  # lihat daftar emulator
emulator -avd NAMA_EMULATOR  # jalankan emulator
```

### 2. Install & Run
```bash
cd android-poc

# Install APK
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Launch app
adb shell am start -n com.arkana.fingerprintpoc/.MainActivity
```

**ATAU** buka project di Android Studio dan klik Run button!

## UI yang Akan Terlihat

### Main Screen Layout:
```
┌─────────────────────────────────────┐
│  Status: Ready - Tap 'Initialize...'│  ← Status Bar
├─────────────────────────────────────┤
│  [1. Reader Initialization]         │
│  [Initialize Reader Button]         │
├─────────────────────────────────────┤
│  [2. Enrollment]                    │
│  Name: [_____________]              │
│  Email: [____________]              │
│  [Start Enrollment Button]          │
│  [Progress Bar]                     │
│  Status: ...                        │
├─────────────────────────────────────┤
│  [3. Verification & Identification] │
│  Selected: No user selected         │
│  [Start Verification Button]        │
│  [Identify User Button]             │
│  Result: ...                        │
├─────────────────────────────────────┤
│  [4. User List]        [Refresh]    │
│  Total users: 0                     │
│  [Empty List - scrollable]          │
│  [Delete Selected User Button]      │
└─────────────────────────────────────┘
```

### Features UI:
- ✅ Material Design Cards
- ✅ Text Input Fields dengan Material Design
- ✅ Progress Bar untuk enrollment
- ✅ Status Labels yang update
- ✅ RecyclerView untuk user list
- ✅ Responsive ScrollView
- ✅ Modern buttons dengan proper states

## Test UI Interactivity

1. **Tap "Initialize Reader"** → Status akan update
2. **Enter name di enrollment form** → Input field berfungsi
3. **Tap "Start Enrollment"** → Button akan disable, progress bar muncul
4. **Tap "Refresh"** → User list akan refresh (walaupun kosong)
5. **Scroll** → Semua sections bisa di-scroll

## Troubleshooting

**"adb: no devices/emulators found"**
→ Start emulator dulu via Android Studio atau command line

**App crash saat open**
→ Check logcat: `adb logcat | grep -i error`

**UI tidak muncul dengan benar**
→ Pastikan emulator menggunakan API 29+ (Android 10+)

## Next Steps

Setelah melihat UI, untuk full functionality:
1. Integrate `libdigitalpersona` module
2. Start REST API server (`middleware/rust-api`)
3. Setup PostgreSQL database

Tetapi untuk melihat UI saja, APK sudah siap dijalankan! 🎉

