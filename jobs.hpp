#ifndef JOBS_HPP
#define JOBS_HPP

#include <string>
#include <vector>
#include <sys/types.h>

// Enumeration for job states
enum JobState {
    RUNNING,
    STOPPED,
    DONE
};

// Structure representing a job
struct Job
{
    int jobId;
    pid_t pgid;
    std::string command;
    JobState state;
    std::vector<pid_t> pids;
};

// Global Job Table
std::vector<Job> jobTable;
// Global Job Counter
int nextJobId = 1;


#endif // JOBS_HPP