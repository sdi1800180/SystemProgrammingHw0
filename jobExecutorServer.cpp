#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <queue>
#include <vector>
#include <cstring>
#include <cstdlib>


#define FIFO_FILE "/tmp/pipe"
#define FIFO2_FILE "/tmp/pipe2"

#define BUFFER_SIZE 100
using namespace std;

void executeJob(std::vector<char*> argv);


//declare the Job struct
class Job {
    private :
        string jobID;
        string job;
        int queuePosition;
        string triplet;
        int job_ID;
        pid_t pid_for_kill;
    public:
        Job(int id, string& name, int position) : job_ID(id), job(name), queuePosition(position) {}
        
        void decreasePosition() {
            queuePosition--;
        }

        string get_job(){
            return job;
        }

        void set_triplet(){
            set_jobID();
            triplet = "<" + get_jobID() + "," + job + "," + to_string(queuePosition) + ">";
        }

        string get_triplet(){
            return triplet;
        }

        void set_jobID(){
            jobID = "job_" + to_string(job_ID);
        }

        string get_jobID(){
            return jobID;
        }

        void set_Pid(pid_t p){
            pid_for_kill = p;
        }
        
        pid_t get_Pid(){
            return pid_for_kill;
        }
};


//declare globally the queue 
queue<Job> JobQueue;
queue<Job> OnRunningQueue;

//declare globally the counters 
int jobs_on_running=0;
int received_jobs=0;
int jobsInQueue=0;
bool termination = false;
const char * myfifo2 = FIFO2_FILE;
int concurrency = 1; 



vector<char *> arg_seperator(const string& command)
{
    // Tokenize the command into arguments using strtok
    vector<char *> argv;
    char *token = strtok(const_cast<char *>(command.c_str()), " ");
    while (token != nullptr) {
        argv.push_back(token);
        token = strtok(nullptr, " ");
    }
    argv.push_back(nullptr);

    // cout<<"from function argv is ";
    for (int i = 0; argv[i] != nullptr; i++) {
        cout << argv[i] << " ";
    }
    // cout <<"DEBUUUUUUUUUUUUUUUUUUUUUGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGGG"<< endl;

    return argv;
}
// the above function be implemented in order to help me to monitor the queue sometimes
void QueuePrinting(queue<Job> Q)
{
    
    if (Q.empty())
    {
        cout<<"Empty"<<endl;
    }
    queue<Job> tmp = Q;
    while(!tmp.empty()){
        Job job = tmp.front();
        cout<<" "<<job.get_triplet()<<"pid is "<<job.get_Pid()<<"|    ";
        tmp.pop();
    }
    
    cout<<endl;
}
// this function is for solution of returned message to Commander
string PrintingQueueForPipe(queue<Job> Q){
    
    string res ;
    if (Q.empty())
    {
        res = "Empty";
        return res;
    }

    while (!Q.empty())
    {
        res += Q.front().get_triplet()+" ";
        
        Q.pop();
    }

    return res;
}
//Generate the above function in order to call it after the "stop" command execution
string QueueUpdating(string id, queue<Job> &Q)
{
    // cout<<"message from function queueUpdating!!!!!! "<<id<<endl;
    queue<Job> tmp;
    string triplet ;
    bool flag = false;
    while(!Q.empty()){
        
        string ID = Q.front().get_jobID();
        
        if(ID != id)
            tmp.push(Q.front());
        else
        {
            flag = true;
            triplet = Q.front().get_triplet();
        }
        
        Q.pop();
    }

    Q = tmp;
    if (flag) return triplet;
    else return "not found";
}

//for updating the position in Queue if a job poped for running
void QueuePositionsUpdating()
{
  
    queue<Job> tempQueue;
    int currentPosition = 1; 

    while(!JobQueue.empty()){
        Job currentJob = JobQueue.front();
        JobQueue.pop();

        currentJob.decreasePosition();
        currentJob.set_triplet();

        tempQueue.push(currentJob);
        currentPosition++;
    }

    swap(JobQueue, tempQueue);

    
}
// Searching the job_XX in the running Queue , that the user input at the terminal. Pass the Queue by value not by pointer. So ensure that the real Queue will not be changed
pid_t JobRunSearching(string jobID,queue<Job> Q)
{
    while (!Q.empty())
    {
        if (jobID == Q.front().get_jobID())
        {
            return Q.front().get_Pid();
        }
        else
        {
            Q.pop();
        }
        
    }

    return -1;
    
}

