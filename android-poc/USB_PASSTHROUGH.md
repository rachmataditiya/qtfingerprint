# USB Device Passthrough ke Android Emulator

## Status Device Terdeteksi

✅ **Fingerprint Reader Terdeteksi di Host (macOS):**
- **Vendor:** DigitalPersona, Inc. (0x05BA / 1466)
- **Product:** U.are.U® 4500 Fingerprint Reader (0x000A / 10)
- **Serial:** {DD8C5757-4679-B242-88C2-D6D4CAACB7AB}

## Metode USB Passthrough

### ⚠️ Catatan Penting
Android emulator modern (Android Studio emulator) **mendukung USB passthrough** melalui QEMU flags. Berikut adalah beberapa opsi:

## Opsi 1: Menggunakan Android Physical Device (Recommended ✅)

**Ini adalah cara paling mudah dan reliable:**

```bash
# 1. Enable USB Debugging di physical device
# Settings > Developer Options > USB Debugging

# 2. Connect device via USB
adb devices

# 3. Install app ke physical device
cd android-poc
./gradlew installDebug
```

**Keuntungan:**
- ✅ Full USB access (termasuk fingerprint reader)
- ✅ Real hardware testing
- ✅ No configuration needed
- ✅ Native Android Biometric API akan bekerja langsung

## Opsi 2: Android Emulator dengan QEMU USB Passthrough ✅

Berdasarkan [Stack Overflow discussion](https://stackoverflow.com/questions/53205809/connecting-usb-devices-to-android-emulator), kita bisa menggunakan QEMU USB passthrough langsung!

### Cara Otomatis (Recommended) ✅

Gunakan script yang sudah disediakan:
```bash
cd android-poc
./run_emulator_with_usb.sh
```

Script ini akan:
1. ✅ Auto-detect fingerprint reader
2. ✅ Extract USB bus/address (Linux) atau vendor/product ID (macOS)
3. ✅ Start emulator dengan USB passthrough flags

### Cara Manual

#### Untuk Linux:
```bash
# 1. List USB devices dan cari fingerprint reader
lsusb
# Output: Bus 001 Device 023: ID 05ba:000a DigitalPersona, Inc. U.are.U 4500 Fingerprint Reader

# 2. Extract bus dan address
BUS=1
ADDR=23

# 3. Start emulator dengan USB passthrough
emulator -avd Pixel_Tablet \
  -qemu -usb \
  -device usb-host,hostbus=$BUS,hostaddr=$ADDR
```

**Source:** [Stack Overflow Answer by Yingchun](https://stackoverflow.com/a/56203526)

#### Untuk macOS:
```bash
# Method 1: Menggunakan vendor/product ID (Recommended untuk macOS)
emulator -avd Pixel_Tablet \
  -qemu -usb \
  -device usb-host,vendorid=0x05ba,productid=0x000a

# Method 2: Mencoba dengan locationID (experimental)
# Get locationID dulu:
ioreg -p IOUSB -l -w 0 | grep -A 30 "U.are.U" | grep locationID

# Lalu gunakan:
emulator -avd Pixel_Tablet \
  -qemu -usb \
  -device usb-host,hostbus=1,hostaddr=<locationID>
```

### Catatan Penting

**Linux:**
- ✅ QEMU USB passthrough biasanya bekerja dengan baik
- ⚠️ Mungkin perlu run dengan `sudo` jika ada permission issues
- ✅ Android emulator modern sudah include USB host driver di kernel

**macOS:**
- ⚠️ Android emulator di macOS menggunakan Hypervisor.framework
- 💡 Coba method vendor/product ID terlebih dahulu
- ⚠️ Jika tidak bekerja, gunakan physical device (lebih reliable)

## Opsi 3: Network-based Solution (Workaround)

Karena Android PoC kita sudah menggunakan REST API, ada workaround:

1. **Fingerprint operations dilakukan di host** (Qt desktop app)
2. **Android tablet hanya sebagai UI** yang mengirim fingerprint data via REST API
3. **Backend (Rust middleware)** melakukan verification/identification

**Arsitektur:**
```
[Fingerprint Reader] -> [Qt Desktop App] -> [REST API] -> [PostgreSQL]
                                                              ↑
                                                              |
[Android Tablet] -> [REST API] <-------------------------------┘
```

## Opsi 4: VirtualBox dengan Android x86 (Alternative)

1. Install VirtualBox
2. Download Android x86 ISO image
3. Setup USB passthrough di VirtualBox:
   - Devices > USB > Select "U.are.U 4500 Fingerprint Reader"
4. Install app di Android x86 VM

**Limitation:** 
- Android x86 mungkin tidak support semua Android APIs
- Performance lebih lambat dibanding emulator native

## Rekomendasi untuk Development

### Untuk Development/Testing UI: ✅
Gunakan **Android Emulator** (tanpa USB passthrough):
- UI testing
- API integration testing
- Flow validation
- Mock fingerprint responses

### Untuk Testing Fingerprint Hardware: ✅
Gunakan **Physical Android Tablet** dengan:
- USB OTG connection ke fingerprint reader, ATAU
- Built-in fingerprint sensor (jika tablet punya)

### Untuk Production-like Testing: ✅
Gunakan **Hybrid Approach**:
- Desktop Qt app untuk enrollment (dengan fingerprint reader)
- Android tablet untuk identification/verification (via REST API)
- Shared PostgreSQL database

## Testing dengan Physical Device

### Setup USB OTG (jika tablet support)

1. **Check tablet support USB OTG:**
```bash
# Connect tablet via USB
adb shell getprop ro.boot.hardware.usb
```

2. **Connect fingerprint reader via USB OTG adapter**

3. **Install app:**
```bash
cd android-poc
./install_to_tablet.sh
```

### Setup dengan Built-in Fingerprint Sensor

Jika tablet punya built-in fingerprint sensor:
- Android Biometric API akan bekerja otomatis
- Tidak perlu USB OTG
- Langsung bisa test enrollment/verification

## Checklist

- [ ] Fingerprint reader terdeteksi di host (✅ DONE)
- [ ] Android emulator running untuk UI testing
- [ ] Physical tablet ready untuk hardware testing
- [ ] REST API running (middleware/rust-api)
- [ ] PostgreSQL database accessible
- [ ] Test enrollment flow
- [ ] Test verification flow
- [ ] Test identification flow

## Resources

- [Stack Overflow: Connecting USB Devices to Android Emulator](https://stackoverflow.com/questions/53205809/connecting-usb-devices-to-android-emulator)
- [Android USB Host Documentation](https://developer.android.com/guide/topics/connectivity/usb/host)
- [QEMU USB Passthrough (Linux)](https://www.linux-kvm.org/page/USB_Host_Device_Assigned_to_Guest)

