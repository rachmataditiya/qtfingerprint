# USB Passthrough di macOS - Troubleshooting

## Status

✅ **Emulator berhasil dijalankan dengan USB passthrough flags**
❌ **USB device tidak berhasil di-passthrough karena macOS security restriction**

## Error yang Ditemukan

```
Error creating device: Failed to create IOUSBHostObject.
Service authorization failed with error with return code: -536870202
```

## Penjelasan

macOS memiliki security restrictions yang mencegah aplikasi mengakses USB devices secara langsung. Error `-536870202` adalah `kIOReturnNotPrivileged`, yang berarti emulator tidak punya permission untuk access USB device.

## Solusi

### Opsi 1: Gunakan Physical Android Device (Recommended ✅)

Cara paling reliable untuk testing dengan fingerprint hardware:
- Connect tablet via USB OTG
- Atau gunakan built-in fingerprint sensor
- Tidak ada permission issues

### Opsi 2: VirtualBox dengan Android x86

1. Install VirtualBox
2. Download Android x86 ISO
3. Setup USB passthrough di VirtualBox UI (lebih mudah daripada command line)
4. VirtualBox handle permission dengan lebih baik

### Opsi 3: Grant System Extensions Permission (Advanced)

MacOS memerlukan system extensions permission:

1. **Enable System Extensions:**
   ```bash
   # System Preferences > Security & Privacy > General
   # Allow extensions from developers
   ```

2. **Grant USB Access (may not work for emulator):**
   - System Preferences > Security & Privacy > Privacy
   - USB Accessories (jika ada di list)

3. **Note:** Android emulator tidak selalu bisa mendapatkan permission ini karena menggunakan QEMU.

### Opsi 4: Use Linux (Better USB Passthrough Support)

USB passthrough bekerja lebih baik di Linux:
- Direct QEMU access
- Kurang security restrictions
- Mudah grant USB access via udev rules

## Command yang Sudah Dicoba

```bash
# Start emulator dengan USB passthrough
emulator -avd Pixel_Tablet \
  -no-snapshot-load \
  -qemu -usb \
  -device usb-host,vendorid=0x05ba,productid=0x000a
```

**Result:** Emulator boots successfully, tapi USB device tidak terpassthrough karena permission error.

## Recommendation

Untuk development/testing fingerprint hardware di macOS:
1. ✅ Use **Physical Android Tablet** dengan USB OTG
2. ✅ Use **Hybrid Approach**: Qt desktop app untuk enrollment, Android untuk verification via REST API
3. ⚠️ USB passthrough ke emulator di macOS **tidak reliable** karena security restrictions

## Status Saat Ini

- ✅ Emulator running: `emulator-5554`
- ✅ Device model: Pixel Tablet
- ✅ ADB connection: Working
- ❌ USB passthrough: Failed (macOS security restriction)

## Next Steps

1. Test dengan physical tablet (recommended)
2. Atau continue dengan mock/testing tanpa hardware
3. Untuk production, gunakan physical devices