// This function is called ONLY when a job terminated normally (you will see it in the waitpid in the handler if sigchld signal)
void RemoveFromRun(pid_t id, queue<Job> &Q){
    queue<Job> tmp;
    // cout<<" EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE"<<endl;
    
    while(!Q.empty()){
        
        pid_t ID = Q.front().get_Pid();
        
        if(ID != id)
            tmp.push(Q.front());

        
        Q.pop();
    }

    Q = tmp;    
}

// Searching function but for searching Jobs in the queue that we hold jobs before running them.
string JobQueueSearching(string jobID,queue<Job> Q)
{
    while (!Q.empty())
    {
        if (jobID == Q.front().get_jobID())
        {
            return Q.front().get_jobID();
        }
        else
        {
            Q.pop();
        }
    }

    return "job not found";
}

//function for handling the SIGCHLD signal
void handler_sigchld(int sig)
{ 
    if(sig == SIGCHLD)
    {
        int status;
        pid_t terminated_id = waitpid(-1,&status,WNOHANG) ; //set to -1, it means to wait for any child process
        if (terminated_id>0)
        {
            // here we want to check if the job terminated by "stop <job__XX>". If not then call the removeFromRun 
            // in case that terminated by the user with stop, we call another function. You will see in the code far below
            if(WIFEXITED(status))
                RemoveFromRun(terminated_id, OnRunningQueue); // we pass the terminated_id <pid_t> . This is the process ID that is terminated normally

        }
        
        
        
        // cout<<"by handler_sigchld "<<getpid()<<endl;
        
        jobs_on_running--; //Cause a job that was running before, now is done

        //MAYBE WHILE INSTEAD OF EMPTY 
        if(!JobQueue.empty() && jobs_on_running < concurrency)
        {
           
            string argument = JobQueue.front().get_job();
            vector<char*> argj = arg_seperator(argument);
            
            /* HERE WE PUSH THE JOB THAT WE WILL RUN INTO A SECOND QUEUE FOR STOP/POLL REASONS */
            OnRunningQueue.push(JobQueue.front());
            /* AFTER TAHT WE POP THE JOB FROM FIRST QUEUE */
            
            JobQueue.pop();
            jobsInQueue--;
            QueuePositionsUpdating();
            jobs_on_running++; //Cause a new job is going to run after that !
            // cout<<endl<<"New job is about to run now so current jobs right now are "<<jobs_on_running<<endl;
            // cout<<"BEFORE CALL OF THE EXECUTE JOB "<<argj[0]<<" "<<argj[1]<<endl;
            executeJob(argj);
        }

        


    }
    else{
        cout<<"Received unexpected signal"<<endl;
    }
}


//function for execute the Job from the queue
void executeJob(vector<char*> argv){
    
    pid_t pid_c = fork();
    

    if (pid_c == 0)
    {

        //cout<<"!!!!!!!!!I am child and this is my PID before execv !!!!!!!!!!"<<getpid()<<endl;
        cout<<"THE JOB IS "<<argv[0]<<endl;
        execvp(argv[0], &argv[0]); //Execute the job !
        while(1){
            int a = 2;
        }
        // cout<<"Failed to execute received command with code " <<e<<endl;

        //_exit(1); // o ensure that any file descriptors opened by the parent process are not closed.
    }
    else if (pid_c < 0)
    {
            cout << "Failed to fork process" << endl;
    }
    else
    {
        int status;
        // cout << "Parent process" << endl;
        // cout<<"!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!Child pid is "<<pid_c<<endl;
        OnRunningQueue.back().set_Pid(pid_c);
        
        cout<<" These jobs are running right now ! ";
        QueuePrinting(OnRunningQueue);
        
        signal(SIGCHLD, handler_sigchld); //HERE THE SERVER MUST BE INFORM THAT THE CHILD PROCESS THAT CREATED EARLIER HAS BEEN TERINATED      
        
    }
}



