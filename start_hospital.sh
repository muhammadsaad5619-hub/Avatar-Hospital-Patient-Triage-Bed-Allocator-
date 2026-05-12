#!/bin/bash
# ============================================================
# Script  : start_hospital.sh
# Purpose : Launch the hospital admissions system
# ============================================================

echo "======================================"
echo "   HOSPITAL PATIENT TRIAGE SYSTEM"
echo "   ICU: 4 beds | ISO: 4 | General: 12"
echo "======================================"

STRATEGY=${1:-best}

cd "$(dirname "$0")/.."

./admissions --strategy "$STRATEGY" &
echo "Admissions manager started. PID=$!"
echo $! > /tmp/admissions.pid
