# Multilevel Feedback Queue (MLFQ) Scheduler

## Overview
This project implements an advanced **Multilevel Feedback Queue (MLFQ)** CPU scheduling algorithm with configurable parameters, aging mechanism, priority boosting, and comparative analysis with other scheduling algorithms (Round Robin, FCFS, SJF).

## Features

### Core MLFQ Features
- **Multiple Queue Levels**: Configurable number of queues (default: 3)
- **Flexible Scheduling Algorithms**: Each queue can use different algorithms (RR, FCFS)
- **Preemption Support**: Higher priority processes can preempt lower priority ones
- **Aging Mechanism**: Prevents starvation by promoting long-waiting processes
- **Priority Boosting**: Periodically moves all processes to highest priority queue
- **Dynamic Process Migration**: Processes move between queues based on behavior

### Analysis Features
- **Comprehensive Metrics**: Turnaround time, waiting time, throughput, CPU utilization, context switches
- **Gantt Chart Visualization**: Visual representation of process execution timeline
- **Comparative Analysis**: Compare MLFQ with RR, FCFS, and SJF schedulers
- **Queue Usage Statistics**: Shows time spent in each queue with percentages

## Project Structure
```
mlfq_scheduler/
├── mlfq_scheduler.cpp   # Main source code (COMPLETE IMPLEMENTATION)
├── config.txt           # Configuration file for queue parameters
├── sample_input.txt     # Sample input with 6 processes
├── mlfq_extreme.txt     # Test case where MLFQ excels
├── README.md            # This file
├── mlfq_results.txt     # Generated: MLFQ execution results
└── comparison_results.txt # Generated: Comparative analysis results
```
## Compilation

### Requirements
- C++17 compatible compiler (g++ 7.0+)
- Standard C++ libraries

### Compile Command
```g++ -std=c++17 mlfq_scheduler.cpp -O2 -o mlfq_scheduler```

## Usage

### 1. Basic Usage (Default Configuration)
```./mlfq_scheduler sample_input.txt```

When prompted:
- Enter `y` for comparative analysis
- Enter `n` to skip comparison

### 2. With Custom Configuration
```./mlfq_scheduler sample_input.txt -c```
Then enter process details manually when prompted.

### 4. Test MLFQ Advantage
```./mlfq_scheduler mlfq_extreme.txt```
Press `y` for comparative analysis to see MLFQ outperform other algorithms!

## Input File Format

### Process Input File Format
```
<Number of Processes>
<PID> <Arrival Time> <Burst Time> 
<PID> <Arrival Time> <Burst Time> 
...
```
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

## Algorithm Explanation

### MLFQ Queue Structure (Default)
```
+-----------------------------------+ 
| Q0: Round Robin (TQ=4)            | ← Highest Priority
| - New processes enter here        |
| - Interactive/short processes     |
+-----------------------------------+
          ↓ (if quantum exhausted)
+-----------------------------------+
| Q1: Round Robin (TQ=8)            | ← Medium Priority
| - Processes needing more CPU time |
| - Longer quantum for efficiency   |
+-----------------------------------+
          ↓ (if quantum exhausted)
+-----------------------------------+
| Q2: FCFS                          | ← Lowest Priority
| - CPU-bound, long-running         |
| - Runs to completion              |
+-----------------------------------+
```
### Process Movement Rules
**Demotion (↓):**
  - Process exhausts time quantum → Move to next lower queue
  - Prevents CPU-bound processes from monopolizing high-priority queues

**Promotion (↑):**
  - Aging: Process waits ≥15 time units → Move to higher queue
  - Priority Boosting: Every 50 time units, all processes → Q0

## Performance Metrics

| Metric               | Formula                         | Description              | Lower/Higher is Better |
|-----------------------|---------------------------------|--------------------------|-------------------------|
| Turnaround Time (TAT) | Completion − Arrival            | Total time in system     | Lower ✓                |
| Waiting Time (WT)     | TAT − Burst                     | Time waiting in queues   | Lower ✓                |
| Throughput            | Processes / Total Time          | Jobs completed per unit  | Higher ✓               |
| CPU Utilization       | (Busy Time / Total Time) × 100  | % CPU is active          | Higher ✓ (ideally 100%)|
| Context Switches      | Count of process switches       | Overhead indicator       | Lower ✓                |

## Advantages of MLFQ

1. No Burst Time Prediction: Unlike SJF, doesn't need prior knowledge
2. Responsive to Interactive Jobs: Short jobs complete quickly
3. No Starvation: Aging and boosting prevent indefinite waiting
4. Adaptive: Learns process behavior dynamically
5. Fair: Balances short and long processes
6. Practical: Used in real operating systems (Windows, Linux)