//function for executin the command from the Commander 
void executeCommand(const char *cmd)
{

    char *first_arg = strtok(const_cast<char *>(cmd), " "); // seperate first arg

    char *rest_args = strtok(NULL, ""); // strtok still points the same


    // HERE IS THE COMMAND HANDLING
    if (strcmp(first_arg, "issueJob") == 0)     /////////////  IN CASE OF COMMAND IS A JOB
    {
        cout<<"current jobs before executin "<<jobs_on_running<<endl;
        string jobName(rest_args);
        Job currentJob(received_jobs,jobName,jobsInQueue);
        jobsInQueue++;
        currentJob.set_triplet();
        cout<<"triplet is "<<currentJob.get_triplet()<<endl;
        received_jobs++; 
        JobQueue.push(currentJob);

        
        /* OPEN and WRITE INTO FIFO */
        mkfifo(myfifo2, 0666);
        int fd2 = open(myfifo2, O_WRONLY);
        const char* char_triplet = currentJob.get_triplet().c_str();
        
        write(fd2, currentJob.get_triplet().c_str(), currentJob.get_triplet().length()+1);
        close(fd2);
        unlink(myfifo2);
        /*     CLOSE and REMOVE FIFO     */
        
        
        cout<<"front Job = "<<JobQueue.front().get_job()<<endl;
        
        
        
        if (jobs_on_running < concurrency)  //if concurrency allows it
        {
                     
            jobs_on_running++;
            jobsInQueue--;      
                       
            string argum = JobQueue.front().get_job();
            vector<char*> argv = arg_seperator(argum);
            
            
            /* HERE WE PUSH THE JOB THAT WE WILL RUN INTO A SECOND QUEUE FOR STOP/POLL REASONS */
            OnRunningQueue.push(JobQueue.front());
            /* AFTER TAHT WE POP THE JOB FROM QUEUE */

            JobQueue.pop();
            QueuePositionsUpdating();
            // if (JobQueue.empty())
            // {
            //     cout<<"queue is empty!!!!!!!!!!!!!"<<endl;
            // }
           executeJob(argv);
        }else {
            cout<<endl<<"Concurrency does not allow me to run this job . Thank you"<<endl;
        }
    }
    else if (strcmp(first_arg, "exit") == 0)
    {
        cout << "Server is going to be turned off" << endl;
        termination=true;
        // MESSAGE THAT WILL BE PASSED INTO THE PIPE FOR COMMANDER :
        string mess ="jobExecutorServer terminated";
        /* OPEN and WRITE INTO FIFO */
        mkfifo(myfifo2, 0666);
        int fd2 = open(myfifo2, O_WRONLY);
        // const char* char_triplet = triplet.c_str();

        write(fd2, mess.c_str(), mess.length()+1);
        close(fd2);
        unlink(myfifo2);
        /*     CLOSE and REMOVE FIFO     */
        
        
    }
    else if (strcmp(first_arg, "set") == 0)
    {
        
        char *second_arg = strtok(rest_args, " ");
    
        char *concurrency_arg = strtok(NULL, " ");

        int concurrency_level = atoi(concurrency_arg);

        cout<<"Concurrency now is "<<concurrency_level<<endl;

        concurrency = concurrency_level;

        while (!JobQueue.empty() && jobs_on_running < concurrency)
        {
            //char* const argj[] = {const_cast<char*>(JobQueue.front().get_job().c_str()), NULL };
            string argument = JobQueue.front().get_job();
            vector<char*> argj = arg_seperator(argument);
            //"/bin/sh", "-c",
            /* HERE WE PUSH THE JOB THAT WE WILL RUN INTO A SECOND QUEUE FOR STOP/POLL REASONS */
            OnRunningQueue.push(JobQueue.front());
            /* AFTER TAHT WE POP THE JOB FROM FIRST QUEUE */
            
            JobQueue.pop();
            jobsInQueue--;
            QueuePositionsUpdating();
            jobs_on_running++; //Cause a new job is going to run after that !
            cout<<endl<<"New job is about to run now so current jobs right now are "<<jobs_on_running<<endl;
            cout<<"BEFORE CALL OF THE EXECUTE JOB "<<argj[0]<<" "<<argj[1]<<endl;
            executeJob(argj);
        }
        

    }
    else if (strcmp(first_arg, "stop") == 0)
    {
        string id(rest_args);
        pid_t found_id = JobRunSearching(id,OnRunningQueue);
        
        if (found_id != -1)
        {
            cout<<"Found id :"<<found_id<<endl;
            //so here , if this job running, must be terminated. So we send SIGKILL in the found_id process.
            kill(found_id,SIGKILL);
            // Cause of the above, we must remove this job from OnRunningQueue. 
            // The function RemoveFromRun is only for the case that the job has been terminated NORMALLY by itself
            string triplet = QueueUpdating(id,OnRunningQueue);

            // cout<<"Execution is over so now current jobs are : "<<jobs_on_running<<endl;
            // cout<<"Queue right now is "<<endl;
            // QueuePrinting(JobQueue);
            // cout<<"On Running ";
            // QueuePrinting(OnRunningQueue);

            /* OPEN and WRITE INTO FIFO */
            mkfifo(myfifo2, 0666);
            int fd2 = open(myfifo2, O_WRONLY);
            // const char* char_triplet = triplet.c_str();
            
            write(fd2, triplet.c_str(), triplet.length()+1);
            close(fd2);
            unlink(myfifo2);
            /*     CLOSE and REMOVE FIFO     */




            // sleep(0.5);
            // waitpid(pid_id,&st, WEXITED);
        }
        else
        {
            cout<<"This job is not running right now"<<endl;
            string str_found_id = JobQueueSearching(id, JobQueue);
            string triplet;
            if(str_found_id != "job not found")
            {
                cout<<"But it has been found in Queue and will be removed: "<<str_found_id<<endl;
                triplet  = QueueUpdating(id,JobQueue);
                

            }
            else{
                triplet = "job not found";
            }
            /* OPEN and WRITE INTO FIFO */
            mkfifo(myfifo2, 0666);
            int fd2 = open(myfifo2, O_WRONLY);
            write(fd2, triplet.c_str(), triplet.length()+1);
            close(fd2);
            unlink(myfifo2);
            /*     CLOSE and REMOVE FIFO     */



        }
                
    }
    else if (strcmp(first_arg, "poll") == 0)
    {
        string message;
        string rest(rest_args);
        if (rest == "running")
        {
            message = PrintingQueueForPipe(OnRunningQueue);
            
        }
        else if (rest == "queued")
        {
            message = PrintingQueueForPipe(JobQueue);
        }
        
        
        /* OPEN and WRITE INTO FIFO */
        mkfifo(myfifo2, 0666);
        int fd2 = open(myfifo2, O_WRONLY);
        // const char* char_triplet = triplet.c_str();

        write(fd2, message.c_str(), message.length()+1);
        close(fd2);
        unlink(myfifo2);
        /*     CLOSE and REMOVE FIFO     */


    }
    
}




