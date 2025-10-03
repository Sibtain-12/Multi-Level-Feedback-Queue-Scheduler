#include <iostream>
#include <vector>
#include <queue>
#include <deque>
#include <algorithm>
#include <string>
#include <iomanip>
#include <fstream>
#include <map>
#include <climits>

using namespace std;

/*
 =======================================================
 MULTILEVEL FEEDBACK QUEUE (MLFQ) SCHEDULER
 =======================================================
*/

// ==================== PROCESS STRUCTURE ====================
struct Process {
    int pid;
    int arrival;
    int burst;
    int remaining;
    int priority;
    int start_time;
    int completion;
    int time_in_current_quantum;
    int time_in_queue;
    bool started;

    Process() {}
    Process(int pid_, int a, int b) {
        pid = pid_; 
        arrival = a; 
        burst = b;
        remaining = b; 
        priority = 0;
        start_time = -1; 
        completion = -1;
        time_in_current_quantum = 0;
        time_in_queue = 0;
        started = false;
    }
    
    Process(const Process& other) {
        pid = other.pid;
        arrival = other.arrival;  // Copy original arrival
        burst = other.burst;      // Copy original burst
        remaining = other.burst;  // Reset remaining to original burst
        priority = 0;             // Reset to highest priority
        start_time = -1;          // Reset start time
        completion = -1;          // Reset completion
        time_in_current_quantum = 0;
        time_in_queue = 0;
        started = false;          // Reset started flag
    }
};

// Store original process data for multiple simulations
struct ProcessOriginal {
    int pid;
    int arrival;
    int burst;
    
    ProcessOriginal(int p, int a, int b) : pid(p), arrival(a), burst(b) {}
};

// ==================== CONFIGURATION ====================
struct Config {
    int num_queues;
    vector<int> time_quantum;
    vector<string> algo_names;
    int aging_threshold;
    int aging_check_interval;
    int boost_interval;
    
    Config() {
        num_queues = 3;
        time_quantum = {4, 8, 0};
        algo_names = {"Round-Robin", "Round-Robin", "FCFS"};
        aging_threshold = 15;
        aging_check_interval = 3;
        boost_interval = 50;
    }
    
    bool load_from_file(const string& filename) {
        ifstream fin(filename);
        if (!fin) return false;
        
        fin >> num_queues;
        time_quantum.resize(num_queues);
        algo_names.resize(num_queues);
        
        for (int i = 0; i < num_queues; i++) {
            fin >> time_quantum[i];
        }
        
        fin.ignore();
        for (int i = 0; i < num_queues; i++) {
            getline(fin, algo_names[i]);
        }
        
        fin >> aging_threshold >> aging_check_interval >> boost_interval;
        fin.close();
        return true;
    }
    
    void display() {
        cout << "Configuration:\n";
        cout << "  Number of Queues: " << num_queues << "\n";
        for (int i = 0; i < num_queues; i++) {
            cout << "  Q" << i << ": " << algo_names[i];
            if (time_quantum[i] > 0) {
                cout << " (TQ=" << time_quantum[i] << ")";
            }
            cout << "\n";
        }
        cout << "  Aging Threshold: " << aging_threshold << " time units\n";
        cout << "  Aging Check Interval: Every " << aging_check_interval << " time units\n";
        cout << "  Priority Boost Interval: Every " << boost_interval << " time units\n";
    }
};

// ==================== PERFORMANCE METRICS ====================
struct Metrics {
    double avg_turnaround;
    double avg_waiting;
    double throughput;
    double cpu_util;
    int context_switches;
    
    void display(const string& scheduler_name) {
        cout << "\n" << scheduler_name << " Performance:\n";
        cout << "  Avg Turnaround Time: " << fixed << setprecision(2) << avg_turnaround << "\n";
        cout << "  Avg Waiting Time   : " << fixed << setprecision(2) << avg_waiting << "\n";
        cout << "  Throughput         : " << fixed << setprecision(3) << throughput << " jobs/unit\n";
        cout << "  CPU Utilization    : " << fixed << setprecision(2) << cpu_util << " %\n";
        cout << "  Context Switches   : " << context_switches << "\n";
    }
};

