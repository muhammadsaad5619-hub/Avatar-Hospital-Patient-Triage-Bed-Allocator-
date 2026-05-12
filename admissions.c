/*
 * ============================================================
 * Project : Hospital Patient Triage & Bed Allocator
 * File    : admissions.c
 * Purpose : Central admissions manager.
 *           Handles process spawning, IPC, threads,
 *           scheduling, synchronization, memory management.
 * Compile : gcc -Wall -pthread -o admissions admissions.c -lpthread
 * ============================================================
 */

#include "bed_allocator.h"

/* ════════════════════════════════════════
   GLOBALS
   ════════════════════════════════════════ */

/* Shared memory bed array */
BedPartition ward[MAX_BEDS];
int          total_partitions = 0;
AllocStrategy strategy        = STRATEGY_BEST;

/* Mutexes & condition variables */
pthread_mutex_t bed_mutex   = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  bed_freed   = PTHREAD_COND_INITIALIZER;
pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  queue_ready = PTHREAD_COND_INITIALIZER;

/* Semaphores */
sem_t icu_sem;
sem_t iso_sem;
sem_t queue_sem;   /* bounded buffer */

/* Patient waiting queue (linked list) */
typedef struct WaitNode {
    PatientRecord       patient;
    struct WaitNode    *next;
} WaitNode;

WaitNode *wait_head = NULL;
int       next_patient_id = 1;
int       running         = 1;   /* set 0 to stop threads */

/* Named FIFO file descriptor */
int discharge_fd = -1;

/* ════════════════════════════════════════
   WARD INITIALISATION
   ════════════════════════════════════════ */
void init_ward(void) {
    int idx = 0, unit = 0;

    /* 4 ICU beds — 3 care units each */
    for (int i = 0; i < ICU_COUNT; i++, idx++) {
        ward[idx].partition_id = idx;
        ward[idx].start_unit   = unit;
        ward[idx].size         = 3;
        ward[idx].is_free      = 1;
        ward[idx].patient_id   = -1;
        strcpy(ward[idx].bed_type, "ICU");
        unit += 3;
    }
    /* 4 Isolation beds — 2 care units each */
    for (int i = 0; i < ISO_COUNT; i++, idx++) {
        ward[idx].partition_id = idx;
        ward[idx].start_unit   = unit;
        ward[idx].size         = 2;
        ward[idx].is_free      = 1;
        ward[idx].patient_id   = -1;
        strcpy(ward[idx].bed_type, "ISOLATION");
        unit += 2;
    }
    /* 12 General beds — 1 care unit each */
    for (int i = 0; i < GEN_COUNT; i++, idx++) {
        ward[idx].partition_id = idx;
        ward[idx].start_unit   = unit;
        ward[idx].size         = 1;
        ward[idx].is_free      = 1;
        ward[idx].patient_id   = -1;
        strcpy(ward[idx].bed_type, "GENERAL");
        unit += 1;
    }
    total_partitions = idx;
    printf("[WARD] Initialized: %d ICU | %d ISO | %d General\n",
           ICU_COUNT, ISO_COUNT, GEN_COUNT);
}

/* ════════════════════════════════════════
   ALLOCATION ALGORITHMS
   ════════════════════════════════════════ */
int best_fit(int units_needed, const char *type) {
    int best = -1, best_size = INT_MAX;
    for (int i = 0; i < total_partitions; i++) {
        if (ward[i].is_free &&
            strcmp(ward[i].bed_type, type) == 0 &&
            ward[i].size >= units_needed &&
            ward[i].size < best_size) {
            best      = i;
            best_size = ward[i].size;
        }
    }
    return best;
}

int first_fit(int units_needed, const char *type) {
    for (int i = 0; i < total_partitions; i++) {
        if (ward[i].is_free &&
            strcmp(ward[i].bed_type, type) == 0 &&
            ward[i].size >= units_needed)
            return i;
    }
    return -1;
}

int worst_fit(int units_needed, const char *type) {
    int worst = -1, worst_size = -1;
    for (int i = 0; i < total_partitions; i++) {
        if (ward[i].is_free &&
            strcmp(ward[i].bed_type, type) == 0 &&
            ward[i].size >= units_needed &&
            ward[i].size > worst_size) {
            worst      = i;
            worst_size = ward[i].size;
        }
    }
    return worst;
}

