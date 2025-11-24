#!/bin/bash
# Wrapper script to run FingerprintApp with proper environment setup

APP_PATH="$(dirname "$0")/bin/FingerprintApp.app/Contents/MacOS/FingerprintApp"

# Set library paths for Homebrew libraries
export DYLD_FALLBACK_LIBRARY_PATH="/opt/homebrew/lib:/opt/homebrew/opt/qtbase/lib:/usr/local/lib:/usr/lib"

# Run the application
exec "$APP_PATH" "$@"
