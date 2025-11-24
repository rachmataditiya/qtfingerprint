# Libfprint Integration untuk Rust Server

## Strategi

Daripada re-implement FP3 parser dan matching algorithm, kita akan menggunakan `libfprint` secara langsung via FFI.

## Fungsi yang Dibutuhkan

1. **`fp_print_deserialize`** - Deserialize FP3 template menjadi FpPrint object
2. **`fpi_print_bz3_match`** - Match menggunakan bozorth3 algorithm
3. **`fpi_print_add_from_image`** - Convert raw image menjadi FpPrint untuk matching

## Pendekatan

### Opsi 1: Rust FFI langsung ke libfprint
- **Pros**: Langsung, tidak ada wrapper layer
- **Cons**: Kompleks karena GObject, GLib, memory management yang manual

### Opsi 2: C Wrapper sederhana (RECOMMENDED)
- **Pros**: 
  - Mudah handle GObject lifecycle
  - Bisa expose API yang lebih sederhana
  - Safe wrapper untuk Rust
- **Cons**: Extra compilation step

Kita akan gunakan **Opsi 2**.

## Struktur File

```
middleware/rust-api/
├── src/
│   ├── libfprint_wrapper/
│   │   ├── mod.rs           # Rust FFI bindings
│   │   └── safe.rs          # Safe Rust wrapper
│   └── ...
├── libfprint-wrapper/
│   ├── wrapper.c            # C wrapper functions
│   ├── wrapper.h            # Header
│   └── Makefile             # Build script
└── Cargo.toml               # Add build.rs untuk compile C code
```

## API C Wrapper

```c
// Deserialize FP3 template
int libfprint_deserialize_template(
    const uint8_t* data,
    size_t len,
    void** print_out,  // FpPrint* as opaque pointer
    char** error_out
);

// Create FpPrint from raw image
int libfprint_create_print_from_image(
    const uint8_t* image_data,
    size_t image_len,
    uint32_t width,
    uint32_t height,
    void** print_out,
    char** error_out
);

// Match two prints
int libfprint_match_prints(
    void* template_print,  // FpPrint* (from deserialize)
    void* probe_print,     // FpPrint* (from image)
    int threshold,         // bozorth3 threshold
    int* score_out,
    char** error_out
);

// Free FpPrint
void libfprint_free_print(void* print);
```

## Build Process

1. Compile C wrapper → `liblibfprint_wrapper.so/dylib`
2. Link dengan `libfprint-2` dan dependencies (GLib, GObject, GUSB)
3. Rust `build.rs` akan compile C wrapper dan link
4. Rust FFI bindings menggunakan `bindgen`

## Dependencies

- `libfprint-2` (shared library)
- `glib-2.0`, `gobject-2.0`, `gio-2.0`, `gusb` (via pkg-config)

## Testing

1. Test FP3 deserialization dengan data dari database
2. Test image → FpPrint conversion dari Android
3. Test matching dengan threshold yang sama seperti Qt client

