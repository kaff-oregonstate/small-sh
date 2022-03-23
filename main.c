// main.c

// Auth: David Kaff
// Inst: Bram Lewis
// Date: 10 Feb 2021
// Crse: CS 344

// references:
// 'https://stackoverflow.com/a/1248017'
// 'https://stackoverflow.com/a/3068420'
// CS344 coursework
// https://man7.org/linux/man-pages/

// removed commented-out printf lines throughout which were there for error
    // testing, and were cluttering the code. If those are desired by the
    // grader I do have them saved.

// include libraries for functions
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <math.h>

struct commando {
    char* commo;
    int runBG;
    int inputFLAG;
    char* input_file;
    int outputFLAG;
    char* output_file;
    char* args[512];
    int numOfArgs;
};

void bgToggle(int signo) {
    char* bgFlagName = "BGENABLED";
    char *toggle = getenv(bgFlagName);
    // printf("curr toggle = '%s'\n", toggle);
    if (!strcmp(toggle,"1")) {
        setenv(bgFlagName, "0", 1);
        char *message = "\nEntering foreground-only mode (& is now ignored)\n: ";
        write(STDOUT_FILENO, message, 53);
    }
    else {
        setenv(bgFlagName, "1", 1);
        char *message = "\nExiting foreground-only mode\n: ";
        write(STDOUT_FILENO, message, 33);
    }
}

