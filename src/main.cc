#include <iostream>
#include <fstream>
#include <queue>
#include <locale>
#include <tuple>
#include <string>
#include <chrono> 
#include <iomanip>
#include <vector>
#include "header.h"

namespace s = scheduler;

namespace scheduler {

    bool comp_proc(Process a, Process b) {
        if (a.arrival_time < b.arrival_time) return true; 
        if (a.arrival_time == b.arrival_time) return a.pid < b.pid;
        return false;
    }


    bool comp_proc_ptr(Process* a, Process* b) {
        if (a -> arrival_time < b -> arrival_time) return true;
        if (a -> arrival_time == b -> arrival_time) return a -> pid < b -> pid;
        return false;
    }


    bool comp_proc_id(Process a, Process b) { return a.pid < b.pid; }


    void set_queue_front_to_running(
        std::queue<Process*> &q, 
        RandNumAccessor &rnum, 
        int quantum = QT_UNDEF
    ) {
        if (q.size() == 0) {
            return;
        } else {
            if (q.front() -> state == TERMINATED) q.pop();
            if (q.size() > 0 && q.front() -> state == READY) {
                q.front() -> ready_to_run(rnum, quantum);
            }
        }
    }


    void do_arrival_process(
        std::vector<Process> &pv, 
        std::queue<Process*> &q, 
        int cycle
    ) {
        for (int i = 0; i < pv.size(); i++) {
            if (pv[i].arrival_time == cycle) {
                pv[i].state = READY;
                q.push(&pv[i]);
            }
        }
    }


    void do_running_process(std::queue<Process*> &q, int quantum = QT_UNDEF) {
        if (q.size() == 0) return;
        q.front() -> decr_cpu_burst();
        if (quantum != QT_UNDEF) {
            q.front() -> update_quantum_vars(quantum);
        }
    }


    void running_process_to_blocked(
        std::queue<Process*> &q, 
        std::vector<Process*> &v, 
        RandNumAccessor &rnum,
        std::vector<Process*> &queuepool, 
        int quantum = QT_UNDEF
    ) {
        if (q.size() == 0) return;
        if (q.front() -> state == RUNNING && 
            (q.front() -> remaining_cpu_burst == 0 || 
            (quantum != QT_UNDEF && q.front() -> should_preempt(quantum)))
        ) {
            if (!q.front() -> is_finished()) {
                if (q.front() -> remaining_cpu_burst == 0) {
                    q.front() -> running_to_blocked(rnum);
                    v.push_back(q.front());
                } else {
                    std::cout << "QT: " << quantum << std::endl;
                    if (quantum != QT_UNDEF) {
                        q.front() -> run_to_ready();
                        queuepool.push_back(q.front());
                    }
                    // q.push(q.front());
                }
            } 
            q.pop();
            std::sort(v.begin(), v.end(), comp_proc_ptr);
        }
    }

    void running_process_to_ready() {

    }


    void do_blocked_process(std::vector<Process*> &v) {
        for (int i = 0; i < v.size(); i++) {
            v[i] -> decr_io_burst();
        }
    }


    void blocked_process_to_ready(
        std::queue<Process*> &q, 
        std::vector<Process*> &v,
        std::vector<Process*> &queuepool
    ) {
        for (int i = 0; i < v.size(); i++) {
            if (v[i] -> remaining_io_burst == 0) {
                if (! v[i] -> is_finished()) {
                    v[i] -> blocked_to_ready();
                    // q.push(v[i]);
                    queuepool.push_back(v[i]);
                }
                v.erase(v.begin() + i);
                i--;
            }
        }
    }


    void terminate_finished_processes(std::vector<Process> &pv, int cycle) {
        for (int i = 0; i < pv.size(); i++) {
            if (pv[i].state != TERMINATED && pv[i].is_finished()) {
                pv[i].terminate_process(cycle);
                pv[i].calc_turnaround_time();
            }
        }
    }


    void update_queue_waiting_time(std::vector<Process> &pv) {
        for (int i = 0; i < pv.size(); i++) {
            if (pv[i].state == READY) pv[i].waiting_time++;
        }
    }


    void first_come_first_serve(
        std::vector<Process> pv, 
        bool should_preempt = false, 
        int quantum = QT_UNDEF
    ) {
        RandNumAccessor rnum;
        std::queue<Process*> running_queue;
        std::vector<Process*> blocked_vect;
        std::vector<Process*> join_queue_pool;
        int cycle = 0;
        int io_used_time = 0;
        int cpu_used_time = 0;
        std::cout << "--------------- FCFS ---------------\n" << std::endl;
        while (!is_procs_terminated(pv) && cycle < 100) {

            print_process_vect_simp(pv, cycle, should_preempt);

            do_blocked_process(blocked_vect);
            do_running_process(running_queue, quantum);

            blocked_process_to_ready(running_queue, blocked_vect, join_queue_pool);
            running_process_to_blocked(running_queue, blocked_vect, rnum, join_queue_pool, quantum);
            std::sort(join_queue_pool.begin(), join_queue_pool.end(), comp_proc_ptr);
            for (int i = 0; i < join_queue_pool.size(); i++) {
                running_queue.push(join_queue_pool[i]);
                join_queue_pool.erase(join_queue_pool.begin() + i);
                i--;
            }

            do_arrival_process(pv, running_queue, cycle);
            // terminating before setting queue allows terminating processes within queues 
            terminate_finished_processes(pv, cycle); 
            set_queue_front_to_running(running_queue, rnum, quantum);
            update_queue_waiting_time(pv);
            
            if (running_queue.size() > 0) cpu_used_time++;
            if (blocked_vect.size() > 0) io_used_time++;

            cycle++;
        }
        cycle--;

        std::string algo_name = should_preempt ? 
            "Round Robin" : "First Come First Served";
        std::cout 
            << "The scheduling algorithm used was " << algo_name
            << std::endl;

        print_process_vect_out(pv);
        print_summary_data(pv, cycle, cpu_used_time, io_used_time);
    }


    void roundrobin(std::vector<Process> pv) {
        int quantum = 2;
        first_come_first_serve(pv, true, quantum);
    }

    void uniprogrammed() { }

    void shortest_job_first() { }


}


int main(int argc, char** argv) {
    std::string fname;
    try {
        fname = argv[1];
    } catch (std::exception& e) {
        std::cout << "Please provide a filename as an argument." << std::endl;
        return 0;
    }
    std::vector<s::Process> procvect = s::read_file(fname);

    s::print_process_vect(procvect);
    std::sort(procvect.begin(), procvect.end(), s::comp_proc);
    std::cout << "After sorting" << std::endl;
    s::print_process_vect(procvect);

    s::first_come_first_serve(procvect);
    // s::roundrobin(procvect);
    s::uniprogrammed();
    s::shortest_job_first();


    return 0;
}