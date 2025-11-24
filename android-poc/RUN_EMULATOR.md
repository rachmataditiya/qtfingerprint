# Menjalankan Android PoC di Emulator

## Build Berhasil! ✅

APK sudah berhasil di-build di: `app/build/outputs/apk/debug/app-debug.apk`

## Cara Menjalankan di Emulator

### 1. Start Android Emulator

**Option A: Via Android Studio**
1. Buka Android Studio
2. Klik **Tools > Device Manager**
3. Klik **Play** button pada emulator yang ingin digunakan
4. Tunggu emulator sampai fully loaded

**Option B: Via Command Line**
```bash
# List available emulators
emulator -list-avds

# Start emulator (replace dengan nama AVD Anda)
emulator -avd Pixel_5_API_33
```

### 2. Install & Run App

Setelah emulator running, jalankan script ini:

```bash
cd android-poc
./run_emulator.sh
```

Atau manual:
```bash
# Install APK
adb install -r app/build/outputs/apk/debug/app-debug.apk

# Launch app
adb shell am start -n com.arkana.fingerprintpoc/.MainActivity
```

### 3. Atau Build & Run dari Android Studio

1. Buka project `android-poc` di Android Studio
2. Pilih emulator di device dropdown
3. Klik **Run** button (Shift+F10)

## UI Features yang Bisa Dilihat

✅ **4 Main Sections:**
1. Reader Initialization - Button untuk initialize fingerprint reader
2. Enrollment - Form untuk input name/email + enrollment button
3. Verification & Identification - Buttons untuk verify/identify
4. User List - RecyclerView untuk menampilkan daftar users

✅ **UI Components:**
- Material Design cards untuk setiap section
- Progress bar untuk enrollment
- Status labels yang update
- ScrollView untuk responsive layout
- Modern Material buttons

## Note

- Untuk demo UI saja, fingerprint functionality akan menunjukkan error karena libdigitalpersona belum di-integrate
- Semua UI components sudah fully functional dan bisa di-click
- REST API calls akan fail karena server belum running (tapi UI tetap bisa dilihat)

## Screenshot UI

Setelah app running, Anda akan melihat:
- Status bar di atas
- 4 card sections dengan Material Design
- Input fields untuk enrollment
- Buttons untuk semua actions
- User list area (akan kosong karena belum ada data)

Silakan jalankan emulator dan install app untuk melihat UI-nya! 🎨

## USB Device Passthrough

⚠️ **Important:** Android emulator tidak support USB passthrough langsung untuk fingerprint reader.

Untuk testing dengan fingerprint hardware:
- ✅ **Recommended:** Gunakan physical Android tablet dengan USB OTG atau built-in fingerprint sensor
- 📖 **Details:** Lihat `USB_PASSTHROUGH.md` untuk opsi lengkap
- 🔍 **Check Device:** Jalankan `./check_usb_device.sh` untuk melihat USB devices yang terdeteksi

