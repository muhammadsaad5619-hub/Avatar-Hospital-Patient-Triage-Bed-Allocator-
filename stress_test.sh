#!/bin/bash
# Spawns 20 patients rapidly

NAMES=("Ali" "Sara" "Ahmed" "Fatima" "Hassan" "Zara" "Omar" "Nadia"
       "Bilal" "Hina" "Usman" "Ayesha" "Kamran" "Sana" "Tariq"
       "Maryam" "Asad" "Rabia" "Faisal" "Amna")

for i in $(seq 0 19); do
    NAME=${NAMES[$i]}
    AGE=$((20 + RANDOM % 60))
    SEV=$((1 + RANDOM % 10))
    echo "Sending patient: $NAME age=$AGE severity=$SEV"
    bash scripts/triage.sh "$NAME" "$AGE" "$SEV" &
    sleep 0.3
done

wait
echo "Stress test done — 20 patients sent."