//function for hanling the SIGUSR1 signal from the Commander 
void sig_usr1(int signo)
{
    if (signo == SIGUSR1)
    {
        int fd;
        const char *myfifo = FIFO_FILE;
        char buf[BUFFER_SIZE];
        cout << "Server is expecting a command" << endl;
        /* read from the FIFO */
        fd = open(myfifo, O_RDONLY);
        
        read(fd, buf, BUFFER_SIZE);
        
         
        cout<<"Received "<<buf<<endl;
        executeCommand(buf);
        close(fd);
    }
    else
    {
        cout << "Received unexpected signal" << endl;
    }
}






// function for creation of jobExecutorServer.txt file :
void createTxt()
{

    pid_t pid = getpid();                                                           // get the current process id in order
    cout << "From Server : I am going to write " << pid << " into the txt" << endl; // to insert it into the txt later
    string stringpid = to_string(pid);                                              // cast it to string here

    int fd = open("jobExecutorServer.txt", O_RDWR | O_CREAT, 0666);

    if (fd == -1)
    {
        cout << "Error on creation of file" << endl;
        exit(1);
    }

    ssize_t bytesWritten = write(fd, stringpid.c_str(), stringpid.length()); // writtin the casted pid into the txt

    if (bytesWritten == -1)
    {
        cout << "Failed to write PID into the txt file" << endl;
    }

    close(fd);
}


// function for deleting the jobExecutorServer.txt file :
void deleteTxt()
{
    unlink("jobExecutorServer.txt");
    cout<<"jobExecutorServer.txt deleted"<<endl;
}



int main()
{

    received_jobs = 0;
    createTxt();

    //pid_t pid = getpid();
    //a while loop : when the user inserts exit termination be false.
    while (!termination)
    {
        signal(SIGUSR1, sig_usr1); /* install SIGUSR1 handler */            
    }
    //delete the jobExecutorServer.txt
    deleteTxt();
    return 0;
}





