#define _GNU_SOURCE

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// GLOBAL VARIABLES
int bgFlag = 0;
int fgOnly = 0;
int bgArray[50];
int bgCounter = 0;
int numArgs = 0;
char *inFile = NULL;
char *outFile = NULL;
char *args[513] = {0};
int statusCode = 0;
pid_t spawnpid;
int allProcesses[1000];
int processCounter = 0;

// FUNCTIONS
void handle_SIGTSTP();
void userInput();
void varExpansion();
void checkAmpersand();
void checkCommands();
void otherCommands();
void bgCheck();
void childProcess();

// SIGNAL HANDLERS
struct sigaction SIGINT_action;
struct sigaction SIGTSTP_action;

int main()
{
    // IGNORE ^C
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // CATCH ^Z AND REDIRECT TO HANDLER
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    while (1)
    {
        bgFlag = 0;
        userInput();
        checkCommands();

        // RESET FOR NEXT COMMAND
        memset(args, 0, sizeof args);
        inFile = NULL;
        outFile = NULL;
        numArgs = 0;
    }

    return 0;
}

void handle_SIGTSTP()
{
    char *message;
    int messageSize = 0;

    switch (fgOnly)
    {
    case 0:
        message = "\nEntering Foreground-Only Mode (& is Now Ignored)\n";
        messageSize = 50;
        fgOnly = 1;
        break;

    case 1:
        message = "\nExiting Foreground-Only Mode\n";
        messageSize = 30;
        fgOnly = 0;
    }

    write(STDOUT_FILENO, message, messageSize);
    fflush(stdout);

    // RE-PROMPT
    message = ": ";
    write(STDOUT_FILENO, message, 2);
    fflush(stdout);
}

void userInput()
{
    char *token;
    char *line = NULL;

    printf(": ");
    fflush(stdout);

    size_t size = 0;
    getline(&line, &size, stdin);

    // CHECK LINE FOR VARIABLE EXPANSION
    char *isExpansion = strstr(line, "$$");
    if (isExpansion)
    {
        char pid[6];
        sprintf(pid, "%d", getpid());
        varExpansion(line, "$$", pid);
    }

    // FIRST TOKEN
    token = strtok(line, "  \n");

    while (token != NULL)
    {
        if (strcmp(token, "<") == 0)
        {
            // NEXT TOKEN HAS INFILE NAME
            token = strtok(NULL, " \n");
            inFile = strdup(token);

            token = strtok(NULL, " \n");
        }

        else if (strcmp(token, ">") == 0)
        {
            // NEXT TOKEN HAS OUTFILE NAME
            token = strtok(NULL, " \n");
            outFile = strdup(token);

            token = strtok(NULL, " \n");
        }

        else
        {
            // STORE IN ARGUMENT ARRAY
            args[numArgs] = strdup(token);

            token = strtok(NULL, " \n");
            numArgs++;
        }
    }
}

// ADAPTED FROM STACK OVERFLOW
// https://stackoverflow.com/questions/32413667/replace-all-occurrences-of-a-substring-in-a-string-in-c/32413923
void varExpansion(char *line, const char *find, const char *replacement)
{
    char buffer[2048] = {0};
    char *insert = &buffer[0];
    const char *tmp = line;

    while (1)
    {
        // FIND ALL $$ IN LINE
        const char *varFound = strstr(tmp, find);

        // NO MORE OCCURENCES
        if (varFound == NULL)
        {
            // SAVE UPDATED ARGUMENTS TO BUFFER AT START
            strcpy(insert, tmp);
            break;
        }

        // COPY LINE BEFORE $$
        memcpy(insert, tmp, varFound - tmp);
        insert += varFound - tmp;

        // COPY PID
        memcpy(insert, replacement, strlen(replacement));
        insert += strlen(replacement);

        // ADJUST POINTER
        tmp = varFound + strlen(find);
    }

    // SAVE UPDATED ARGUMENTS TO LINE
    strcpy(line, buffer);
}

void checkCommands()
{
    // BLANK LINE OR COMMENT
    if (args[0] == NULL || !(strncmp(args[0], "#", 1)))
    {
        // DO NOTHING
    }

    else
    {
        // EXIT COMMAND
        if (strcmp(args[0], "exit") == 0)
        {
            if (processCounter == 0)
            {
                exit(0);
            }

            // KILL EACH PROCESS BEFORE EXITING
            else
            {
                int i;
                for (i = 0; i < processCounter; i++)
                {
                    kill(allProcesses[i], SIGTERM);
                    exit(1);
                }
            }
        }

        // STATUS COMMAND
        else if (strcmp(args[0], "status") == 0)
        {
            // FROM LECTURE
            if (WIFEXITED(statusCode))
            {
                printf("Exit Value %i\n", WEXITSTATUS(statusCode));
                fflush(stdout);
            }

            else
            {
                printf("Terminated By Signal %i\n", statusCode);
                fflush(stdout);
            }
        }

        // CD COMMAND
        else if (strcmp(args[0], "cd") == 0)
        {
            // NO ARGUMENTS
            if (args[1] == NULL)
            {
                // GO TO DIRECTORY SPECIFIED IN HOME ENVIRONMENT VARIABLE
                chdir(getenv("HOME"));
            }

            else
            {
                if (chdir(args[1]) == -1)
                {
                    printf("Directory Not Found.\n");
                    fflush(stdout);
                }
            }
        }

        else
        {
            checkAmpersand();
            otherCommands();
        }
    }
    bgCheck();
}