int allocate_bed(int units_needed, const char *type) {
    int idx = -1;
    if      (strategy == STRATEGY_BEST)  idx = best_fit(units_needed, type);
    else if (strategy == STRATEGY_FIRST) idx = first_fit(units_needed, type);
    else                                  idx = worst_fit(units_needed, type);

    if (idx >= 0) {
        ward[idx].is_free    = 0;
        ward[idx].patient_id = next_patient_id;
    }
    return idx;
}

/* ════════════════════════════════════════
   FREE BED + COALESCING
   ════════════════════════════════════════ */
void print_ward_map(void) {
    printf("[WARD MAP] ");
    for (int i = 0; i < total_partitions; i++) {
        if (ward[i].is_free)
            printf("[FREE:%d] ", ward[i].size);
        else
            printf("[P%d:%s] ", ward[i].patient_id, ward[i].bed_type);
    }
    printf("\n");
}

void coalesce(void) {
    for (int i = 0; i < total_partitions - 1; i++) {
        if (ward[i].is_free && ward[i+1].is_free &&
            strcmp(ward[i].bed_type, ward[i+1].bed_type) == 0) {
            ward[i].size += ward[i+1].size;
            for (int j = i+1; j < total_partitions-1; j++)
                ward[j] = ward[j+1];
            total_partitions--;
            i--;
        }
    }
}

void free_bed(int patient_id) {
    for (int i = 0; i < total_partitions; i++) {
        if (ward[i].patient_id == patient_id) {
            printf("[BED FREE] Patient %d discharged from %s bed %d\n",
                   patient_id, ward[i].bed_type, i);
            print_ward_map();            ward[i].is_free    = 1;
            ward[i].patient_id = -1;
            coalesce();
            printf("[AFTER COALESCE] ");
            print_ward_map();

            /* release semaphore based on bed type */
            if (strcmp(ward[i].bed_type, "ICU") == 0)
                sem_post(&icu_sem);
            else if (strcmp(ward[i].bed_type, "ISOLATION") == 0)
                sem_post(&iso_sem);
            return;
        }
    }
}

/* ════════════════════════════════════════
   FRAGMENTATION REPORT
   ════════════════════════════════════════ */
void report_fragmentation(void) {
    int total_free = 0, largest = 0;
    for (int i = 0; i < total_partitions; i++) {
        if (ward[i].is_free) {
            total_free += ward[i].size;
            if (ward[i].size > largest) largest = ward[i].size;
        }
    }
    float frag = 0;
    if (total_free > 0)
        frag = (1.0f - (float)largest / total_free) * 100.0f;

    printf("[FRAG] Free=%d | Largest=%d | Ext.Frag=%.1f%%\n",
           total_free, largest, frag);

    /* Log to file */
    FILE *fp = fopen("logs/memory_log.txt", "a");
    if (fp) {
        time_t t = time(NULL);
        char *ts = ctime(&t); ts[strlen(ts)-1] = '\0';
        fprintf(fp, "[%s] Free=%d Largest=%d Frag=%.1f%%\n",
                ts, total_free, largest, frag);
        fclose(fp);
    }
}

/* ════════════════════════════════════════
   PAGING SIMULATION
   ════════════════════════════════════════ */
void paging_report(int patient_id, int care_units) {
    int pages   = (care_units + PAGE_SIZE - 1) / PAGE_SIZE;
    int wasted  = pages * PAGE_SIZE - care_units;
    printf("[PAGING] Patient %d: needs %d care units → %d pages "
           "(internal frag = %d unit(s))\n",
           patient_id, care_units, pages, wasted);
}

/* ════════════════════════════════════════
   PRIORITY QUEUE
   ════════════════════════════════════════ */
void enqueue(PatientRecord p) {
    WaitNode *node = malloc(sizeof(WaitNode));
    node->patient  = p;
    node->next     = NULL;

    if (!wait_head || p.priority < wait_head->patient.priority) {
        node->next = wait_head;
        wait_head  = node;
    } else {
        WaitNode *cur = wait_head;
        while (cur->next && cur->next->patient.priority <= p.priority)
            cur = cur->next;
        node->next = cur->next;
        cur->next  = node;
    }
}

PatientRecord dequeue(void) {
    WaitNode *tmp  = wait_head;
    PatientRecord p = tmp->patient;
    wait_head      = wait_head->next;
    free(tmp);
    return p;
}

/* ════════════════════════════════════════
   SIGCHLD HANDLER
   ════════════════════════════════════════ */
