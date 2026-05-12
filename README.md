# Avatar-Hospital-Patient-Triage-Bed-Allocator-
A C-based Operating System simulation project that models a hospital admission and patient triage system using core OS concepts such as:

Process Management
CPU Scheduling
Memory Allocation
Multithreading
Synchronization
IPC (Inter-Process Communication)
Semaphores & Mutexes
Paging Simulation

This project simulates how patients are admitted, prioritized, allocated beds, and discharged in a hospital environment.

# 📌 Features
 Patient Admission Simulation
Simulates incoming patients with different priorities and bed requirements.
Supports ICU, Isolation, and General wards.
 Bed Allocation Algorithms

Implements multiple memory allocation strategies:

Best Fit
First Fit
Worst Fit
 Multithreading

# Uses POSIX Threads (pthread) for:

Patient scheduling
Admission handling
Queue management
Bed monitoring
 Synchronization

# Implements:

Mutex Locks
Condition Variables
POSIX Semaphores

to avoid race conditions and manage shared resources safely.

Fragmentation & Coalescing
Simulates external fragmentation.
Performs coalescing of free partitions after patient discharge.
Logging System

# Maintains:

Memory logs
Scheduling logs
Patient activity logs
# 🧠 Operating System Concepts Used
Concept	Implementation
Process Management	Patient admission handling
Threads	Parallel scheduling and monitoring
IPC	Named FIFO communication
Synchronization	Mutexes, Semaphores, Condition Variables
Memory Management	Bed allocation strategies
Fragmentation	External fragmentation reporting
Scheduling	Patient priority handling
Paging	Basic paging simulation
# 📂 Project Structure
hospital-triage-os-simulator/
│
├── admissions.c              # Main hospital admission manager
├── patient_simulator.c       # Simulates incoming patients
├── bed_allocator.h           # Shared structures & definitions
├── Makefile                  # Build automation
├── triage.sh                 # Run triage simulation
├── start_hospital.sh         # Start complete simulation
├── stop_hospital.sh          # Stop running processes
├── stress_test.sh            # Stress testing script
├── memory_log.txt            # Memory allocation logs
├── schedule_log.txt          # Scheduling logs
└── README.md
# ⚙️ Requirements
Linux Environment

Recommended:

Ubuntu / Kali Linux / WSL
GCC Compiler

# Install:

sudo apt install build-essential
# 🚀 Compilation
Using Makefile
make
Manual Compilation
gcc -Wall -pthread -o admissions admissions.c -lpthread


gcc -Wall -pthread -o patient_simulator patient_simulator.c -lpthread
# ▶️ Running the Project
Start Full Simulation
./start_hospital.sh
Run Triage System
./triage.sh
Stress Testing
./stress_test.sh
Stop Simulation
./stop_hospital.sh
# 🛏️ Ward Configuration
Ward Type	Count	Units
ICU	4 Beds	3 Units Each
Isolation	4 Beds	2 Units Each
General	12 Beds	1 Unit Each
#📊 Allocation Strategies
Best Fit
Allocates the smallest suitable partition.
First Fit
Allocates the first available partition.
Worst Fit
Allocates the largest available partition.


# 👨‍💻 Author

Muhammad Saad

Artificial Intelligence Student | MERN Stack Learner | OS & AI Projects Enthusiast

# 📜 License

This project is created for educational and academic purposes.
