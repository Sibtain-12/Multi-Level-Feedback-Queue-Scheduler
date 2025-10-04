# Multi-Level Feedback Queue (MLFQ) Scheduler

## Overview
This project implements an advanced **Multilevel Feedback Queue (MLFQ)** CPU scheduling algorithm with configurable parameters, aging mechanism, priority boosting, and comparative analysis with other scheduling algorithms (Round Robin, FCFS, SJF).

### Key Highlights

- **Initial Priority Support**: Processes can arrive with different priority levels (0=highest)
- **Dynamic Queue Migration**: Processes move between queues based on CPU usage patterns
- **Starvation Prevention**: Aging and priority boosting mechanisms ensure all processes eventually execute
- **Detailed Timeline**: Shows what happens after every preemption, demotion, and completion
- **Comparative Analysis**: Compare MLFQ performance with Round Robin, FCFS, and SJF schedulers

### Why MLFQ?

- **No burst time prediction required** (unlike SJF)
- **Better than Round Robin** for mixed workloads
- **Prevents convoy effect** (unlike FCFS)
- **Adaptive** - learns process behavior dynamically
- **Fair** - prevents starvation through aging

---

## Features

### Core MLFQ Features

- **Multiple Priority Queues**: Configurable number of queues (default: 3)
- **Initial Priority Assignment**: Processes start in queues based on their initial priority
- **Preemptive Scheduling**: Higher priority processes can preempt lower priority ones
- **Quantum-based Execution**: Each queue has configurable time quantum
- **Dynamic Demotion**: CPU-intensive processes move to lower priority queues
- **Aging Mechanism**: Prevents starvation by promoting long-waiting processes
- **Priority Boosting**: Periodic reset to initial priority for long-term fairness

### Analysis Features

- **Comprehensive Metrics**: Turnaround time, waiting time, throughput, CPU utilization, context switches
- **Gantt Chart**: Visual timeline of process execution
- **Detailed Event Logging**: Shows preemptions, demotions, promotions, and next process information
- **Queue Usage Statistics**: Percentage of time spent in each queue
- **Comparative Analysis**: Side-by-side comparison with RR, FCFS, and SJF

### Additional Features

- **Configurable Parameters**: Load settings from external config file
- **File I/O Support**: Read processes from file, save results to file
- **Interactive Mode**: Manual input of process data
- **Verbose/Silent Modes**: Detailed output for debugging or quiet mode for comparison

---

## Project Structure
```
mlfq_scheduler/
├── mlfq_scheduler.cpp   # Main source code
├── config.txt           # Configuration file for queue parameters
├── sample_input.txt     # Sample input with 6 processes
├── mlfq_extreme.txt     # Test case with 8 processes
├── README.md            # This file
├── mlfq_results.txt     # Generated: MLFQ execution results
└── comparison_results.txt # Generated: Comparative analysis results
```
---

## Algorithm Explanation

### MLFQ Queue Structure (Default)
```
+--------------------------------------+ 
| Q0 (Priority 0 - Highest)            | ← Highest Priority
| - Algorithm: Round Robin (TQ=4)      |
| - Purpose: System/Critical processes |
| - New arrivals with priority 0       |
+--------------------------------------+
          ↓ (if quantum exhausted)
+-----------------------------------+
| Q1 (Priority 1 - Medium)          | ← Medium Priority
| - Algorithm: Round Robin (TQ=8)   |
| - Purpose: User/Normal processes  |
| - New arrivals with priority 1    |  
+-----------------------------------+
          ↓ (if quantum exhausted)
+---------------------------------------+
| Q2 (Priority 2 - Lowest)              | ← Lowest Priority
| - Algorithm: FCFS                     |
| - Purpose: Background/Batch processes |
| - New arrivals with priority 2        |
+---------------------------------------+
```
### Process Movement Rules

#### 1. Initial Placement
- Process arrives with **initial priority** (0, 1, or 2)
- Placed in corresponding queue immediately
- System processes (priority 0) → Q0
- User processes (priority 1) → Q1
- Background processes (priority 2) → Q2

#### 2. Demotion (Downward Movement)
- **Trigger**: Process exhausts time quantum without completing

- **Action**: Move to next lower priority queue

#### 3. Aging (Upward Movement)
- **Trigger**: Process waits ≥ aging_threshold (default: 15 time units)

- **Action**: Promote to next higher priority queue

- **Purpose**: Prevent starvation

#### 4. Priority Boosting
- **Trigger**: Every boost_interval (default: 50 time units)

- **Action**: Reset ALL processes to their **initial priority**

- **Purpose**: 
  - Prevent gaming of the scheduler
  - Give all processes a fresh chance
  - Respect original process importance

#### 5. Preemption
**Rule**: Higher priority queue has work → preempt lower priority process

---

## Compilation

### Requirements
- C++17 compatible compiler (g++ 7.0+)
- Standard C++ libraries

### Compile Command
```g++ -std=c++17 mlfq_scheduler.cpp -O2 -o mlfq_scheduler```