void sigchld_handler(int sig) {
    (void)sig;
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0);
}

/* ════════════════════════════════════════
   ADMIT PATIENT — fork + exec
   ════════════════════════════════════════ */
void admit_patient(PatientRecord *p, int bed_idx) {
    /* Semaphore acquire */
    if (strcmp(ward[bed_idx].bed_type, "ICU") == 0)
        sem_wait(&icu_sem);
    else if (strcmp(ward[bed_idx].bed_type, "ISOLATION") == 0)
        sem_wait(&iso_sem);

    ward[bed_idx].patient_id = p->patient_id;

    paging_report(p->patient_id, p->care_units);
    report_fragmentation();

    /* Log to schedule_log.txt */
    FILE *fp = fopen("logs/schedule_log.txt", "a");
    if (fp) {
        time_t t = time(NULL);
        char *ts = ctime(&t); ts[strlen(ts)-1] = '\0';
        fprintf(fp, "[%s] Admitted P%d (Priority=%d) to %s bed %d\n",
                ts, p->patient_id, p->priority,
                ward[bed_idx].bed_type, bed_idx);
        fclose(fp);
    }

    pid_t pid = fork();
    if (pid == 0) {
        /* Child — become patient_simulator */
        char pid_s[12], pri_s[12], bed_s[12];
        sprintf(pid_s, "%d", p->patient_id);
        sprintf(pri_s, "%d", p->priority);
        sprintf(bed_s, "%d", bed_idx);
        char *args[] = { "./patient_simulator", pid_s, pri_s, bed_s, NULL };
        execv("./patient_simulator", args);
        perror("execv failed");
        _exit(1);
    } else if (pid < 0) {
        perror("fork failed");
    }
}

/* ════════════════════════════════════════
   DETERMINE BED TYPE FROM PRIORITY
   ════════════════════════════════════════ */
const char *bed_type_for(int priority) {
    if (priority <= 2) return "ICU";
    if (priority == 3) return "ISOLATION";
    return "GENERAL";
}

int care_units_for(int priority) {
    if (priority <= 2) return 3;
    if (priority == 3) return 2;
    return 1;
}

/* ════════════════════════════════════════
   THREAD: 
RECEPTIONIST
   Reads from triage FIFO, enqueues patients
   ════════════════════════════════════════ */
void *receptionist_thread(void *arg) {
    (void)arg;
    int fd = open(TRIAGE_FIFO, O_RDONLY);
    if (fd < 0) { perror("receptionist: open triage fifo"); return NULL; }

    PatientRecord p;
    while (running) {
        ssize_t n = read(fd, &p, sizeof(PatientRecord));
        if (n <= 0) { usleep(100000); continue; }

        printf("[RECEPTIONIST] New patient: %s (Priority %d)\n",
               p.name, p.priority);

        sem_wait(&queue_sem);   /* bounded buffer — block if full */

        pthread_mutex_lock(&queue_mutex);
        enqueue(p);
        pthread_cond_signal(&queue_ready);
        pthread_mutex_unlock(&queue_mutex);
    }
    close(fd);
    return NULL;
}

/* ════════════════════════════════════════
   THREAD: SCHEDULER
   Dequeues patients, finds bed, admits
   ════════════════════════════════════════ */
void *scheduler_thread(void *arg) {
    (void)arg;
    while (running) {
        pthread_mutex_lock(&queue_mutex);
        while (!wait_head && running)
            pthread_cond_wait(&queue_ready, &queue_mutex);
        if (!running) { pthread_mutex_unlock(&queue_mutex); break; }

        PatientRecord p = dequeue();
        sem_post(&queue_sem);   /* release one slot in bounded buffer */
        pthread_mutex_unlock(&queue_mutex);

        /* Find a bed */
        const char *btype  = bed_type_for(p.priority);
        p.care_units        = care_units_for(p.priority);

        pthread_mutex_lock(&bed_mutex);
        int bed_idx = -1;
        while (bed_idx < 0) {
            bed_idx = allocate_bed(p.care_units, btype);
            if (bed_idx < 0) {
                printf("[SCHEDULER] No bed for P%d (%s). Waiting...\n",
                       p.patient_id, btype);
                pthread_cond_wait(&bed_freed, &bed_mutex);
            }
        }
        pthread_mutex_unlock(&bed_mutex);

        admit_patient(&p, bed_idx);
    }
    return NULL;
}

