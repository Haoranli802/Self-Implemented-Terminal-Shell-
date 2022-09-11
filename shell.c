//Name1: Weiyu Hao, 59955246
//Name2: Haoran Li, 80921159

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>
#include <fcntl.h>
#include <stdbool.h>

//#define Running 1;
//#define Stopped 0;
pid_t pid;
int fgwait = 0;
struct Status
{
    int pid;
    int jid;
    char status[15];
    char commands[80];
}allStatus[5];

void runCommand();
int childCount = 0;
int bg = 0;
int jid_count = 0;
char copyOfCommandLine2[80];
int indicator;

int inputParser(char** argcs, char* commandline){
    int token_count = 0;
    if((argcs[0] = strtok(commandline," \n\t\r")) == NULL){
        return -1;  // nothing inputted
    }
    token_count ++;
    while((argcs[token_count] = strtok(NULL, " \n\t\r")) != NULL){
        token_count ++;
    }
    if(argcs[token_count - 1][0] == '&'){
        argcs[token_count - 1] = NULL;
        return 1;
    }
    argcs[token_count] = NULL;
    return 0;
}
int searchStatusIndex(int pid)
{
    int i;
    for(i=0;i<5;i++)
    {
        if(allStatus[i].pid==pid)
        {
            return i;
        }
    }
    return -1;
}

int searchPid(int jid)
{
    int i;
    for(i=0;i<childCount;i++)
    {
        if(allStatus[i].jid == jid)
        {
            return allStatus[i].pid;
        }
    }
    return -1;
}

void showJob(){
    int i;
    for(i=0;i<childCount;i++)
    {
        printf("[%i] (%i) %s %s",
               allStatus[i].jid + 1, allStatus[i].pid, allStatus[i].status,
               allStatus[i].commands);
    }
}

int checkInputOutputFile(char* argcs)
{
    //整体没啥问题，就是<总是有小问题，不知道为什么
    int i;
    char* token;
    char s[2] = " ";
    token = strtok(argcs,s);
    int indexCount = 0;
    int index = -1;
    mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
    while(token!=NULL)
    {
        indexCount++;
        //printf("token1: %s",token);
        if(strcmp(token,"<")==0)
        {
            if(index==-1)
            {
                index = indexCount;
            }
            token = strtok(NULL, s);

            char fileName[80];
            strcpy(fileName,token);
            //printf("last char: %c",fileName[strlen(fileName)-1]);
            if(strcmp(&fileName[strlen(fileName)-1],"\n")==0){
                //printf("terminate");
                fileName[strlen(fileName)-1]='\0';
            }
            /*
            else{
                strcat(fileName,"\n");
            }
           */

            //printf("token<: %s",token);

            int inFileID = open (fileName, O_RDONLY, mode);

            if(inFileID<0)
            {
                printf("Couldn't open the input file %s.\n",fileName);
                exit(0);
            }

            dup2(inFileID, STDIN_FILENO);
            //close(inFileID);
        }
        if(strcmp(token,">")==0)
        {
            if(index==-1)
            {
                index = indexCount;
            }

            //mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
            token = strtok(NULL, s);

            char fileName[80];
            strcpy(fileName,token);
            if(strcmp(&fileName[strlen(fileName)-1],"\n")==0){
                //printf("terminate");
                fileName[strlen(fileName)-1]='\0';
            }
            /*
            else{
                strcat(fileName,"\n");
            }
            */
            //printf("token>: %s",token);
            int outFileID = open (fileName, O_CREAT|O_WRONLY|O_TRUNC, mode);
            dup2(outFileID, STDOUT_FILENO);
            //close(outFileID);
        }
        if(strcmp(token,">>")==0)
        {
            if(index==-1)
            {
                index = indexCount;
            }
            //printf("In >>");
            //mode_t mode = S_IRWXU | S_IRWXG | S_IRWXO;
            token = strtok(NULL, s);
            char fileName[80];
            strcpy(fileName,token);

            if(strcmp(&fileName[strlen(fileName)-1],"\n")==0){
                //printf("terminate");
                fileName[strlen(fileName)-1]='\0';
            }


            int outFileID = open(fileName, O_CREAT|O_APPEND|O_WRONLY, mode);
            dup2(outFileID, STDOUT_FILENO);
            //sortclose(outFileID);
        }
        //printf("token2: %s\n",token);
        token = strtok(NULL, s);
        //printf("token3: %s\n",token);
    }
    return index;

}

