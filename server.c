/* **
COMP 8567 - Advanced Systems Programming
Instructors: Dr. Boufama and Dr. Yacoub
Project Work
Title: Simulation of FTP protocol

Submitted By:
    Parvendra Singh   Student ID: 110080746
    Simrandeep Singh  Student ID: 110077588

FTP Server
*/

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <limits.h>

// Declaring methods
void child(int);

void cwd(int, char **);
void cdup(int, char **);
void rein(int, char **);
void quit(int, char **);
void port(int, char **);
void retr(int, char **);
void stor(int, char **);
void appe(int, char **);
void rnfr(int, char **);
void rnto(int, char **);
void dele(int, char **);
void rmd(int, char **);
void mkd(int, char **);
void list(int, char **);
void abor(int, char **);
void sta(int, char **);

void sendResponse(char *);
void recieveFile(char *, int);
void sig_handler(int);
int checkUserLoggedIn();
int checkDataConnectionOpen();
// Declaring global variables
int userLoggedIn = 0, dataConnectionOpen = 0, renameFromExecuted = 0;
int sd, client, portNumber, status;
char dataConnection[50];
char message[255];
int n, pid, i;
char *command;
char renameFrom[PATH_MAX];
struct sockaddr_in servAdd; // client socket address

struct TransferProcess // Structure for storing pid's of processes
{
    pid_t pid;
    char fileName[100];
    char transferType[20];
    char transferStatus[20];
};
struct TransferProcess transferProcesses[1000];
int transferProcessIndex = 0;

int main(int argc, char *argv[])    
{
    if (argc == 1 || argc == 3 || argc > 4)    // server will require 2 or 4 agruments only
    {
        printf("Call model: %s <Port Number> [ -d <path> ]\n", argv[0]);
        exit(0);                                // exiting if desired arguments not received
    }
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        fprintf(stderr, "Cannot create socket\n");
        exit(1);
    }
    sd = socket(AF_INET, SOCK_STREAM, 0);
    servAdd.sin_family = AF_INET;
    servAdd.sin_addr.s_addr = htonl(INADDR_ANY);
    sscanf(argv[1], "%d", &portNumber);
    servAdd.sin_port = htons((uint16_t)portNumber);

    bind(sd, (struct sockaddr *)&servAdd, sizeof(servAdd));
    listen(sd, 5);
    if (argc > 2)
    {
        if (strcasecmp(argv[2], "-d") == 0 && argv[3] != NULL) // if '-d' present in arguments
        {
            chdir(argv[3]);         // working directory of server is changed to 4th Argument
        }
        else
        {
            printf("Call model: %s <Port Number> [ -d <path> ]\n", argv[0]); //else exit
            exit(0);
        }
    }
    while (1)       // infinite loop to accept multiple clients
    {
        printf("Ready to accept client connection .. \n");
        client = accept(sd, NULL, NULL);
        printf("Client connected.\n");

        if (!fork())            // parent process is forked and returned back(line 109) to accept to next client
            child(client);         //child function will be executed for child process for client

        close(client);
        waitpid(0, &status, WNOHANG);
    }
}