int loopOfSmallSH(int *currChildren, int zztotalNumOfChildren, int *numOfChildren) {

    char* statusName = "344STATUS";

  ///////////////////////////
 // begin loop of smallsh //
///////////////////////////

// prompt for input
printf(": ");
fflush(stdout);

// allocate memory in stack for input
char input[2048];


// scan for input
fflush(stdin);
fgets(input, 2048, stdin);

if ((strlen(input) > 0) && (input[strlen (input) - 1] == '\n'))
        input[strlen (input) - 1] = '\0';

// check for comment line
if (input[0] == '#') {
  return 0;
}

// --- get tokens from stream --- //

// allocate memory in stack for array of pointers to strings
char *tokens[512];
int inp_fpFLAG = 0;
char *inp_fp;
int out_fpFLAG = 0;
char *out_fp;
int curr = 0;

// stream input
char *savePtr;
char *token = strtok_r(input, " ", &savePtr);

// check for blank line
if (!token) {
  return 0;
}

while (token) {
    // --- do thing with token

    // scan token for in/out reassign

    // scan for '<', get input_file
    if (token[0] == '<'){
        // --- get next token
        token = strtok_r(NULL, " ", &savePtr);
        if (token){
            // copy path into inp_fp
            char *thisTok = (char*)malloc((strlen(token) + 1) * sizeof(char));
            sprintf(thisTok,"%s",token);
            inp_fp = thisTok;
            inp_fpFLAG = 1;

            // --- get next token
            token = strtok_r(NULL, " ", &savePtr);
            continue;
        }
    }

    // scan for '>', get output_file
    else if (token[0] == '>'){
        // --- get next token
        token = strtok_r(NULL, " ", &savePtr);
        if (token){
            // copy path into out_fp
            char *thisTok = (char*)malloc((strlen(token) + 1) * sizeof(char));
            sprintf(thisTok,"%s",token);
            out_fp = thisTok;
            out_fpFLAG = 1;

            // --- get next token
            token = strtok_r(NULL, " ", &savePtr);
            continue;
        }
    }

    // allocate memory in heap for token
    // calloc ea tokens
    char *thisTok = (char*)malloc((strlen(token) + 1) * sizeof(char));
    sprintf(thisTok,"%s",token);
    tokens[curr] = thisTok;
    curr++;


    // --- get next token
    token = strtok_r(NULL, " ", &savePtr);
}

// error handling for ctrl+z entering
if (!strcmp(tokens[0],"////////////////")) {
    return 0;
}

// scan each tokens[j] for '$$' and replace with pid
for (int j = 0; j < (curr); j++) {for (int i = 0; i < strlen(tokens[j]); i++){
    if (tokens[j][i] == '$'){ if (tokens[j][i+1] == '$'){

        int b4True = 0;
        int aftTrue = 0;
        char *b4;
        char *aft;

        if (i > 0){
            b4True = 1;
            b4 = (char*)malloc((i) * sizeof(char));
            for (int k=0; k<i; k++) {
                b4[k]=tokens[j][k];
            }
            b4[i]='\0';
        }

        if ((i+2) < strlen(tokens[j])){
            aftTrue = 1;
            aft = (char*)malloc((strlen(tokens[j])-(i+1)) * sizeof(char));
            for (int k=0; k<(strlen(tokens[j])-(i+1)); k++) {
                aft[k]=tokens[j][k+i+2];
            }
            aft[strlen(tokens[j])-(i+1)] = '\0';
        }

        // get pid
        pid_t bing = getpid();

        // digits in pid (need for newArg mem alloc)
        int nDigits = floor(log10(abs(bing))) + 1;

        int newArgLen = nDigits + 1;
        if(b4True) newArgLen += strlen(b4);
        if(aftTrue) newArgLen += strlen(aft);

            // can't remember what the (char*) in front of malloc is doing but it worked on the last assignment so I'm not going to question it yet...

        // slap it together
        char *newArg = (char*)malloc(newArgLen * sizeof(char));
        int lenSoFar = 0;
        if (b4True) {
            lenSoFar += sprintf((newArg + lenSoFar),"%s", b4);
        }
        lenSoFar += sprintf((newArg + lenSoFar),"%d", bing);
        if (aftTrue) {
            lenSoFar += sprintf((newArg + lenSoFar),"%s", aft);
        }
        fflush(stdout);

        // clear up memory on the other end of tokens[j]
        free(tokens[j]);
        if(b4True) free(b4);
        if(aftTrue) free(aft);

        // copy address of newArg memory into pointer tokens[j]
        tokens[j] = newArg;

        continue;
    }}
}}

// create struct commando from tokens, using pointer assignments
struct commando *currComm = malloc(sizeof(struct commando));
    // command
    currComm->commo = tokens[0];

    // run in background
    currComm->runBG = 0;
    // checking if background running is enabled, along with final arg
    char* bgFlagName = "BGENABLED";
    char *toggle = getenv(bgFlagName);
    if (tokens[curr-1][0] == '&') {
        if (!strcmp(toggle, "1")) {
            currComm->runBG = 1;
            curr--;
        }
        else {
            free(tokens[curr-1]);
            curr -= 1;
        }
    }

    // input_file
    if (inp_fpFLAG) {
        currComm->input_file = inp_fp;
        currComm->inputFLAG = 1;
    } else currComm->inputFLAG = 0;

    // output_file
    if (out_fpFLAG) {
        currComm->output_file = out_fp;
        currComm->outputFLAG = 1;
    } else currComm->outputFLAG = 0;

    // any args
    for (int j = 1; j < (curr); j++) {
        currComm->args[j-1] = tokens[j];
    }
    currComm->numOfArgs = (curr - 1);

//================================================//


       /////////////////////////////////
      // executing BUILT-IN commands //
     /////////////////////////////////


if (!strcmp(currComm->commo,"exit")) {
    // printf("exiting now.\n");
    return 1;
}

else if (!strcmp(currComm->commo,"cd")) {
    // printf("changing dir now now.\n");
    if (currComm->numOfArgs >= 1) chdir(currComm->args[0]);
    else chdir(getenv("HOME"));
}

else if (!strcmp(currComm->commo,"status")) {
    // printf("getenv(statusName) = %s\n", getenv(statusName));
    int currStatus = atoi(getenv(statusName));
    // printf("atoi(getenv(statusName)) = %d\n", currStatus);
    if(WIFEXITED(currStatus)){
        printf("exit value %d\n", WEXITSTATUS(currStatus));
    } else{
        printf("terminated by signal %d\n", WTERMSIG(currStatus));
    }
}

//=============================================//


       //////////////////////////////
      // executing OTHER commands //
     //////////////////////////////


else if (currComm->runBG) {
    // init sigaction
    struct sigaction SIGTSTP_action4 = {0};

    // create arguments vector
    char *argsFinal[1 + currComm->numOfArgs + 1];

    // point to command
    argsFinal[0] = currComm->commo;
    // point to arguments
    for (int l = 0; l < currComm->numOfArgs; l++) {
        argsFinal[l+1] = currComm->args[l];
    }
    // paste in a null pointer for good measure (thought we couldn't do this?)
    argsFinal[currComm->numOfArgs + 1] = NULL;

    // run this command in the background!
    pid_t spawnPid = fork();
    switch (spawnPid) {
        case -1:
            perror("fork()\n");
            exit(1);
            break;
        case 0:
            // SIGTSTP HANDLER: CHILD
                // signal handler = ignore
                SIGTSTP_action4.sa_handler = SIG_IGN;
                // Block no signals while ignoring
                sigemptyset(&SIGTSTP_action4.sa_mask);
                // Set no flags
                SIGTSTP_action4.sa_flags = 0;
                // Install signal handler
                sigaction(SIGTSTP, &SIGTSTP_action4, NULL);

            // child process instructions

            // dup2 the input and output as appropriate
            if (currComm->inputFLAG) {
                // open the file the user has said to stream from, save the file descriptor
                int ifd = open(currComm->input_file, O_RDONLY);
                if (ifd == -1) {
                    perror("open()");
                    exit(1);
                }
                // dup2: stdin point to ifd not command line
                int res = dup2(ifd, 0);
                if (res == -1) {
                    perror("dup2");
                    exit(2);
                }
            }
            else {
                // open /dev/null, save the file descriptor
                int dn = open("/dev/null", O_WRONLY);
                if (dn == -1) {
                    perror("open()");
                    exit(1);
                }
                // dup2: stdin point to /dev/null not command line
                int res = dup2(dn, 0);
                if (res == -1) {
                    perror("dup2");
                    exit(2);
                }
            }
            if (currComm->outputFLAG) {
                // open the file the user has said to write to, save the file descriptor
                int ofd = open(currComm->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if (ofd == -1) {
                    perror("open()");
                    exit(1);
                }
                // dup2: stdout point to ofd not terminal
                int res = dup2(ofd, 1);
                if (res == -1) {
                    perror("dup2");
                    exit(2);
                }
            }
            else {
                // open /dev/null, save the file descriptor
                int dn = open("/dev/null", O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if (dn == -1) {
                    perror("open()");
                    exit(1);
                }
                // dup2: stdout point to /dev/null not command line
                int res = dup2(dn, 1);
                if (res == -1) {
                    perror("dup2");
                    exit(2);
                }
            }
            // !!! no more printing! stdout could be a file!!

            // execute, handle any errors
            execvp(argsFinal[0], argsFinal);
            perror(argsFinal[0]);

            exit(1);
            break;
        default:
            // parent process instructions

            // keep track of new child
            currChildren[*numOfChildren] = spawnPid;

            // increment numOfChildren
            *numOfChildren = *numOfChildren + 1;

            // // don't wait, go back to prompting input
            printf("background pid is %d\n", spawnPid);
            // sleep(14);
            break;
    }
}

else {
    // init sigaction
    struct sigaction SIGINT_action2 = {0};
    // init sigaction
    struct sigaction SIGTSTP_action4 = {0};

    // create arguments vector
    char *argsFinal[1 + currComm->numOfArgs + 1];

    // point to command
    argsFinal[0] = currComm->commo;
    // point to arguments
    for (int l = 0; l < currComm->numOfArgs; l++) {
        argsFinal[l+1] = currComm->args[l];
    }
    // paste in a null pointer for good measure (thought we couldn't do this?)
    argsFinal[currComm->numOfArgs + 1] = NULL;

    // don't run this command in the background!
    pid_t spawnPid = fork();
    int childStatus;
    switch (spawnPid) {
        case -1:
        perror("fork()\n");
        exit(1);
        break;
        case 0:
            // child process instructions

            // SIGINT HANDLER: FG CHILD
                // signal handler = return to default behavior
                SIGINT_action2.sa_handler = SIG_DFL;
                // Block nothing while ignoring the signal
                sigemptyset(&SIGINT_action2.sa_mask);
                // Set no flags
                SIGINT_action2.sa_flags = 0;
                // Install signal handler
                sigaction(SIGINT, &SIGINT_action2, NULL);

            // SIGTSTP HANDLER: CHILD
                // signal handler = ignore
                SIGTSTP_action4.sa_handler = SIG_IGN;
                // Block no signals while ignoring
                sigemptyset(&SIGTSTP_action4.sa_mask);
                // Set no flags
                SIGTSTP_action4.sa_flags = 0;

                // Install signal handler
                sigaction(SIGTSTP, &SIGTSTP_action4, NULL);

            // dup2 the input and output as appropriate
            if (currComm->inputFLAG) {
                // open the file the user has said to stream from, save the file descriptor
                int ifd = open(currComm->input_file, O_RDONLY);
                if (ifd == -1) {
                    perror("open()");
                    exit(1);
                }
                // dup2: stdin point to ifd not command line
                int res = dup2(ifd, 0);
                if (res == -1) {
                    perror("dup2");
                    exit(2);
                }
            }
            if (currComm->outputFLAG) {
                // open the file the user has said to write to, save the file descriptor
                int ofd = open(currComm->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0640);
                if (ofd == -1) {
                    perror("open()");
                    exit(1);
                }
                // dup2: stdout point to ofd not terminal
                int res = dup2(ofd, 1);
                if (res == -1) {
                    perror("dup2");
                    exit(2);
                }
            }
            // !!! no more printing! stdout could be a file!!

            // execute, handle any errors
            execvp(argsFinal[0], argsFinal);
            perror(argsFinal[0]);

            exit(1);
            break;
        default:
            // parent process instructions
            // do a wait, let process run in foreground
            spawnPid = waitpid(spawnPid, &childStatus, 0);

            // update status
            char newStatus[1];
            sprintf(newStatus, "%d", childStatus);
            setenv("344STATUS", newStatus, 1);
            break;
    }
}

// free any memory inside commando struct
    // free ea 'memory inside commando struct', nee 'tokens'
    free(currComm->commo);
    for (int k = 0; k < (currComm->numOfArgs); k++) {
        free(currComm->args[k]);
    }
    if (currComm->inputFLAG) free(currComm->input_file);
    if (currComm->outputFLAG) free(currComm->output_file);



  /////////////////////////
 // end loop of smallsh //
/////////////////////////

return 0;

}

int main(int argc, char const *argv[]) {
    if (argc > 1) {
        perror("incorrect number of arguments to boot smallsh");
        exit(0);
    }

    // SIGINT HANDLER: PARENT AND BG CHILD (TO BE PASSED ON)
        // init sigaction
        struct sigaction SIGINT_action1 = {0};
        // signal handler = just ignore the signal
        SIGINT_action1.sa_handler = SIG_IGN;
        // Block nothing while ignoring the signal
        sigemptyset(&SIGINT_action1.sa_mask);
        // Set no flags
        SIGINT_action1.sa_flags = 0;
        // Install signal handler
        sigaction(SIGINT, &SIGINT_action1, NULL);

    // SIGTSTP HANDLER: PARENT ONLY (WILL BE RESET IN CHILDREN)
        // init sigaction
        struct sigaction SIGTSTP_action3 = {0};
        // signal handler = toggle BGENABLED
        SIGTSTP_action3.sa_handler = bgToggle;
        // Block all signals while toggling BGENABLED
        sigfillset(&SIGTSTP_action3.sa_mask);
        // Set no flags
        SIGTSTP_action3.sa_flags = SA_RESTART;
        // Install signal handler
        sigaction(SIGTSTP, &SIGTSTP_action3, NULL);


    // init env vars
    char* statusName = "344STATUS";
    setenv(statusName, "0", 1);
    char* bgEnabl = "BGENABLED";
    setenv(bgEnabl, "1", 1);

    // init holder for child process pids
    int currChildren[512]={0} ;
    int numOfChildren = 0;

    // init resp for smallSH loop
    int resp = 0;
    while (!resp) {
        // run smallSH loop
        resp = loopOfSmallSH(currChildren, 512, &numOfChildren);

        // check for any zombies
        for (int i = 0; i < numOfChildren; i++) {
            int childStatus;
            int hasExited = waitpid(currChildren[i], &childStatus, WNOHANG);
            if (hasExited){

                // change the status of smallsh
                char newStatus[8];
                sprintf(newStatus, "%d", childStatus);
                setenv(statusName, newStatus, 1);

                // print status of background id
                if(WIFEXITED(atoi(newStatus))){
                    printf("background pid %d is done: exit value %d\n", hasExited, WEXITSTATUS(atoi(newStatus)));
                } else{
                    printf("background pid %d is done: terminated by signal %d\n", hasExited, WTERMSIG(atoi(newStatus)));
                }

                // do loop to clean up currChildren array
                for (int j=i; j<(numOfChildren - 1); j++) {
                    currChildren[j] = currChildren[j+1];
                }
                currChildren[numOfChildren - 1] = 0;

                // decrement numOfChildren and i
                numOfChildren = numOfChildren - 1;
                i = i - 1;
            }
        }
    }

    if (resp == 1) {
        // clean up all child processes
        for (int i = 0; i < numOfChildren; i++) {
            kill(currChildren[i], SIGTERM);
        }
        // exit
        exit(0);
    }

    return EXIT_SUCCESS;
}