/* ════════════════════════════════════════
   THREAD: NURSE (one per bed type)
   Reads discharge FIFO, frees beds
   ════════════════════════════════════════ */
void *nurse_thread(void *arg) {
    char *my_type = (char *)arg;
    printf("[NURSE-%s] Ready.\n", my_type);

    while (running) {
        int patient_id;
        ssize_t n = read(discharge_fd, &patient_id, sizeof(int));
        if (n <= 0) { usleep(50000); continue; }

        pthread_mutex_lock(&bed_mutex);
        free_bed(patient_id);
        report_fragmentation();
        pthread_cond_broadcast(&bed_freed);
        pthread_mutex_unlock(&bed_mutex);
    }
    return NULL;
}

/* ════════════════════════════════════════
   MAIN
   ════════════════════════════════════════ */
int main(int argc, char *argv[]) {
    /* Parse --strategy flag */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--strategy") == 0 && i+1 < argc) {
            if      (strcmp(argv[i+1], "first") == 0) strategy = STRATEGY_FIRST;
            else if (strcmp(argv[i+1], "worst") == 0) strategy = STRATEGY_WORST;
            else                                       strategy = STRATEGY_BEST;
        }
    }

    /* SIGCHLD */
    signal(SIGCHLD, sigchld_handler);

    /* Init ward */
    init_ward();

    /* Semaphores */
    sem_init(&icu_sem,   0, ICU_COUNT);
    sem_init(&iso_sem,   0, ISO_COUNT);
    sem_init(&queue_sem, 0, QUEUE_LIMIT);

    /* Create FIFOs */
    unlink(TRIAGE_FIFO);
    unlink(DISCHARGE_FIFO);
    mkfifo(TRIAGE_FIFO,    0666);
    mkfifo(DISCHARGE_FIFO, 0666);

    /* Open discharge FIFO (non-blocking so thread doesn't hang) */
    discharge_fd = open(DISCHARGE_FIFO, O_RDWR);
    if (discharge_fd < 0) { perror("open discharge fifo"); return 1; }

    printf("[ADMISSIONS] Hospital is OPEN. Strategy: %s\n",
           strategy == STRATEGY_BEST  ? "Best-Fit"  :
           strategy == STRATEGY_FIRST ? "First-Fit" : "Worst-Fit");

    /* Launch threads */
    pthread_t t_receptionist, t_scheduler;
    pthread_t t_nurse_icu, t_nurse_iso, t_nurse_gen;

    pthread_create(&t_receptionist, NULL, receptionist_thread, NULL);
    pthread_create(&t_scheduler,    NULL, scheduler_thread,    NULL);
    pthread_create(&t_nurse_icu,    NULL, nurse_thread, "ICU");
    pthread_create(&t_nurse_iso,    NULL, nurse_thread, "ISOLATION");
    pthread_create(&t_nurse_gen,    NULL, nurse_thread, "GENERAL");

    /* Main loop — read patient records from stdin (anonymous pipe from triage.sh) */
    PatientRecord p;
    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        memset(&p, 0, sizeof(p));
       sscanf(line, "%63s %d %d %d", p.name, &p.age, &p.severity, &p.priority);
printf("[DEBUG] Read: name=%s age=%d sev=%d pri=%d\n", p.name, p.age, p.severity, p.priority);
        p.patient_id   = next_patient_id++;
        p.arrival_time = time(NULL);
        p.care_units   = care_units_for(p.priority);
        p.burst_time   = (p.priority <= 2) ? 10 :
                         (p.priority == 3) ?  6 : 4;

        sem_wait(&queue_sem);
        pthread_mutex_lock(&queue_mutex);
        enqueue(p);
        pthread_cond_signal(&queue_ready);
        pthread_mutex_unlock(&queue_mutex);
    }

    /* Keep running until manually killed */
    pause();

    running = 0;
    pthread_cond_broadcast(&queue_ready);
    pthread_cond_broadcast(&bed_freed);

    pthread_join(t_receptionist, NULL);
    pthread_join(t_scheduler,    NULL);
    pthread_join(t_nurse_icu,    NULL);
    pthread_join(t_nurse_iso,    NULL);
    pthread_join(t_nurse_gen,    NULL);

    sem_destroy(&icu_sem);
    sem_destroy(&iso_sem);
    sem_destroy(&queue_sem);
    close(discharge_fd);
    unlink(TRIAGE_FIFO);
    unlink(DISCHARGE_FIFO);

    return 0;
}
