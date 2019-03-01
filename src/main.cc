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

    void set_queue_first_to_running(std::queue<Process*> &q, RandNumAccessor &rnum) {
        if (q.size() == 0) {
            return;
        } else {
            // print_process_one_line(*q.front());
            if (q.front() -> state == TERMINATED) q.pop();
            if (q.size() > 0 && q.front() -> state == READY) {
                q.front() -> ready_to_run(rnum);
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


    void do_running_process(std::queue<Process*> &q) {
        if (q.size() == 0) return;
        q.front() -> decr_cpu_burst();
    }


    void finished_running_process_to_blocked(
        std::queue<Process*> &q, 
        std::vector<Process*> &v, 
        RandNumAccessor &rnum
    ) {
        if (q.size() == 0) return;
        if (q.front() -> state == RUNNING 
            && q.front() -> remaining_cpu_burst == 0
        ) {
            if (!q.front() -> is_finished()) {
                q.front() -> running_to_blocked(rnum);
                v.push_back(q.front());
            }
            q.pop();
            std::sort(v.begin(), v.end(), comp_proc_ptr);
        }
    }


    void do_blocked_process(std::vector<Process*> &v) {
        for (int i = 0; i < v.size(); i++) {
            v[i] -> decr_io_burst();
        }
    }


    void finished_blocked_process_to_ready(
        std::queue<Process*> &q, 
        std::vector<Process*> &v
    ) {
        for (int i = 0; i < v.size(); i++) {
            // print_process_one_line(*v[i]);
            if (v[i] -> remaining_io_burst == 0) {
                if (! v[i] -> is_finished()) {
                    v[i] -> blocked_to_ready();
                    q.push(v[i]);
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


    void fcfs(std::vector<Process> pv, RandNumAccessor &rnum) {
        std::queue<Process*> running_queue;
        std::vector<Process*> blocked_vect;
        int cycle = 0;
        int io_used_time = 0;
        int cpu_used_time = 0;
        std::cout << "---------- FCFS ----------\n" << std::endl;
        while (!is_procs_terminated(pv)) {

            print_process_vect_simp(pv, cycle);

            do_blocked_process(blocked_vect);
            do_running_process(running_queue);

            finished_blocked_process_to_ready(running_queue, blocked_vect);
            finished_running_process_to_blocked(running_queue, blocked_vect, rnum);            

            do_arrival_process(pv, running_queue, cycle);
            // terminating before setting queue allows terminating processes within queues 
            terminate_finished_processes(pv, cycle); 
            set_queue_first_to_running(running_queue, rnum);
            update_queue_waiting_time(pv);
            
            if (running_queue.size() > 0) cpu_used_time++;
            if (blocked_vect.size() > 0) io_used_time++;

            cycle++;
        }
        cycle--;
        std::cout 
            << "The scheduling algorithm used was First Come First Served" 
            << std::endl;
        std::sort(pv.begin(), pv.end(), comp_proc);
        print_process_vect_out(pv);
        print_summary_data(pv, cycle, cpu_used_time, io_used_time);
    }


    void rr_scheduler() { }

    void uniprogrammed_scheduler() { }

    void sjf_scheduler() { }


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

    s::RandNumAccessor rnum;
    s::fcfs(procvect, rnum);

    return 0;
}