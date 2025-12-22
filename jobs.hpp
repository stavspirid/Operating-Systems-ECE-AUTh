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
    bool is_current;
    bool notified;
};

// Global Job Table
extern std::vector<Job> jobTable;
// Global Job Counter
extern int nextJobId;

/**
 * Add a new job to the job table
 * 
 * @param pgid Process Group ID of the job
 * @param command Command string
 * @param state Initial state of the job
 * @param pids Vector of PIDs associated with the job
 */
void addJob(pid_t pgid, const std::string& command, JobState state, const std::vector<pid_t>& pids);

/**
 * Remove a job from the job table
 * 
 * @param jobId ID of the job to remove
 */
void removeJob(int jobId);

/**
 * Get a job by its job ID
 * 
 * @param jobId ID of the job
 * @return Pointer to the Job, or nullptr if not found
 */
Job* getJob(int jobId);

/**
 * Get a job by its process group ID
 * 
 * @param pgid Process Group ID
 * @return Pointer to the Job, or nullptr if not found
 */
Job* getJobByPgid(pid_t pgid);

/**
 * Update the state of a job
 * 
 * @param jobId ID of the job to update
 * @param newState New state to set
 */
void updateJobState(int jobId, JobState newState);

/**
 * Print all jobs in the job table (in Bash format)
 */
void printJobs();

/**
 * Get the most recent job (by job ID)
 * Used for 'fg' and 'bg' commands
 * @return Pointer to the most recent Job, or nullptr if no jobs exist
 */
Job* getMostRecentJob();

/**
 * Mark a job as the current job running in the foreground
 * 
 * @param jobId ID of the job to mark as current
 */
void markJobAsCurrent(int jobId);

#endif // JOBS_HPP