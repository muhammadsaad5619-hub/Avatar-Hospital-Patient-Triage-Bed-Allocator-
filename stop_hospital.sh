#!/bin/bash
# ============================================================
# Script  : stop_hospital.sh
# Purpose : Shut down hospital and clean up IPC resources
# ============================================================

echo "Shutting down hospital..."

if [ -f /tmp/admissions.pid ]; then
    kill -SIGTERM $(cat /tmp/admissions.pid) 2>/dev/null
    rm /tmp/admissions.pid
fi

pkill -f admissions 2>/dev/null
pkill -f patient_simulator 2>/dev/null

rm -f /tmp/discharge_fifo /tmp/triage_fifo

echo "All resources cleaned. Hospital closed."
