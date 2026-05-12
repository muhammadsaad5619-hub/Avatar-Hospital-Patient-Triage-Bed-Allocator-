CC      = gcc
CFLAGS  = -Wall -Wextra -pthread
TARGETS = admissions patient_simulator

all: $(TARGETS)

admissions: src/admissions.c src/bed_allocator.h
	$(CC) $(CFLAGS) -o admissions src/admissions.c -lpthread

patient_simulator: src/patient_simulator.c src/bed_allocator.h
	$(CC) $(CFLAGS) -o patient_simulator src/patient_simulator.c -lpthread

clean:
	rm -f $(TARGETS) *.o logs/schedule_log.txt logs/memory_log.txt

run:
	bash scripts/start_hospital.sh

test:
	bash scripts/stress_test.sh