void sigchild_handler(int signum){
    //printf("in Sigchild,signum: %i\n",signum);

    pid_t child_pid;
    int status;
    //printf("4\n");
    while((child_pid = waitpid(pid, &status, WNOHANG|WUNTRACED)) > 0){
        //printf("in while\n");
        if(WIFSTOPPED(status)){
            //printf("in stopped\n");
            int i;
            for(i = 0; i < childCount; i++){
                if(allStatus[i].pid == child_pid){
                    strcpy(allStatus[i].status, "Stopped");
                }
            }
        }
        else if(WIFSIGNALED(status) || WIFEXITED(status)){
            struct Status temp;
            int count = 0;
            int i;
            for(i = 0; i < childCount; i++){
                if(allStatus[i].pid != child_pid){
                    temp.jid = allStatus[i].jid;
                    temp.pid = allStatus[i].pid;
                    strcpy(temp.commands, allStatus[i].commands);
                    strcpy(temp.status, allStatus[i].status);
                    allStatus[count] = temp;
                    count ++;
                }
            }
            childCount = count;
        }
        else{
            fprintf(stdout, "%s\n", "waitpid error");
        }
    }


}


void signalHandlerC(int signum)
{
    // for testing
    //printf("inside handler handling signal %i\n",signum);

    struct Status temp;
    int count = 0;
    int i;
    for(i = 0; i < childCount; i++){
        if(allStatus[i].pid != pid){
            temp.jid = allStatus[i].jid;
            temp.pid = allStatus[i].pid;
            strcpy(temp.commands, allStatus[i].commands);
            strcpy(temp.status, allStatus[i].status);
            allStatus[count] = temp;
            count ++;
        }
    }
    childCount = count;
    kill(pid,SIGINT);
}


void signalHandlerZ(int signum)
{
    // for testing
    //if(getpid()==0)
    //
    //这里的正确逻辑应该是，发个SIGTSTP，然后让parent不要再wait child了，应该就可以继续prompt了
    //pid_t pgid = getpgid(pid);
    //printf("z pid: %i\n",pid);
    kill(pid,SIGTSTP);

    int i;
    for(i = 0; i < childCount; i++){
        if(allStatus[i].pid == pid){
            strcpy(allStatus[i].status, "Stopped");
            return;
        }
    }
    if(pid != getpid()) {
        struct Status temp;
        temp.pid = pid;
        temp.jid = jid_count;
        strcpy(temp.status, "Stopped");
        strcpy(temp.commands, copyOfCommandLine2);
        allStatus[childCount] = temp;
        childCount++;
    }
}


extern void unix_error(const char *msg){

}

void fg(char *argcs[]){

    if(*argcs[0]=='%')
    {
        int pid;
        pid = searchPid(atoi(argcs[0]+1)-1);
        if(pid == -1)
        {
            printf("Given job_id|pid not found.\n");
        }
        else
        {
            //这里是发个继续的signal，然后让parent开始wait，应该就是fg了
            //pid_t pgid = getpgid(pid);
            fgwait = 1;
            //printf("Inside fg, pid = %i\n",pid);
            kill(pid,SIGCONT);

            int status;
            //printf("5\n");
            waitpid(pid, &status, WUNTRACED);
            //printf("stopwait");

        }



    }
    else
    {
        int pid = atoi(*argcs);
        bool found = false;
        int i;
        for(i=0;i<5;i++)
        {
            if(allStatus[i].pid == pid)
            {
                found = true;
            }
        }
        if(found) {
            fgwait = 1;
            kill(pid, SIGCONT);
            int status;
            waitpid(pid, &status, WUNTRACED);
            /*
            int status;
            if(waitpid(pid, &status, WUNTRACED) < 0) {
                if (WIFSTOPPED(status)) {
                    return;
                }
            }
             */
        }
        else
        {
            printf("Given job_id|pid not found.\n");
        }
    }
}

void buildIn_bg(char *argcs[]){
    if(*argcs[0]=='%')
    {
        int pid;
        pid = searchPid(atoi(argcs[0]+1)-1);
        if(pid == -1)
        {
            printf("Given job_id|pid not found.\n");
        }
        else
        {
            //这应该是给stopped的发个继续的signal，并且不wait，就是bg了
            kill(pid,SIGCONT);
            int i = searchStatusIndex(pid);
            strcpy(allStatus[i].status,"Running");
        }
    }
    else
    {
        int pid = atoi(*argcs);
        bool found = false;
        int i;
        for(i=0;i<5;i++)
        {
            if(allStatus[i].pid == pid)
            {
                found = true;
            }
        }
        if(found)
        {
            kill(pid,SIGCONT);
            int i = searchStatusIndex(pid);
            strcpy(allStatus[i].status,"Running");
        }
        else
        {
            printf("Given job_id|pid not found.\n");
        }
    }

}