void child(int sd)      // this function will handle all client requests 
{                   // it will read commands from clients 
    char **args = malloc(10 * sizeof(char *));  // and call particular functions to execute it
    signal(SIGINT, sig_handler); // Register ^C signal handler
    signal(SIGTSTP, sig_handler);  //Register ^Z signal handler
    sendResponse("Connection Established.");
    while (1)                   // infinite loop to accept all commands from client
        if ((n = read(sd, message, 255)))
        {
            message[n] = '\0';
            printf("Client Request: %s\n", message);
            command = strtok(message, " \n\0");
            i = 0;
            do
            {
                args[i++] = strtok(NULL, " \n\0");
            } while (args[i - 1] != NULL);
            i--;
            if (!strcasecmp(command, "USER")) // All Commands read and compared using if-else
            {
                userLoggedIn = 1;
                sendResponse("230 User logged in, proceed.");
            }
            else if (!strcasecmp(command, "CWD"))  
            {
                cwd(i, args);
            }
            else if (!strcasecmp(command, "CDUP"))
            {
                cdup(i, args);
            }
            else if (!strcasecmp(command, "REIN"))
            {
                rein(i, args);
            }
            else if (!strcasecmp(command, "QUIT"))
            {
                quit(i, args);
            }
            else if (!strcasecmp(command, "PORT"))
            {
                port(i, args);
            }
            else if (!strcasecmp(command, "RETR"))
            {
                retr(i, args);
            }
            else if ((!strcasecmp(command, "STOR")))
            {
                stor(i, args);
            }
            else if (!strcasecmp(command, "APPE"))
            {
                appe(i, args);
            }
            else if (!strcasecmp(command, "REST"))
            {
                sendResponse("200 Command Okay. Currently, there are no ongoing processes to restart");
            }
            else if (!strcasecmp(command, "RNFR"))
            {
                rnfr(i, args);
            }
            else if (!strcasecmp(command, "RNTO"))
            {
                rnto(i, args);
            }
            else if (!strcasecmp(command, "ABOR"))
            {
                abor(i, args);
            }
            else if (!strcasecmp(command, "DELE"))
            {
                dele(i, args);
            }
            else if (!strcasecmp(command, "RMD"))
            {
                rmd(i, args);
            }
            else if (!strcasecmp(command, "MKD"))
            {
                mkd(i, args);
            }
            else if (!strcasecmp(command, "PWD"))
            {
                char *buf = malloc(PATH_MAX);
                sendResponse(getcwd(buf, PATH_MAX));
                free(buf);
            }
            else if (!strcasecmp(command, "LIST"))
            {
                list(i, args);
            }
            else if (!strcasecmp(command, "STAT"))
            {
                sta(i, args);
            }
            else if (!strcasecmp(command, "NOOP"))
            {
                sendResponse("200 Command okay.");
            }
            else
            {
                sendResponse("502 Command not implemented.");
            }
        }
}

// Definition of all the declared methods
void sendResponse(char *message) //method will write message in socket 
{                                // from where client will read and process
    write(client, message, strlen(message));
    write(client, "\n", 1);
}

void cwd(int i, char **args) // CWD will change the working directory of server
{                            
     if (!checkUserLoggedIn()) // it will first check if user is already logged
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    if (chdir(args[0]) == 0)
        sendResponse("200 Working directory changed"); //Successfull
    else
        sendResponse("550 Requested action not taken."); // Error response
}

void cdup(int i, char **args) // CDUP- Server's working directory is changed to its parent
{
     if (!checkUserLoggedIn())
        return;
    if (chdir("..") == 0)
        sendResponse("200 Working directory changed"); //Successful
    else
        sendResponse("550 Requested action not taken."); // Error Response
}

void rein(int i, char **args)  //REIN resets the connection between client and server
{
     if (!checkUserLoggedIn()) // User must be logged in
        return;
    unlink(dataConnection);
    userLoggedIn = 0;
    dataConnectionOpen = 0;
}

void quit(int i, char **args) //QUIT will close descriptor of socket to quit the server connection
{
    sendResponse("221 Service closing control connection. Logged out if appropriate.");
    close(sd);
    exit(0);
}

void port(int i, char **args) //PORT will open FIFO or namedPipe to transfer files
{                             //between client and server
    if (!checkUserLoggedIn()) 
        return;              //User must be logged in
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    strcpy(dataConnection, "/tmp/");
    strcat(dataConnection, args[0]);
    unlink(dataConnection);
    if (mkfifo(dataConnection, 0777) != 0)
    {
        sendResponse("425 Can't open data connection.");
        return;
    }
    chmod("dataConnection", 0777); 
    dataConnectionOpen = 1;
    sendResponse("200 Command okay.");
}

