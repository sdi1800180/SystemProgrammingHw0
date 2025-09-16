#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>

#define FIFO_FILE "/tmp/pipe"
#define FIFO2_FILE "/tmp/pipe2"

// #define STRING "ls"
using namespace std;
const char* argv[] = {"./jobExecutorServer", nullptr};


//function to check if there is the txt of Server. If not , returns false
bool txtExist(){
    
    struct stat buffer;

    const char* filename = "jobExecutorServer.txt";

    if(stat(filename, &buffer) == 0) {
        cout<< "The file exists."<<endl;
        return true;
    }
    else
    {
        cout<<"The file does NOT exist"<<endl;
        return false;
    }
    
}

int main(int argc, char *argv[]) {
    
    
    if(argc == 1) {
        cout<<"Usage : ./jobCommander issueJob <job> | set concurrency <int> | stop <job_XX> | poll [running|queued] | exit"<<endl;
        return 1;
    }
    cout<<argv[1]<<endl;
    //COMMAND GENERATOR :
    if(strcmp(argv[1] , "issueJob")!=0 && strcmp(argv[1] , "exit")!=0 && strcmp(argv[1],"set")!=0 && strcmp(argv[1],"stop")!=0 && strcmp(argv[1],"poll")!=0) {
       cout<<"Usage : [issueJob <job> | exit | set concurrency <x> | poll [running|queued] | stop job__XX]"<<endl;
        return 1;
    }
    string command ;
    string args;
    for (int i=1; i<argc; ++i) { //we want to seperate the input from second arg and after
        args += argv[i];
        if(i< argc - 1 ){       // we add a space in order to pass the command in the right way 
            args += " ";        
        }
    }
    command = args;       //after all this is the command that we ll pass into the server

    // NOW SERVER RUNNER : 
    //sleep(1) in order to wait the server create the txt. Ok I do not find another way cause of deadline...
    sleep(1);
    if(!txtExist())
    {
        cout<<"I must create the JobExecutorServer process"<<endl;
        pid_t pid = fork();
        if(pid == 0) {
            // Child process
            cout<<"I am going to run the jobExecutorServer"<<endl;
            execv("./jobExecutorServer", const_cast<char* const*>(argv));
        } else if (pid > 0) {
            // Parent process
            cout<<"child process"<<endl;
            sleep(1);
        } else {
            // Error handling for fork failure
            cerr << "Fork failed!" << endl;
            return 1;
        }
    } else {
        cout<<"jobExecutorServer.txt exists"<<endl;
    }
    //THE SERVER IS ON SO NOW OPENNIN THE PIPE 
    
    int fd,txt_fd;
    const char * myfifo = FIFO_FILE;
    
     

    /* create the FIFO (named pipe) */
    mkfifo(myfifo, 0666);

    /* open the PID txt file for reading */
    
    
    //OPEN THE TEXT FILE CREATED BY SERVER IN ORDER TO FIND THE SERVER PID
    
    txt_fd = open ("jobExecutorServer.txt", O_RDONLY);
    char buffer[128]; // assuming the PID won't be larger than 128 characters

    //WE WANT TO read the PID of Server
 
    ssize_t txt_read = read(txt_fd, buffer, sizeof(buffer) - 1);
    if(txt_read == -1){
        cout << "Error reading file: " << strerror(errno) << "\n";
        close(txt_fd);
        return 1;
    }

    buffer[txt_read] = '\0'; //null-terminate the buffer
    close(txt_fd);

    pid_t server_pid = static_cast<pid_t>(std::atoi(buffer));
    
    if(server_pid==0) {
        cout<<"Error converting stringPid to pid"<<endl;
        return 1;
    }
    
    cout<<"From Commander : Server process id is "<<server_pid<<endl;
    
    //NOW   SEND THE SIGUSR1 SIGNAL TO SERVER  
    
    kill(server_pid, SIGUSR1);
    

    const char* charCommand = command.c_str();


    /* write STRING to the FIFO */
    fd = open(myfifo, O_WRONLY);
    write(fd, charCommand, strlen(charCommand)+1);
    close(fd);

    /* remove the FIFO */
    unlink(myfifo);
    sleep(1); 
    //sleep one second in order to wait the server to create the pipe and write into it 
    
    //in case that the command is set concurrency, the server does not need to send soemthing back
    if(strcmp(argv[1],"set")) 
    {
        const char* myfifo2 = FIFO2_FILE;
        char buf2[1000];
        int f=open(myfifo2,O_RDONLY);
        

        read(f,buf2,1000);
        close(f);


        string mess(buf2);
        cout<<"Received from Server "<<mess<<endl;
        

    }

    return 0;
}
