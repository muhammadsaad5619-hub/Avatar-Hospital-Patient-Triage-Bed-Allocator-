/*
 * ============================================================
 * Project : Hospital Patient Triage & Bed Allocator
 * File    : patient_simulator.c
 * Purpose : Simulates one patient's treatment lifecycle.
 * Compile : gcc -Wall -o patient_simulator patient_simulator.c
 * ============================================================
 */

#include "../src/bed_allocator.h"

int main(int argc, char *argv[]) {
    if (argc < 4) {
        fprintf(stderr, "Usage: patient_simulator <patient_id> <priority> <bed_id>\n");
        return 1;
    }

    int patient_id = atoi(argv[1]);
    int priority   = atoi(argv[2]);
    int bed_id     = atoi(argv[3]);

    srand(time(NULL) ^ getpid());

    /* ── Print arrival message ── */
    printf("[PATIENT %d] Arrived. Priority=%d BedID=%d PID=%d\n",
           patient_id, priority, bed_id, getpid());
    fflush(stdout);

    /* ── Determine sleep time by bed type ── */
    int sleep_time;
    if (priority <= 2)
        sleep_time = 5 + rand() % 11;   /* ICU: 5-15 sec */
    else if (priority == 3)
        sleep_time = 3 + rand() % 8;    /* Isolation: 3-10 sec */
    else
        sleep_time = 2 + rand() % 7;    /* General: 2-8 sec */

    printf("[PATIENT %d] Treatment started. Duration=%d sec\n",
           patient_id, sleep_time);
    fflush(stdout);

    sleep(sleep_time);

    /* ── Print discharge message ── */
    printf("[PATIENT %d] Treatment complete. Leaving bed %d.\n",
           patient_id, bed_id);
    fflush(stdout);

    /* ── Notify admissions via named FIFO ── */
    int fifo_fd = open(DISCHARGE_FIFO, O_WRONLY);
    if (fifo_fd < 0) {
        perror("[PATIENT] Failed to open discharge FIFO");
        return 1;
    }
    write(fifo_fd, &patient_id, sizeof(int));
    close(fifo_fd);

    return 0;
}