---

## Usage

### 1. Basic Usage (Default Configuration)
```./mlfq_scheduler sample_input.txt```

When prompted:
- Enter `y` for comparative analysis
- Enter `n` to skip comparison

### 2. With Custom Configuration
```./mlfq_scheduler sample_input.txt -c```

This loads parameters from `config.txt`

### 3. Interactive Mode (Manual Input)
```./mlfq_scheduler```

Then enter process details manually when prompted.

### 4. Test MLFQ Advantage
```./mlfq_scheduler mlfq_extreme.txt```

Press `y` for comparative analysis to see MLFQ outperform other algorithms!

---

## Input File Format

### Process Input File Format
```
<Number of Processes>
<PID> <Arrival Time> <Burst Time> <InitialPriority>
<PID> <Arrival Time> <Burst Time> <InitialPriority>
...
```

**Field Descriptions**:
- **PID**: Process ID (unique identifier)
- **Arrival**: Arrival time in the system
- **Burst**: Total CPU time required
- **InitialPriority**: Starting priority level

### Priority Levels

| Priority | Queue | Typical Use Case | Examples |
|----------|-------|------------------|----------|
| **0** | Q0 | System/Critical | Init, kernel threads, system daemons |
| **1** | Q1 | User/Normal | User applications, interactive programs |
| **2** | Q2 | Background/Batch | Backups, compilers, long computations |

### Configuration File Format
```
<Number of Queues>
<Time Quantum Q0> <Time Quantum Q1> ... (0 for FCFS)
<Algorithm Name Q0>
<Algorithm Name Q1>
...
<Aging Threshold> <Aging Check Interval> <Boost Interval>
```
### Configuration Parameters
| Parameter            | Description                       | Default   | Range     |
|----------------------|-----------------------------------|-----------|-----------|
| Number of Queues     | Total queue levels in MLFQ        | 3         | 1–10      |
| Time Quantum         | Time slice for RR queues (0=FCFS) | [4, 8, 0] | 0–100     |
| Aging Threshold      | Time units before promotion       | 15        | 5–50      |
| Aging Check Interval | How often to check aging          | 3         | 1–10      |
| Boost Interval       | Priority boost frequency          | 50        | 10–1000   |

---

## Output

### Console Output
The program displays:
  1. **Configuration Details:** Queue settings and parameters
  2. **Execution Timeline:** Process arrivals, demotions, completions
  3. **Queue Usage Statistics:** Time spent in each queue
  4. **Process-wise Metrics:** Individual TAT and WT for each process
  5. **Overall Performance Metrics:** Averages and system-wide statistics
  6. **Gantt Chart:** Visual execution timeline
  7. **Comparative Analysis:** Side-by-side comparison with other algorithms
### Generated Files
1. ```mlfq_results.txt``` : Contains detailed MLFQ scheduler results
2. ```comparison_results.txt``` : Contains comparative analysis

---

## Performance Metrics

| Metric               | Formula                         | Description              | Lower/Higher is Better |
|-----------------------|---------------------------------|--------------------------|-------------------------|
| Turnaround Time (TAT) | Completion − Arrival            | Total time in system     | Lower ✓                |
| Waiting Time (WT)     | TAT − Burst                     | Time waiting in queues   | Lower ✓                |
| Throughput            | Processes / Total Time          | Jobs completed per unit  | Higher ✓               |
| CPU Utilization       | (Busy Time / Total Time) × 100  | % CPU is active          | Higher ✓ (ideally 100%)|
| Context Switches      | Count of process switches       | Overhead indicator       | Lower ✓                |

---

## Algorithm Characteristics

| Algorithm    |  Strengths                             |  Weaknesses                               |  Best Use Case                          |
|--------------|----------------------------------------|-------------------------------------------|-----------------------------------------|
| MLFQ         |  Adaptive, fair, no prediction needed  |  More complex, overhead                   |  General purpose OS, mixed workloads    |
| Round Robin  |  Fair, simple, good for time-sharing   |  Equal treatment (not adaptive)           |  Pure time-sharing systems              |
| FCFS         |  Simple, low overhead                  |  Convoy effect, poor for interactive      |  Batch systems with known order         |
| SJF          |  Optimal average TAT                   |  Needs burst prediction, starvation risk  |  Batch systems with known bursts        |

### When MLFQ Excels
#### MLFQ performs best when:
- Workload has mix of short and long processes
- Burst times unknown (no prediction available)
- Need responsiveness for interactive processes
- Need fairness (prevent starvation)
- Processes have varying behavior over time

### When Other Schedulers May Be Better
#### Use Round Robin if:
- All processes equally important
- Simple implementation needed
- Time-sharing is the primary goal

#### Use FCFS if:
- Batch processing only
- Simplicity is critical
- Processes arrive in optimal order

#### Use SJF if:
- Burst times known in advance
- Minimizing average TAT is the only goal
- Starvation acceptable for long processes

---
