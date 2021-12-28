#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "LineParser.h"
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

#define HISTORY_LENGTH 10

int debug = 0;
int p = 0;
char *history[HISTORY_LENGTH] = {};
int pos = 0;

void addHistory(char *toAdd)
{
    int index = pos % HISTORY_LENGTH;
    if (pos >= HISTORY_LENGTH)
    {
        char *toClean = history[index];
        free(toClean);
    }
    history[index] = toAdd;
    pos++;
}

void printHistory()
{
    for (int j = 0; j < HISTORY_LENGTH; j++)
    {
        if (history[j])
        {
            printf("%s\n", history[j]);
        }
    }
}

void freeHistory()
{
    for (int j = 0; j < HISTORY_LENGTH; j++)
    {
        if (history[j])
        {
            free(history[j]);
        }
    }
}

int execute(cmdLine *pCmdLine)
{
    int in = 0;
    int out = 1;
    char *command = pCmdLine->arguments[0];
    pid_t pid = -2;
    pid_t pid2 = -2;
    int status;
    int fd[2];

    if (strlen(command) > 1 && strncmp(command, "cd", 2) == 0)
    {
        if (debug == 1)
        {
            fprintf(stderr, "\nprosses: %d ,is executing %s\n", getpid(), pCmdLine->arguments[0]);
        }
        int err = chdir(pCmdLine->arguments[1]);
        if (err == -1)
        {
            perror("Error: ");
            exit(-1);
        }
    }
    else if (strlen(command) == strlen("history") && strncmp(command, "history", strlen("history")) == 0)
    {
        printHistory();
    }
    else
    //**command that a child should execute**//
    {
        /**check if pipe needed**/
        if (pCmdLine->next != NULL)
        {
            p = 1;
            if (pipe(fd) == -1)
            {
                perror("at pipe");
                _exit(1);
            }
        }

        /**create child1**/
        int pid1 = fork();

        if (pid1 == -1)
        {
            perror("at fork");
            _exit(1);
        }

        /**at child1**/
        if (pid1 == 0)
        {
            if (debug == 1)
            {
                fprintf(stderr, "\nprosses: %d ,is executing %s\n", getpid(), pCmdLine->arguments[0]);
            }

            /**check if redirection is needed**/
            if (pCmdLine->inputRedirect)
            {

                in = open(pCmdLine->inputRedirect, S_IRUSR);
                dup2(in, 0);
                close(in);
            }
            if (pCmdLine->outputRedirect)
            {

                out = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT, 0666);
                dup2(out, 1);
                close(out);
            }

            //**if using pipe redirect the fd's**//
            if (p == 1)
            {
                fclose(stdout);
                int d = dup(fd[1]);
                if (d < 0)
                {
                    perror("at dup1");
                    _exit(1);
                }
                close(fd[1]);
            }

            //**execute child1 command**//
            int check = execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if (check == -1)
            {
                perror("Error: ");
                _exit(1);
            }
        }

        /**check in parent of child1 if we are using pipe**/
        if (p == 1)
        {
            /**close write end of parent**/
            close(fd[1]);

            /**create child2**/
            pid2 = fork();
            if (pid2 == -1)
            {
                perror("at fork");
                _exit(1);
            }
        }

        if (pid2 == 0)
        {
            /**get next command for child2 to exec**/
            pCmdLine = pCmdLine->next;

            /**check if redirection is needed**/
            if (pCmdLine->inputRedirect)
            {
                in = open(pCmdLine->inputRedirect, S_IRUSR);
                dup2(in, 0);
                close(in);
            }
            if (pCmdLine->outputRedirect)
            {
                out = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT, 0777);
                dup2(out, 1);
                close(out);
            }

            //** set stdin fd num to our fd[0] **//
            fclose(stdin);
            int d = dup(fd[0]);
            if (d < 0)
            {
                perror("at dup2");
                _exit(1);
            }
            close(fd[0]);

            /**execute command**/
            if (execvp(pCmdLine->arguments[0], pCmdLine->arguments) == -1)
            {
                perror("at exec");
                _exit(1);
            }
        }
        else
        {
            /**we are in the parent**/
            /**check if pipe is being used**/
            if (p == 1)
            {
                close(fd[0]);
            }

            if (waitpid(pid1, NULL, 0) == -1)
            {
                perror("error on waitpid(): child 1");
                _exit(1);
            }
            else if (p == 1)
            {
                if (waitpid(pid2, NULL, 0) == -1)
                {
                    perror("error on waitpid(): child 2");
                    _exit(1);
                }
            }
        }
    }
    p = 0;
    return 1;
}

int main(int argc, char const *argv[])
{
    int err = 0;
    char userInput[2048];
    char buffPath[PATH_MAX];
    cmdLine *userInStruct;
    getcwd(buffPath, PATH_MAX);
    int terminate = 0;
    int i = 0;

    for (i = 0; i < argc; i++)
    {
        if (strlen(argv[i]) > 1 && strcmp(argv[i], "-d") == 0)
        {
            debug = 1;
        }
    }

    while (terminate != 1)
    {

        getcwd(buffPath, PATH_MAX);
        fprintf(stdout, "\nmyShell: %s$ ", buffPath);
        fgets(userInput, 2048, stdin);

        if (strlen(userInput) > 1 && strncmp(userInput, "quit", 4) == 0)
        {
            if (debug == 1)
            {
                fprintf(stderr, "\nprosses: %d ,is executing quit\n", getpid());
            }

            exit(0);
        }
        char *temp = malloc(2048);

        if (strlen(userInput) > 0 && strncmp(userInput, "!", 1) == 0)
        {

            int index = atoi(&userInput[1]);
            if (index > 9 || history[index] == NULL)
            {
                printf("invalid index after '!' \n");
                freeHistory();
                exit(1);
            }

            userInStruct = parseCmdLines(history[index]);
            strncpy(temp, history[index], 2048);
        }
        else
        {
            strncpy(temp, userInput, 2048);
            userInStruct = parseCmdLines(userInput);
        }

        addHistory(temp);
        err = execute(userInStruct);

        if (err == -1)
        {
            exit(1);
        }
        freeCmdLines(userInStruct);
    }
    freeHistory();
    return 0;
}
