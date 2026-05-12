#ifndef BED_ALLOCATOR_H
#define BED_ALLOCATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <limits.h>

/* ─── Constants ─────────────────────────── */
#define SHM_KEY        0xBEDF00D
#define MAX_BEDS       20
#define TOTAL_UNITS    20
#define PAGE_SIZE      2
#define MAX_PATIENTS   50
#define QUEUE_LIMIT    20

#define ICU_COUNT      4
#define ISO_COUNT      4
#define GEN_COUNT      12

#define DISCHARGE_FIFO "/tmp/discharge_fifo"
#define TRIAGE_FIFO    "/tmp/triage_fifo"

/* ─── Patient Record ─────────────────────── */
typedef struct {
    int    patient_id;
    char   name[64];
    int    age;
    int    severity;       /* 1-10 raw */
    int    priority;       /* 1-5 computed */
    int    care_units;     /* memory units needed */
    time_t arrival_time;
    int    burst_time;     /* treatment duration in seconds */
} PatientRecord;

/* ─── Bed Partition ──────────────────────── */
typedef struct {
    int  partition_id;
    int  start_unit;
    int  size;
    int  is_free;
    int  patient_id;
    char bed_type[16];     /* "ICU" "GENERAL" "ISOLATION" */
} BedPartition;

/* ─── Allocation Strategy ───────────────── */
typedef enum {
    STRATEGY_BEST,
    STRATEGY_FIRST,
    STRATEGY_WORST
} AllocStrategy;

#endif /* BED_ALLOCATOR_H */
