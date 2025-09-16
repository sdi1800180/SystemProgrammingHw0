echo "I am bash script"
# Firstly, we want the jobs that are in the queue. First we stop these. After the running jobs.
joblist=$(./jobCommander poll queued | tail -n 1 | grep -oE 'job_[0-9]+')

# Print joblist to the console
echo "Queued jobs: $joblist"

# Loop over each job in the list and run the stop command
for job in $joblist
do
    ./jobCommander stop $job
done
# Run the first command and extract the job names from the last line of output
joblist=$(./jobCommander poll running | tail -n 1 | grep -oE 'job_[0-9]+')

# Print joblist to the console
echo "Running jobs: $joblist"

# Loop over each job in the list and run the stop command
for job in $joblist
do
    ./jobCommander stop $job
done
