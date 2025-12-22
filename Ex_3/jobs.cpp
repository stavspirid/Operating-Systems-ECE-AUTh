#include "jobs.hpp"
#include <iostream>
#include <sys/wait.h>
#include <signal.h>

// Add a new job to the job table
void addJob(pid_t pgid, const std::string& command, JobState state, const std::vector<pid_t>& pids) {
    Job job;
    job.jobId = nextJobId++;
    job.pgid = pgid;
    job.command = command;
    job.state = state;
    job.pids = pids;
    job.is_current = true;  // Mark as current (most recent)
    job.notified = false;   // Not yet notified about completion
    
    // Mark all other jobs as not current
    for (auto& j : jobTable) {
        j.is_current = false;
    }
    
    jobTable.push_back(job);
    
    std::cout << "[" << job.jobId << "] " << pgid << std::endl;
}

// Remove a job from the job table
void removeJob(int jobId) {
    for (auto it = jobTable.begin(); it != jobTable.end(); ++it) {
        if (it->jobId == jobId) {
            jobTable.erase(it);
            
            // Update current job marker
            if (!jobTable.empty()) {
                jobTable.back().is_current = true;
            }
            return;
        }
    }
}

// Get a job by job ID
Job* getJob(int jobId) {
    for (auto& job : jobTable) {
        if (job.jobId == jobId) {
            return &job;
        }
    }
    return nullptr;
}

// Get a job by process group ID
Job* getJobByPgid(pid_t pgid) {
    for (auto& job : jobTable) {
        if (job.pgid == pgid) {
            return &job;
        }
    }
    return nullptr;
}

// Update job state
void updateJobState(int jobId, JobState newState) {
    Job* job = getJob(jobId);
    if (job) {
        job->state = newState;
    }
}

// Print all jobs in bash format
void printJobs() {
    if (jobTable.empty()) {
        return;
    }
    
    for (const auto& job : jobTable) {
        // Skip DONE jobs - they'll be printed by check_job_status_changes()
        if (job.state == DONE) {
            continue;
        }
        
        std::string stateStr;
        switch (job.state) {
            case RUNNING:
                stateStr = "Running";
                break;
            case STOPPED:
                stateStr = "Stopped";
                break;
            case DONE:
                stateStr = "Done";
                break;
        }
        
        // Print in bash format: [jobId]+ State    command
        std::cout << "[" << job.jobId << "]";
        if (job.is_current) {
            std::cout << "+";
        } else {
            std::cout << "-";
        }
        std::cout << " " << stateStr;
        
        // Add padding for alignment
        int padding = 12 - stateStr.length();
        for (int i = 0; i < padding; i++) {
            std::cout << " ";
        }
        
        std::cout << job.command;
        
        // Add " &" suffix for running background jobs
        if (job.state == RUNNING) {
            std::cout << " &";
        }
        
        std::cout << std::endl;
    }
}

// Get the most recent job (for fg/bg with no arguments)
Job* getMostRecentJob() {
    if (jobTable.empty()) {
        return nullptr;
    }
    
    // Find the job marked as current
    for (auto& job : jobTable) {
        if (job.is_current) {
            return &job;
        }
    }
    
    // Fallback to last job
    return &jobTable.back();
}

// Mark a job as current (when bringing to foreground)
void markJobAsCurrent(int jobId) {
    for (auto& job : jobTable) {
        job.is_current = (job.jobId == jobId);
    }
}