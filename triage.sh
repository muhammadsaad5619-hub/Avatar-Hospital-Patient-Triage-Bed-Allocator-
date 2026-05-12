#!/bin/bash
# ============================================================
# Project : Hospital Patient Triage & Bed Allocator
# Script  : triage.sh
# Purpose : Validate input, compute priority, pipe to admissions
# Usage   : ./triage.sh <name> <age> <severity 1-10>
# ============================================================

NAME=$1
AGE=$2
SEVERITY=$3

# Validate
if [ -z "$NAME" ]; then echo "Error: Name required"; exit 1; fi
if ! [[ "$AGE" =~ ^[0-9]+$ ]]; then echo "Error: Age must be numeric"; exit 1; fi
if ! [[ "$SEVERITY" =~ ^[0-9]+$ ]]; then echo "Error: Severity must be numeric"; exit 1; fi
if [ "$SEVERITY" -lt 1 ] || [ "$SEVERITY" -gt 10 ]; then
    echo "Error: Severity must be 1-10"; exit 1
fi

# Map severity to priority
if   [ "$SEVERITY" -le 2 ]; then PRIORITY=5
elif [ "$SEVERITY" -le 4 ]; then PRIORITY=4
elif [ "$SEVERITY" -le 6 ]; then PRIORITY=3
elif [ "$SEVERITY" -le 8 ]; then PRIORITY=2
else PRIORITY=1
fi

echo "Triage: $NAME Age=$AGE Severity=$SEVERITY → Priority=$PRIORITY"
echo "$NAME $AGE $SEVERITY $PRIORITY" > /tmp/triage_fifo