void buildIn_kill(char* argcs[]){
    if(*argcs[0]=='%')
    {
        int pid;
        //printf("%i",atoi(argcs[0]+1));
        pid = searchPid(atoi(argcs[0]+1)-1);
        if(pid == -1)
        {
            printf("Given job_id|pid not found.\n");
        }
        else
        {
            struct Status temp;
            int count = 0;
            int i;
            for(i = 0; i < childCount; i++){
                if(allStatus[i].pid != pid){
                    temp.jid = allStatus[i].jid;
                    temp.pid = allStatus[i].pid;
                    strcpy(temp.commands, allStatus[i].commands);
                    strcpy(temp.status, allStatus[i].status);
                    allStatus[count] = temp;
                    count ++;
                }
            }
            childCount = count;
            kill(pid,SIGCONT);
            kill(pid,SIGINT);
            int status;
            //printf("6\n");
            waitpid(pid,&status,0);

        }
    }
    else
    {
        int pid = atoi(*argcs);
        bool found = false;
        int i;
        for(i=0;i<5;i++)
        {
            if(allStatus[i].pid == pid)
            {
                found = true;
            }
        }
        if(found)
        {
            struct Status temp;
            int count = 0;
            int i;
            for(i = 0; i < childCount; i++){
                if(allStatus[i].pid != pid){
                    temp.jid = allStatus[i].jid;
                    temp.pid = allStatus[i].pid;
                    strcpy(temp.commands, allStatus[i].commands);
                    strcpy(temp.status, allStatus[i].status);
                    allStatus[count] = temp;
                    count ++;
                }
            }
            childCount = count;
            kill(pid,SIGCONT);
            kill(pid,SIGINT);
            int status;
            waitpid(pid,&status,0);
        }
        else
        {
            printf("Given job_id|pid not found.\n");
        }
    }
}

void cd(char* argc[]){
    const char *homedir = getenv("HOME");
    if(argc[1] == NULL){
        chdir(homedir);
    }
    else{
        chdir(argc[1]);
    }
}

int main(int argc, char *argv[]) {


    signal(SIGTSTP, signalHandlerZ);
    signal(SIGINT, signalHandlerC);
    //signal(SIGCONT, signalContHandler);

    while(1){
        char commandline[80]; // max 80
        char copyOfCommandLine[80];
        char* argcs[80];
        printf("prompt> ");
        fgets(commandline, 80, stdin);
        strcpy(copyOfCommandLine, commandline);
        strcpy(copyOfCommandLine2, commandline);
        int IOindex = -2;
        
        if((strcmp(commandline, "\n") == 0)){
            continue;
        }
        
        if((bg = inputParser(argcs, commandline)) == -1){
            continue;
        }
        if(strcmp(argcs[0], "quit") == 0){
            if(childCount!=0)
            {
                int i;
                for(i=0;i<5;i++)
                {
                    if(allStatus[i].jid>=0)
                    {
                        kill(allStatus[i].pid,SIGCONT);
                        kill(allStatus[i].pid,SIGINT);
                        int status;
                        waitpid(allStatus[i].pid,&status,0);
                        
                    }
                }
            }
            break;
        }
        else if(strcmp(argcs[0], "jobs") == 0){
            showJob();
        }
        else if(strcmp(argcs[0], "fg") == 0){
            fg(&argcs[1]);

        }
        else if(strcmp(argcs[0], "bg") == 0){
            buildIn_bg(&argcs[1]);
        }
        else if(strcmp(argcs[0], "kill") == 0){
            buildIn_kill(&argcs[1]);
        }
        else if(strcmp(argcs[0], "cd") == 0){
            cd(argcs);
        }


        else{
            pid = fork();
            if(pid < 0){
                break;
            }
            if(pid == 0){ // child runs user job
                setpgid(0, 0);
                if((IOindex = checkInputOutputFile(copyOfCommandLine)) != -1) {
                    argcs[IOindex - 1] = NULL;
                }
                //printf("%s\n", argcs[0]);
                //printf("%d\n", (int) argcs[1]); fflush(stdout);
                //printf("argcs: %s\n",argcs);
                if(execv(argcs[0], argcs) < 0){
                    if(execvp(argcs[0], argcs) < 0)
                    {
                        printf("%s: Command not found.\n", argcs[0]);
                        exit(0);
                    }
                }

            }
            if(!bg){
                int status;
                //pid_t pid = getpgid(pid);
                //printf("1: %i\n",pid);
                if(waitpid(pid, &status, WUNTRACED) < 0){
                    //printf("7");
                    if (WIFSTOPPED(status)) {
                        continue;
                    }
                    else{
                        //printf("2\n");
                        waitpid(pid, &status, 0);
                    }
                    unix_error("waitfg: waitpid error");
                }
                //while(waitpid(-1,0,WNOHANG)>=0){};
                //printf("8");
                jid_count ++;
            }
            else{
                //printf("3\n");
                struct Status temp = {.jid = jid_count, .pid = pid};
                strcpy(temp.commands, copyOfCommandLine2);
                strcpy(temp.status, "Running");
                allStatus[childCount] = temp;
                childCount ++;
                jid_count ++;
                signal(SIGCHLD, sigchild_handler);
                printf("%d %s", pid, copyOfCommandLine2);
            }
        }

    }
    return 0;
}