void retr(int i, char **args)  // RETR will send files to client requested by name
{                             // in current working directory of server
    int fd1, fd2;
    char buffer[100];
    long int n1, counter;
    if (!checkUserLoggedIn() || !checkDataConnectionOpen())
        return;               // User must be logged in and Named Pipe must be open
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    if ((fd1 = open(dataConnection, O_WRONLY)) == -1) // fd1 FD writing in FIFO from where client will read
    {
        sendResponse("425 Can't open data connection.");
        return;
    }
    if ((fd2 = open(args[0], O_RDONLY)) == -1) //fd2 FD reading file named <args[0]>
    {
        sendResponse("550 Requested action not taken. File unavailable (e.g., file not found, no access).");
        return;
    }
    sendResponse("125 Data connection already open; transfer starting.");
    pid_t pid = fork();     // child process will transfer

    struct TransferProcess process; // file in progress is stored in structure for STAT command
    process.pid = pid;              // PID, file Name, Transfer Status, Transfer Type is stored
    strcpy(process.fileName, args[0]);
    strcpy(process.transferStatus, "In Progress");
    strcpy(process.transferType, "Download");

    transferProcesses[transferProcessIndex] = process;
    transferProcessIndex = (transferProcessIndex + 1) % 1000;
    if (!pid)
    {
        while ((n1 = read(fd2, buffer, 100)) > 0) //reading 100 bytes from buffer file
        {
            if (write(fd1, buffer, n1) != n1)  // writing 100 bytes in buffer 
            {
                sendResponse("552 Requested file action aborted. Can not write");
                exit(0);
            }
        }
        if (n1 == -1)
        {
            sendResponse("552 Requested file action aborted. can't read");
            exit(0);
        }
        close(fd1); // closed fd1, fd2 for child process
        close(fd2);
        sendResponse("250 Requested file action okay, completed.");
        exit(0);
    }
    close(fd1);  // closed fd1, fd2 for parent
    close(fd2);
}

void stor(int i, char **args)   // STOR will store file sent by client
{
    if (!checkUserLoggedIn() || !checkDataConnectionOpen())
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    recieveFile(args[0], 0);
}

void appe(int i, char **args)  // APPE will append into existing file sent by client
{
    if (!checkUserLoggedIn() || !checkDataConnectionOpen())
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    recieveFile(args[0], 1);
}

void recieveFile(char *fileName, int append) // STOR and APPE will use this function to perform write
{

    int fd1, fd2;
    char buffer[100];
    long int n1;

    if ((fd1 = open(dataConnection, O_RDONLY)) == -1)
    {
        sendResponse("425 Can't open data connection.");
        return;
    }
    int fileMode = O_CREAT | O_WRONLY;
    if (append)
        fileMode = fileMode | O_APPEND;
    else
        fileMode = fileMode | O_TRUNC;

    if ((fd2 = open(fileName, fileMode, 0700)) == -1)
    {
        sendResponse("550 Requested action not taken. Can't create file.");
        return;
    }

    pid_t pid = fork();

    struct TransferProcess process;
    process.pid = pid;
    strcpy(process.fileName, fileName);
    strcpy(process.transferStatus, "In Progress");
    strcpy(process.transferType, "Upload");

    transferProcesses[transferProcessIndex] = process;
    transferProcessIndex = (transferProcessIndex + 1) % 1000;
    if (!pid)
    {
        sendResponse("125 Data connection already open; transfer starting.");
        while ((n1 = read(fd1, buffer, 100)) > 0)
        {
            if (write(fd2, buffer, n1) != n1)
            {
                sendResponse("552 Requested file action aborted. Can't write to file.");
                close(fd1);
                close(fd2);
                exit(0);
            }
        }
        if (n1 == -1)
        {
            sendResponse("552 Requested file action aborted. Can't read from Pipe.");
            close(fd1);
            close(fd2);
            exit(0);
        }
        close(fd1);
        close(fd2);
        sendResponse("250 Requested file action okay, completed.");
        exit(0);
    }
    close(fd1);
    close(fd2);
}

void rnfr(int i, char **args) // RNFR will take fileName from client to Rename it
{
    if (!checkUserLoggedIn())
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    strcpy(renameFrom, args[0]);
    renameFromExecuted = 1;
    sendResponse("200 Command okay.");
}