void checkAmpersand()
{
    // IS & LAST ARGUMENT
    if (strcmp(args[numArgs - 1], "&") == 0)
    {
        // CHECK IF FOREGROUND ONLY MODE
        if (fgOnly == 0)
        {
            bgFlag = 1;
        }
        // REMOVE & FROM ARGUMENT ARRAY
        args[numArgs - 1] = NULL;
    }
}

void otherCommands()
{
    spawnpid = fork();

    // KEEP TRACK OF ALL PROCESSES
    allProcesses[processCounter] = spawnpid;
    processCounter++;

    // FORK ERROR
    if (spawnpid < 0)
    {
        printf("Fork Error\n");
        fflush(stdout);
        exit(1);
    }

    // CHILD
    else if (spawnpid == 0)
    {
        childProcess();
    }

    // PARENT
    else
    {
        if (bgFlag == 0)
        {
            waitpid(spawnpid, &statusCode, 0);
            if (WIFSIGNALED(statusCode))
            {
                printf("Terminated by Signal %d\n", WTERMSIG(statusCode));
            }
        }

        else if (bgFlag == 1)
        {
            // KEEP TRACK OF BACKGROUND PROCESSES
            bgArray[bgCounter] = spawnpid;
            bgCounter++;
            waitpid(spawnpid, &statusCode, WNOHANG);

            bgFlag = 0;
            printf("Background pid is %d\n", spawnpid);
            fflush(stdout);
        }
    }
}

void bgCheck()
{
    int i;

    // CHECK IF ANY BACKGROUND PROCESSES HAVE RETURNED
    for (i = 0; i < bgCounter; i++)
    {
        if (waitpid(bgArray[i], &statusCode, WNOHANG) > 0)
        {
            if (WIFSIGNALED(statusCode))
            {
                printf("Background pid %d is Done - Terminated by Signal %d\n", bgArray[i], WTERMSIG(statusCode));
                fflush(stdout);
            }
            if (WIFEXITED(statusCode))
            {
                printf("Background pid %d is Done - Exit Value %d\n", bgArray[i], WEXITSTATUS(statusCode));
                fflush(stdout);
            }
        }
    }
}

void childProcess()
{
    int fd;

    // FOREGROUND PROCESSES CAN BE INTERUPTED BY ^C
    if (bgFlag == 0)
    {
        SIGINT_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &SIGINT_action, NULL);
    }

    // INPUT FILE REDIRECT
    if (inFile != NULL)
    {
        fd = open(inFile, O_RDONLY);

        if (fd == -1)
        {
            printf("Cannot Open %s for Input\n", inFile);
            fflush(stdout);
            exit(1);
        }

        else if (dup2(fd, 0) == -1)
        {
            perror("Cannot Redirect to Input File\n");
            fflush(stdout);
            exit(1);
        }

        close(fd);
    }

    // OUTPUT FILE REDIRECT
    if (outFile != NULL)
    {
        fd = open(outFile, O_WRONLY | O_CREAT | O_TRUNC, 0755);

        if (fd == -1)
        {
            printf("Cannot Open %s for Output\n", outFile);
            fflush(stdout);
            exit(1);
        }

        else if (dup2(fd, 1) == -1)
        {
            printf("Cannot Redirect to Output File\n");
            fflush(stdout);
            exit(1);
        }
        close(fd);
    }

    // REDIRECT STANDARD INPUT TO /dev/null
    if (inFile == NULL && bgFlag == 1)
    {
        fd = open("/dev/null", O_RDONLY);
        if (fd == -1)
        {
            printf("Cannot Redirect Input\n");
            fflush(stdout);
            exit(1);
        }

        else if (dup2(fd, 0) == -1)
        {
            printf("Cannot Redirect Input\n");
            fflush(stdout);
            exit(1);
        }
        close(fd);
    }

    // REDIRECT STANDARD OUTPUT TO /dev/null
    if (outFile == NULL && bgFlag == 1)
    {
        fd = open("/dev/null", O_WRONLY);
        if (fd == -1)
        {
            printf("Cannot Redirect Output\n");
            fflush(stdout);
            exit(1);
        }

        else if (dup2(fd, 1) == -1)
        {
            printf("Cannot Redirect Output\n");
            fflush(stdout);
            exit(1);
        }
        close(fd);
    }

    // EXECUTE COMMAND
    if (execvp(args[0], args))
    {
        fprintf(stderr, "Command Not Found: %s\n", args[0]);
        fflush(stdout);
        exit(1);
    }
}