// ==================== MLFQ SCHEDULER CLASS ====================
class MLFQ_Scheduler {
private:
    vector<Process> all_processes;
    deque<Process*> queues[10];
    Config config;
    int current_time;
    int completed;
    int total_busy_time;
    int context_switches;
    vector<int> timeline_pid;
    vector<int> timeline_queue;
    Process* currently_running;
    bool verbose_mode;  // verbose flag
    
public:
    MLFQ_Scheduler(vector<Process>& procs, const Config& cfg) {
        all_processes = procs;
        config = cfg;
        current_time = 0;
        completed = 0;
        total_busy_time = 0;
        context_switches = 0;
        currently_running = nullptr;
        verbose_mode = true;  // Default to verbose
    }
    
    void add_arrivals() {
        for (auto& p : all_processes) {
            if (p.arrival == current_time && !p.started && p.start_time == -1) {
                p.priority = 0;
                queues[0].push_back(&p);
                
                // Only print if verbose mode enabled
                if (verbose_mode) {
                    cout << "Time " << current_time << ": Process P" << p.pid 
                         << " arrived -> Q0\n";
                }
            }
        }
    }
    
    void apply_aging() {
        for (int q = 1; q < config.num_queues; q++) {
            for (auto it = queues[q].begin(); it != queues[q].end(); ) {
                Process* p = *it;
                
                if (p != currently_running && p->time_in_queue >= config.aging_threshold) {
                    // Only print if verbose mode enabled
                    if (verbose_mode) {
                        cout << "Time " << current_time << ": Process P" << p->pid 
                             << " promoted Q" << q << " -> Q" << (q-1) << " (Aging)\n";
                    }
                    
                    p->priority = q - 1;
                    p->time_in_queue = 0;
                    p->time_in_current_quantum = 0;
                    queues[q - 1].push_back(p);
                    it = queues[q].erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
    
    void apply_priority_boost() {
        // Only print if verbose mode enabled
        if (verbose_mode) {
            cout << "Time " << current_time << ": PRIORITY BOOST - All processes moved to Q0\n";
        }
        
        vector<Process*> all_waiting;
        for (int q = 1; q < config.num_queues; q++) {
            for (auto p : queues[q]) {
                p->priority = 0;
                p->time_in_queue = 0;
                p->time_in_current_quantum = 0;
                all_waiting.push_back(p);
            }
            queues[q].clear();
        }
        
        for (auto p : all_waiting) {
            queues[0].push_back(p);
        }
        
        if (currently_running != nullptr && currently_running->priority > 0) {
            currently_running->priority = 0;
            currently_running->time_in_current_quantum = 0;
        }
    }
    
    void update_waiting_times() {
        for (int q = 0; q < config.num_queues; q++) {
            for (auto p : queues[q]) {
                if (p != currently_running && p->remaining > 0) {
                    p->time_in_queue++;
                }
            }
        }
    }
    
    int get_highest_priority_queue() {
        for (int q = 0; q < config.num_queues; q++) {
            if (!queues[q].empty()) {
                return q;
            }
        }
        return -1;
    }
    
    bool should_preempt(int running_queue) {
        for (int q = 0; q < running_queue; q++) {
            if (!queues[q].empty()) {
                return true;
            }
        }
        return false;
    }
    
    void run(bool verbose = true) {
        verbose_mode = verbose;  // Store verbose flag
        
        if (verbose_mode) {
            cout << "\n========================\n";
            cout << "MLFQ SCHEDULER \n";
            cout << "==========================\n";
            config.display();
            cout << "========================================\n\n";
        }
        
        while (completed < all_processes.size()) {
            add_arrivals();
            
            for (int q = 0; q < config.num_queues; q++) {
                queues[q].erase(
                    remove_if(queues[q].begin(), queues[q].end(),
                             [](Process* p){ return p->remaining <= 0; }),
                    queues[q].end()
                );
            }
            
            if (current_time > 0 && current_time % config.aging_check_interval == 0) {
                apply_aging();
            }
            
            if (current_time > 0 && current_time % config.boost_interval == 0) {
                apply_priority_boost();
            }
            
            if (currently_running != nullptr && currently_running->remaining > 0) {
                int running_queue = currently_running->priority;
                
                if (should_preempt(running_queue)) {
                    if (verbose_mode) {
                        cout << "Time " << current_time << ": Process P" << currently_running->pid 
                             << " preempted in Q" << running_queue << "\n";
                    }
                    queues[running_queue].push_front(currently_running);
                    currently_running = nullptr;
                    context_switches++;
                }
            }
            
            if (currently_running == nullptr || currently_running->remaining == 0) {
                int active_queue = get_highest_priority_queue();
                
                if (active_queue == -1) {
                    bool has_future = false;
                    int next_time = current_time + 1;
                    
                    for (auto& p : all_processes) {
                        if (p.arrival > current_time && p.remaining > 0) {
                            has_future = true;
                            next_time = min(next_time, p.arrival);
                        }
                    }
                    
                    if (has_future) {
                        timeline_pid.push_back(0);
                        timeline_queue.push_back(-1);
                        current_time++;
                    } else {
                        break;
                    }
                    continue;
                }
                
                currently_running = queues[active_queue].front();
                queues[active_queue].pop_front();
                
                if (!currently_running->started) {
                    currently_running->started = true;
                    currently_running->start_time = current_time;
                    context_switches++;
                } else {
                    context_switches++;
                }
            }
            
            timeline_pid.push_back(currently_running->pid);
            timeline_queue.push_back(currently_running->priority);
            
            currently_running->remaining--;
            currently_running->time_in_current_quantum++;
            total_busy_time++;
            
            update_waiting_times();
            
            if (currently_running->remaining == 0) {
                currently_running->completion = current_time + 1;
                completed++;
                if (verbose_mode) {
                    cout << "Time " << (current_time + 1) << ": Process P" << currently_running->pid 
                         << " completed in Q" << currently_running->priority << "\n";
                }
                currently_running = nullptr;
            }
            else if (config.time_quantum[currently_running->priority] > 0 && 
                     currently_running->time_in_current_quantum >= config.time_quantum[currently_running->priority]) {
                
                int old_queue = currently_running->priority;
                
                if (currently_running->priority < config.num_queues - 1) {
                    currently_running->priority++;
                    if (verbose_mode) {
                        cout << "Time " << (current_time + 1) << ": Process P" << currently_running->pid 
                             << " demoted Q" << old_queue << " -> Q" << currently_running->priority 
                             << " (Quantum exhausted)\n";
                    }
                }
                
                currently_running->time_in_current_quantum = 0;
                currently_running->time_in_queue = 0;
                queues[currently_running->priority].push_back(currently_running);
                currently_running = nullptr;
                context_switches++;
            }
            
            current_time++;
        }
        
        if (verbose_mode) {
            print_results();
        }
    }
    
    Metrics get_metrics() {
        Metrics m;
        double total_turnaround = 0.0, total_waiting = 0.0;
        int last_completion = 0;
        
        for (auto& p : all_processes) {
            int tat = p.completion - p.arrival;
            int wt = tat - p.burst;
            total_turnaround += tat;
            total_waiting += wt;
            last_completion = max(last_completion, p.completion);
        }
        
        m.avg_turnaround = total_turnaround / all_processes.size();
        m.avg_waiting = total_waiting / all_processes.size();
        m.throughput = (double)all_processes.size() / max(1, last_completion);
        m.cpu_util = 100.0 * total_busy_time / max(1, last_completion);
        m.context_switches = context_switches;
        
        return m;
    }
    
    void print_results() {
        cout << "\n========================================\n";
        cout << "MLFQ SCHEDULER RESULTS\n";
        cout << "========================================\n\n";
        
        map<int, int> queue_usage;
        for (int q : timeline_queue) {
            if (q >= 0) queue_usage[q]++;
        }
        
        cout << "Queue Usage Statistics:\n";
        for (int q = 0; q < config.num_queues; q++) {
            cout << "  Q" << q << " (" << config.algo_names[q] << "): " 
                 << queue_usage[q] << " time units ("
                 << fixed << setprecision(1) << (100.0 * queue_usage[q] / total_busy_time)
                 << "%)\n";
        }
        cout << "\n";
        
        double total_turnaround = 0.0, total_waiting = 0.0;
        int last_completion = 0;
        
        cout << "Process-wise Metrics:\n";
        cout << "PID\tArrival\tBurst\tStart\tCompletion\tTurnaround\tWaiting\n";
        cout << "---\t-------\t-----\t-----\t----------\t----------\t-------\n";
        
        for (auto& p : all_processes) {
            int tat = p.completion - p.arrival;
            int wt = tat - p.burst;
            
            cout << p.pid << "\t" << p.arrival << "\t" << p.burst << "\t"
                 << p.start_time << "\t" << p.completion << "\t\t"
                 << tat << "\t\t" << wt << "\n";
            
            total_turnaround += tat;
            total_waiting += wt;
            last_completion = max(last_completion, p.completion);
        }
        
        Metrics m = get_metrics();
        cout << "\n========================================\n";
        cout << "Overall Performance Metrics\n";
        cout << "========================================\n";
        m.display("MLFQ");
        
        print_gantt_chart();
        save_to_file();
    }
    
    void print_gantt_chart() {
        if (timeline_pid.empty()) return;
        
        cout << "\n========================================\n";
        cout << "Gantt Chart\n";
        cout << "========================================\n";
        
        int cur_pid = timeline_pid[0];
        int cur_queue = timeline_queue[0];
        int start = 0;
        
        for (size_t i = 1; i <= timeline_pid.size(); i++) {
            if (i == timeline_pid.size() || 
                timeline_pid[i] != cur_pid || 
                timeline_queue[i] != cur_queue) {
                
                if (cur_pid == 0) {
                    cout << "[Idle] " << start << "->" << i << "\n";
                } else {
                    cout << "P" << cur_pid << " [Q" << cur_queue << "] " 
                         << start << "->" << i << "\n";
                }
                
                if (i < timeline_pid.size()) {
                    cur_pid = timeline_pid[i];
                    cur_queue = timeline_queue[i];
                    start = i;
                }
            }
        }
        
        int limit = min(100, (int)timeline_pid.size());
        cout << "\nDetailed Timeline (first " << limit << " units):\n";
        cout << "Time: ";
        for (int i = 0; i < limit; i++) {
            cout << setw(3) << i;
        }
        if (timeline_pid.size() > (size_t)limit) cout << " ...";
        
        cout << "\nProc: ";
        for (int i = 0; i < limit; i++) {
            if (timeline_pid[i] == 0) cout << "  -";
            else cout << " P" << timeline_pid[i];
        }
        if (timeline_pid.size() > (size_t)limit) cout << " ...";
        
        cout << "\nQueue:";
        for (int i = 0; i < limit; i++) {
            if (timeline_queue[i] == -1) cout << "  -";
            else cout << " Q" << timeline_queue[i];
        }
        if (timeline_queue.size() > (size_t)limit) cout << " ...";
        cout << "\n";
    }
    
    void save_to_file() {
        ofstream fout("mlfq_results.txt");
        Metrics m = get_metrics();
        
        fout << "MLFQ Scheduler Results\n";
        fout << "======================\n\n";
        
        fout << "Configuration:\n";
        fout << "Number of Queues: " << config.num_queues << "\n";
        for (int i = 0; i < config.num_queues; i++) {
            fout << "Q" << i << ": " << config.algo_names[i];
            if (config.time_quantum[i] > 0) {
                fout << " (TQ=" << config.time_quantum[i] << ")";
            }
            fout << "\n";
        }
        fout << "\n";
        
        fout << "Performance Metrics:\n";
        fout << "Average Turnaround Time: " << m.avg_turnaround << "\n";
        fout << "Average Waiting Time: " << m.avg_waiting << "\n";
        fout << "Throughput: " << m.throughput << "\n";
        fout << "CPU Utilization: " << m.cpu_util << "%\n";
        fout << "Context Switches: " << m.context_switches << "\n";
        
        fout.close();
        
        if (verbose_mode) {
            cout << "\nResults saved to: mlfq_results.txt\n";
        }
    }
};


// ==================== COMPARISON SCHEDULERS ====================

// Round Robin Scheduler
class RR_Scheduler {
private:
    vector<Process> processes;
    int time_quantum;
    
public:
    RR_Scheduler(vector<Process>& procs, int tq) : processes(procs), time_quantum(tq) {}
    
    Metrics run() {
        deque<Process*> ready_queue;
        int current_time = 0;
        int completed = 0;
        int total_busy_time = 0;
        int context_switches = 0;
        size_t next_arrival = 0;
        Process* currently_running = nullptr;
        int quantum_used = 0;
        
        sort(processes.begin(), processes.end(), 
             [](const Process& a, const Process& b) { return a.arrival < b.arrival; });
        
        while (completed < processes.size()) {
            while (next_arrival < processes.size() && processes[next_arrival].arrival <= current_time) {
                ready_queue.push_back(&processes[next_arrival]);
                next_arrival++;
            }
            
            // Get next process if none running
            if (currently_running == nullptr || currently_running->remaining == 0) {
                if (ready_queue.empty()) {
                    current_time++;
                    continue;
                }
                
                currently_running = ready_queue.front();
                ready_queue.pop_front();
                quantum_used = 0;
                
                if (!currently_running->started) {
                    currently_running->started = true;
                    currently_running->start_time = current_time;
                }
                context_switches++;  
            }
            
            // Execute 1 time unit at a time
            currently_running->remaining--;
            quantum_used++;
            current_time++;
            total_busy_time++;
            
            // Check for completion
            if (currently_running->remaining == 0) {
                currently_running->completion = current_time;
                completed++;
                currently_running = nullptr;
            }
            // Check if quantum exhausted
            else if (quantum_used >= time_quantum) {
                ready_queue.push_back(currently_running);
                currently_running = nullptr;
            }
        }
        
        // Calculate metrics
        Metrics m;
        double total_tat = 0, total_wt = 0;
        int last_completion = 0;
        
        for (auto& p : processes) {
            total_tat += (p.completion - p.arrival);
            total_wt += (p.completion - p.arrival - p.burst);
            last_completion = max(last_completion, p.completion);
        }
        
        m.avg_turnaround = total_tat / processes.size();
        m.avg_waiting = total_wt / processes.size();
        m.throughput = (double)processes.size() / last_completion;
        m.cpu_util = 100.0 * total_busy_time / last_completion;
        m.context_switches = context_switches;
        
        return m;
    }
};

// FCFS Scheduler
class FCFS_Scheduler {
private:
    vector<Process> processes;
    
public:
    FCFS_Scheduler(vector<Process>& procs) : processes(procs) {}
    
    Metrics run() {
        sort(processes.begin(), processes.end(), 
             [](const Process& a, const Process& b) { return a.arrival < b.arrival; });
        
        int current_time = 0;
        int total_busy_time = 0;
        int context_switches = 0;
        
        for (auto& p : processes) {
            if (current_time < p.arrival) {
                current_time = p.arrival;
            }
            
            p.start_time = current_time;
            p.started = true;
            current_time += p.burst;
            p.completion = current_time;
            total_busy_time += p.burst;
            context_switches++;
        }
        
        Metrics m;
        double total_tat = 0, total_wt = 0;
        int last_completion = 0;
        
        for (auto& p : processes) {
            total_tat += (p.completion - p.arrival);
            total_wt += (p.completion - p.arrival - p.burst);
            last_completion = max(last_completion, p.completion);
        }
        
        m.avg_turnaround = total_tat / processes.size();
        m.avg_waiting = total_wt / processes.size();
        m.throughput = (double)processes.size() / last_completion;
        m.cpu_util = 100.0 * total_busy_time / last_completion;
        m.context_switches = context_switches;
        
        return m;
    }
};

// SJF Scheduler
class SJF_Scheduler {
private:
    vector<Process> processes;
    
public:
    SJF_Scheduler(vector<Process>& procs) : processes(procs) {}
    
    Metrics run() {
        sort(processes.begin(), processes.end(), 
             [](const Process& a, const Process& b) { return a.arrival < b.arrival; });
        
        int current_time = 0;
        int completed = 0;
        int total_busy_time = 0;
        int context_switches = 0;
        vector<bool> done(processes.size(), false);
        
        while (completed < processes.size()) {
            int shortest = -1;
            int min_burst = INT_MAX;
            
            for (size_t i = 0; i < processes.size(); i++) {
                if (!done[i] && processes[i].arrival <= current_time && processes[i].burst < min_burst) {
                    min_burst = processes[i].burst;
                    shortest = i;
                }
            }
            
            if (shortest == -1) {
                current_time++;
                continue;
            }
            
            Process& p = processes[shortest];
            p.start_time = current_time;
            p.started = true;
            current_time += p.burst;
            p.completion = current_time;
            total_busy_time += p.burst;
            done[shortest] = true;
            completed++;
            context_switches++;
        }
        
        Metrics m;
        double total_tat = 0, total_wt = 0;
        int last_completion = 0;
        
        for (auto& p : processes) {
            total_tat += (p.completion - p.arrival);
            total_wt += (p.completion - p.arrival - p.burst);
            last_completion = max(last_completion, p.completion);
        }
        
        m.avg_turnaround = total_tat / processes.size();
        m.avg_waiting = total_wt / processes.size();
        m.throughput = (double)processes.size() / last_completion;
        m.cpu_util = 100.0 * total_busy_time / last_completion;
        m.context_switches = context_switches;
        
        return m;
    }
};

// ==================== COMPARATIVE ANALYSIS ====================
void run_comparative_analysis(const vector<ProcessOriginal>& original_data, const Config& config) {
    cout << "\n========================================\n";
    cout << "COMPARATIVE ANALYSIS\n";
    cout << "========================================\n\n";
    cout << "Comparing MLFQ with other scheduling algorithms...\n\n";
    
    // Create fresh processes for each scheduler from original data
    
    // Run MLFQ
    vector<Process> mlfq_procs;
    for (auto& orig : original_data) {
        mlfq_procs.emplace_back(orig.pid, orig.arrival, orig.burst);
    }
    MLFQ_Scheduler mlfq(mlfq_procs, config);
    mlfq.run(false);
    Metrics mlfq_metrics = mlfq.get_metrics();
    
    // Run Round Robin
    vector<Process> rr_procs;
    for (auto& orig : original_data) {
        rr_procs.emplace_back(orig.pid, orig.arrival, orig.burst);
    }
    RR_Scheduler rr(rr_procs, 4);
    Metrics rr_metrics = rr.run();
    
    // Run FCFS
    vector<Process> fcfs_procs;
    for (auto& orig : original_data) {
        fcfs_procs.emplace_back(orig.pid, orig.arrival, orig.burst);
    }
    FCFS_Scheduler fcfs(fcfs_procs);
    Metrics fcfs_metrics = fcfs.run();
    
    // Run SJF
    vector<Process> sjf_procs;
    for (auto& orig : original_data) {
        sjf_procs.emplace_back(orig.pid, orig.arrival, orig.burst);
    }
    SJF_Scheduler sjf(sjf_procs);
    Metrics sjf_metrics = sjf.run();
    
    // Display comparison table
    cout << "\n========================================\n";
    cout << "PERFORMANCE COMPARISON TABLE\n";
    cout << "========================================\n\n";
    
    cout << left << setw(15) << "Algorithm" 
         << right << setw(12) << "Avg TAT" 
         << setw(12) << "Avg WT" 
         << setw(12) << "Throughput"
         << setw(12) << "CPU Util%"
         << setw(12) << "Ctx Switch" << "\n";
    cout << string(73, '-') << "\n";
    
    cout << left << setw(15) << "MLFQ"
         << right << setw(12) << fixed << setprecision(2) << mlfq_metrics.avg_turnaround
         << setw(12) << mlfq_metrics.avg_waiting
         << setw(12) << setprecision(3) << mlfq_metrics.throughput
         << setw(12) << setprecision(2) << mlfq_metrics.cpu_util
         << setw(12) << mlfq_metrics.context_switches << "\n";
    
    cout << left << setw(15) << "Round Robin"
         << right << setw(12) << rr_metrics.avg_turnaround
         << setw(12) << rr_metrics.avg_waiting
         << setw(12) << rr_metrics.throughput
         << setw(12) << rr_metrics.cpu_util
         << setw(12) << rr_metrics.context_switches << "\n";
    
    cout << left << setw(15) << "FCFS"
         << right << setw(12) << fcfs_metrics.avg_turnaround
         << setw(12) << fcfs_metrics.avg_waiting
         << setw(12) << fcfs_metrics.throughput
         << setw(12) << fcfs_metrics.cpu_util
         << setw(12) << fcfs_metrics.context_switches << "\n";
    
    cout << left << setw(15) << "SJF"
         << right << setw(12) << sjf_metrics.avg_turnaround
         << setw(12) << sjf_metrics.avg_waiting
         << setw(12) << sjf_metrics.throughput
         << setw(12) << sjf_metrics.cpu_util
         << setw(12) << sjf_metrics.context_switches << "\n";
    
    cout << "\n========================================\n";
    cout << "ANALYSIS\n";
    cout << "========================================\n";
    
    vector<pair<string, double>> tat_vals = {
        {"MLFQ", mlfq_metrics.avg_turnaround},
        {"RR", rr_metrics.avg_turnaround},
        {"FCFS", fcfs_metrics.avg_turnaround},
        {"SJF", sjf_metrics.avg_turnaround}
    };
    
    auto best_tat = *min_element(tat_vals.begin(), tat_vals.end(),
        [](auto& a, auto& b) { return a.second < b.second; });
    
    vector<pair<string, double>> wt_vals = {
        {"MLFQ", mlfq_metrics.avg_waiting},
        {"RR", rr_metrics.avg_waiting},
        {"FCFS", fcfs_metrics.avg_waiting},
        {"SJF", sjf_metrics.avg_waiting}
    };
    
    auto best_wt = *min_element(wt_vals.begin(), wt_vals.end(),
        [](auto& a, auto& b) { return a.second < b.second; });
    
    cout << "\nBest Average Turnaround Time: " << best_tat.first << " (" << best_tat.second << ")\n";
    cout << "Best Average Waiting Time: " << best_wt.first << " (" << best_wt.second << ")\n";
    
    // Save comparison to file
    ofstream fout("comparison_results.txt");
    fout << "Scheduling Algorithm Comparison\n";
    fout << "================================\n\n";
    fout << "Algorithm\tAvg TAT\tAvg WT\tThroughput\tCPU Util%\tContext Switches\n";
    fout << "MLFQ\t" << mlfq_metrics.avg_turnaround << "\t" << mlfq_metrics.avg_waiting << "\t"
         << mlfq_metrics.throughput << "\t" << mlfq_metrics.cpu_util << "\t" << mlfq_metrics.context_switches << "\n";
    fout << "RR\t" << rr_metrics.avg_turnaround << "\t" << rr_metrics.avg_waiting << "\t"
         << rr_metrics.throughput << "\t" << rr_metrics.cpu_util << "\t" << rr_metrics.context_switches << "\n";
    fout << "FCFS\t" << fcfs_metrics.avg_turnaround << "\t" << fcfs_metrics.avg_waiting << "\t"
         << fcfs_metrics.throughput << "\t" << fcfs_metrics.cpu_util << "\t" << fcfs_metrics.context_switches << "\n";
    fout << "SJF\t" << sjf_metrics.avg_turnaround << "\t" << sjf_metrics.avg_waiting << "\t"
         << sjf_metrics.throughput << "\t" << sjf_metrics.cpu_util << "\t" << sjf_metrics.context_switches << "\n";
    fout.close();
    
    cout << "\nComparison results saved to: comparison_results.txt\n";
}

// ==================== MAIN FUNCTION ====================
int main(int argc, char** argv) {
    cout << "\n";
    cout << "====================================================================\n";
    cout << "   MULTILEVEL FEEDBACK QUEUE (MLFQ) SCHEDULER   \n";
    cout << "====================================================================\n";
    
    Config config;
    if (argc >= 3 && string(argv[2]) == "-c") {
        if (config.load_from_file("config.txt")) {
            cout << "\nConfiguration loaded from config.txt\n";
        } else {
            cout << "\nUsing default configuration\n";
        }
    } else {
        cout << "\nUsing default configuration\n";
    }
    
    // Store original data separately
    vector<ProcessOriginal> original_data;
    
    string infile = (argc >= 2) ? argv[1] : "";
    
    if (infile.empty()) {
        int N;
        cout << "\nEnter number of processes: ";
        cin >> N;
        cout << "Enter PID, Arrival, Burst for each process:\n";
        for (int i = 0; i < N; i++) {
            int pid, a, b;
            cin >> pid >> a >> b;
            original_data.emplace_back(pid, a, b);
        }
    } else {
        ifstream fin(infile);
        if (!fin) {
            cerr << "Error: Cannot open file: " << infile << "\n";
            return 1;
        }
        int N;
        fin >> N;
        for (int i = 0; i < N; i++) {
            int pid, a, b;
            fin >> pid >> a >> b;
            original_data.emplace_back(pid, a, b);
        }
        fin.close();
        cout << "\nProcesses loaded from: " << infile << "\n";
    }
    
    if (original_data.empty()) {
        cerr << "Error: No processes found!\n";
        return 1;
    }
    
    // Create processes for MLFQ
    vector<Process> processes;
    for (auto& orig : original_data) {
        processes.emplace_back(orig.pid, orig.arrival, orig.burst);
    }
    
    sort(processes.begin(), processes.end(), 
         [](const Process& a, const Process& b) {
             return a.arrival < b.arrival || (a.arrival == b.arrival && a.pid < b.pid);
         });
    
    // Run MLFQ Scheduler
    MLFQ_Scheduler scheduler(processes, config);
    scheduler.run(true);
    
    // Run comparative analysis
    cout << "\nWould you like to run comparative analysis? (y/n): ";
    char choice;
    cin >> choice;
    
    if (choice == 'y' || choice == 'Y') {
        run_comparative_analysis(original_data, config);
    }
    
    cout << "\n====================================================================\n";
    cout << "                      Simulation Complete!                          \n";
    cout << "====================================================================\n\n";
    
    return 0;
}
