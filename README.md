# U.are.U 4500 Fingerprint Application

Complete fingerprint enrollment and verification system for DigitalPersona U.are.U 4500 fingerprint readers.

## Project Structure

```
├── digitalpersonalib/          # Reusable fingerprint library
│   ├── include/               # Public headers
│   ├── src/                   # Implementation
│   ├── examples/              # Example code
│   ├── lib/                   # Compiled library
│   ├── README.md              # Library documentation
│   ├── API.md                 # API reference
│   └── INSTALL.md             # Installation guide
│
├── main_app.cpp               # Application entry point
├── mainwindow_app.*           # Main application window
├── database_manager.*         # Database management (app-side)
├── fingerprint_app.pro        # Qt project file
└── run_app.sh                 # Run script
```

## Architecture

```
┌─────────────────────────────────────┐
│   FingerprintApp (Qt GUI)           │
│   ├── MainWindowApp                 │
│   └── DatabaseManager (local)       │
├─────────────────────────────────────┤
│   libdigitalpersona.so               │
│   └── FingerprintManager            │
├─────────────────────────────────────┤
│   libfprint-2                        │
├─────────────────────────────────────┤
│   U.are.U 4500 Device (USB)          │
└─────────────────────────────────────┘
```

**Key Design:**
- **Library** (`digitalpersonalib/`) - Handles fingerprint operations only
- **Application** - Manages UI, database, and business logic
- Clean separation of concerns

## Features

✅ **Fingerprint Enrollment** - 5-stage scanning process with progress feedback  
✅ **Fingerprint Verification** - Match against stored templates  
✅ **User Management** - SQLite database for user storage  
✅ **Qt6 GUI** - Modern, responsive interface  
✅ **Library Architecture** - Reusable fingerprint library  
✅ **ARM64 Optimized** - Native support for ARM64 Linux

## Requirements

### System
- Linux (Ubuntu/Debian recommended)
- Qt6 (Core, Widgets, SQL)
- libfprint-2
- SQLite3
- U.are.U 4500 fingerprint reader (USB)

### Build Tools
- qmake (Qt6)
- g++ with C++17 support
- make

## Installation

### 1. Install Dependencies

```bash
sudo apt update
sudo apt install qt6-base-dev libfprint-2-dev libglib2.0-dev libqt6sql6-sqlite
```

### 2. Build Library

```bash
cd digitalpersonalib
qmake6
make
cd ..
```

### 3. Build Application

```bash
qmake6 fingerprint_app.pro
make
```

## Usage

### Run Application

```bash
./run_app.sh
```

Or directly:

```bash
export LD_LIBRARY_PATH=./bin:./digitalpersonalib/lib:$LD_LIBRARY_PATH
./bin/FingerprintApp
```

### Device Permissions

Add your user to the `plugdev` group:

```bash
sudo usermod -aG plugdev $USER
```

Log out and log back in for changes to take effect.

### Application Workflow

1. **Initialize Reader**
   - Click "Initialize Reader"
   - Device should be detected and opened

2. **Enroll User**
   - Enter name and email
   - Click "Start Enrollment"
   - Click "Capture Fingerprint Sample"
   - Scan your finger 5 times when prompted
   - User saved to database automatically

3. **Verify User**
   - Select user from list
   - Click "Start Verification"
   - Click "Capture for Verification"
   - Place finger on reader
   - View match result

## Development

### Using the Library in Your Own Project

See `digitalpersonalib/README.md` for complete documentation.

Quick start:

```qmake
# In your .pro file
LIBS += -L$$PWD/digitalpersonalib/lib -ldigitalpersona
INCLUDEPATH += $$PWD/digitalpersonalib/include
QMAKE_RPATHDIR += $$PWD/digitalpersonalib/lib
```

```cpp
#include <digitalpersona.h>

FingerprintManager fpManager;
fpManager.initialize();
fpManager.openReader();
// ... use the API
```

### Clean Build

```bash
# Clean everything
make clean
cd digitalpersonalib && make clean && cd ..

# Rebuild all
cd digitalpersonalib && qmake6 && make && cd ..
qmake6 fingerprint_app.pro && make
```

## Troubleshooting

### Device Not Found
- Check USB connection: `lsusb | grep "Digital Persona"`
- Verify permissions: `groups | grep plugdev`
- Restart application after adding to group

### Library Not Found
```bash
export LD_LIBRARY_PATH=./bin:./digitalpersonalib/lib:$LD_LIBRARY_PATH
```

### Compilation Errors
```bash
# Check Qt6 installation
qmake6 --version

# Check libfprint
pkg-config --modversion libfprint-2

# Reinstall dependencies if needed
sudo apt install --reinstall qt6-base-dev libfprint-2-dev
```

## File Overview

| File | Purpose |
|------|---------|
| `fingerprint_app.pro` | Qt project configuration |
| `main_app.cpp` | Application entry point |
| `mainwindow_app.*` | Main window UI and logic |
| `database_manager.*` | SQLite database operations |
| `run_app.sh` | Convenience run script |
| `digitalpersonalib/` | Reusable fingerprint library |

## Database Schema

SQLite database: `fingerprint.db`

```sql
CREATE TABLE users (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,
    email TEXT,
    fingerprint_template BLOB NOT NULL,
    created_at TEXT NOT NULL,
    updated_at TEXT NOT NULL
);
```

## Version History

### v1.0.0 (Current)
- Library-based architecture
- Separate library from application
- Database management in application layer
- Clean API separation

### v0.2.0
- Added reusable library
- pkg-config support

### v0.1.0
- Initial PoC
- Basic enrollment and verification

## Contributing

When contributing:
1. Build and test library separately
2. Ensure application compiles against library
3. Update documentation if API changes
4. Follow Qt coding conventions

## License

This software is provided as-is for use with DigitalPersona U.are.U fingerprint readers.

## Support

- Library Documentation: `digitalpersonalib/README.md`
- API Reference: `digitalpersonalib/API.md`
- Installation: `digitalpersonalib/INSTALL.md`

## Authors

Arkana Development Team

---

**Device:** Digital Persona U.are.U 4000/4000B/4500  
**Backend:** libfprint-2  
**Framework:** Qt6  
**Platform:** Linux (ARM64 optimized)