void rnto(int i, char **args) // RNTO should be executed after RNFR to rename file name
{
    if (!checkUserLoggedIn())
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    if (!renameFromExecuted) // will check if RNFR is executed before running RNTO
    {
        sendResponse("503 Bad sequence of commands.");
        return;
    }
    if (rename(renameFrom, args[0]) == 0)
        sendResponse("250 Requested file action okay, completed.");
    else
        sendResponse("553 Requested action not taken.");
}

void dele(int i, char **args)  //DELE will delete the file requested by client
{
    if (!checkUserLoggedIn()) // User must be logged in
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    if (remove(args[0]) == 0)
        sendResponse("250 Requested file action okay, completed.");
    else
        sendResponse("550 Requested action not taken.");
}

void rmd(int i, char **args) // RMD will remove directory requested by client
{
    if (!checkUserLoggedIn()) //user must be logged in before executing this command
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    if (rmdir(args[0]) == 0)
        sendResponse("250 Requested file action okay, completed.");
    else
        sendResponse("550 Requested action not taken.");
}

void mkd(int i, char **args) // MKD will create new directory
{
    if (!checkUserLoggedIn()) //user must be logged in before executing this command
        return;
    if (i != 1)
    {
        sendResponse("501 Syntax error in parameters or arguments.");
        return;
    }
    if (mkdir(args[0], 0700) == 0)
        sendResponse("250 Requested file action okay, completed.");
    else
        sendResponse("550 Requested action not taken.");
}

void list(int i, char **args) // LIST will list all files and directory in the current directory
{
    if (!checkUserLoggedIn() || !checkDataConnectionOpen()) //user must be logged in and NamedPipe must be opened
        return;                                             // before executing this command
    sendResponse("150 File status okay; about to open data connection.\n");
    sendResponse("125 Data connection already open; transfer starting.\n");
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    if (d)
    {
        char response[9999] = "--------  \n";
        while ((dir = readdir(d)) != NULL)
        {
            strcat(response, dir->d_name);
            strcat(response, "\n");
        }
        closedir(d);
        sendResponse(response);
    }
    sendResponse("250 Requested file action okay, completed.");
}

void sig_handler(int signum)    // Signal ^C and ^Z is handled
{
    printf("Received ^C / ^Z signal, closing all client connections\n");
    rein(0, NULL);  // REIN and QUIT is executed for all the clients(child processes)
    quit(0, NULL);
}

void sta(int i, char **args) // STAT will return current processes on the server
{                           // stored in structure TransferProcesses
    if (!checkUserLoggedIn()) // user must be logged in
        return;
    char response[1000] = "Data Transfer Processes\n";
    strcat(response, "Process ID       Status        Transfer Type   File Name\n");
    strcat(response, "----------       --------      -------------   ---------\n");
    for (int i = 0; i < transferProcessIndex; i++)
    {
        char buf[100];
        if (kill(transferProcesses[i].pid, 0) == 0)    // send 0 signal to check if process exists and completed
            strcpy(transferProcesses[i].transferStatus, "Completed");
        else
            strcpy(transferProcesses[i].transferStatus, "In Progress");

        sprintf(buf, "%-17d %-13s %-16s %s \n", transferProcesses[i].pid, transferProcesses[i].transferStatus, transferProcesses[i].transferType, transferProcesses[i].fileName);
        strcat(response, buf);
    }
    strcat(response, "\n");
    sendResponse(response);
}
void abor(int i, char **args) // ABOR- All the processes in structure will be KILLED using SIGTERM
{                             
    if (!checkUserLoggedIn())
        return;
    for (int i = 0; i < transferProcessIndex; i++)
    {

        if (!kill(transferProcesses[i].pid, 0))
            kill(transferProcesses[i].pid, SIGTERM);
    }
    sendResponse("200 Command OKAY. All ongoing transfers Aborted!");
}

int checkUserLoggedIn() // Fuction will check if User is logged in
{
    if (!userLoggedIn)
    {
        sendResponse("530 Not logged in.");
        return 0;
    }
    return 1;
}
int checkDataConnectionOpen() // Function will check if Named Pipe is opened
{
    if (!dataConnectionOpen)
    {
        sendResponse("425 Can't open data connection.");
        return 0;
    }
    return 1;
}
