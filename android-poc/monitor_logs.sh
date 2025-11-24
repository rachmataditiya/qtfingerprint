#!/bin/bash
# Monitor Android logs for fingerprint capture debugging

DEVICE="192.168.1.25:45935"

echo "📱 Monitoring logs for fingerprint capture..."
echo "Press Ctrl+C to stop"
echo ""

adb -s $DEVICE logcat -v time \
  URU4000Protocol:* \
  URU4000ImageDecoder:* \
  USBFingerprintManager:* \
  MainActivity:* \
  FingerprintManager:* \
  AndroidRuntime:E \
  *:S

