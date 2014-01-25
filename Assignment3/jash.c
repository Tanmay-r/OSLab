#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>


#define MAXLINE 1000
#define DEBUG 0

//global variable used to store the pid of the process(inside a child, it is zero)
pid_t pid;

//declarations
char ** tokenize(char*);
int is_equal(char * s1, char * s2);
void runCommand(char * s);
int cdCommand(char *s);
void executeFile(char ** tokens);
void handler(int signum);
void cronCommand(char *commandFile);
void parallelCommand(char **tokens);

int main(int argc, char** argv){
	//Setting the signal interrupt to its default function. 
	signal(SIGINT, handler);

	//get the pid of the current process
	pid = getpid();

	//Allocating space to store the previous commands.
	int numCmds = 0;
	char **cmds = (char **)malloc(1000 * sizeof(char *));

	int printDollar = 1;

	char input[MAXLINE];
	char** tokens;

	int notEOF = 1;
	int i;

	FILE* stream = stdin;

	while(notEOF) { 
		if (printDollar == 1){ 
			printf("$ "); // the prompt
			fflush(stdin);
		}

		char *in = fgets(input, MAXLINE, stream); //taking input one line at a time

		//Checking for EOF
		if (in == NULL){
			if (DEBUG) printf("EOF found\n");
			exit(0);
		}

		//add the command to the command list.
		cmds[numCmds] = (char *)malloc(sizeof(input));
		strcpy(cmds[numCmds++], input); 

		// Calling the tokenizer function on the input line    
		tokens = tokenize(input);

		// check tokens and execute corresponding command
		if(tokens[0] == NULL){
			printf("");
		}
		else if( is_equal(tokens[0], "run\0") ) {
    			runCommand(tokens[1]);
		}
		else if(is_equal(tokens[0], "cd\0")){
			cdCommand(tokens[1]);
		}
		else if(is_equal(tokens[0], "cron\0")){
			cronCommand(tokens[1]);
		}
		else if(is_equal(tokens[0], "parallel\0")){
			parallelCommand(tokens);
		}
		else{
			executeFile(tokens);
		} 
	}
  
  
	printf("Print and deallocate %s\n", tokens[0]);
	// Freeing the allocated memory	
	for(i=0;tokens[i]!=NULL;i++){
		free(tokens[i]);
	}
	free(tokens);
	return 0;
}

/*Signal handler that handles SIGINT*/
void handler(int signum){
	if(pid == 0){
		exit(1);
	}
}

/*exe cute parallel jobs, takes an array of char strings as input*/
void parallelCommand(char **tokens){
	int index = 1;
	int status = 0;
	pid_t *pidArray = (pid_t *)malloc(1000*sizeof(pid_t));
	int commandNumber = 0;
	while(tokens[index] != NULL){
		char **command = (char **) malloc(MAXLINE*sizeof(char**));
		int i = 0;	
		//concatenate a command till :::
		while ((tokens[index] != NULL) && (!is_equal(tokens[index], ":::\0"))){					
			command[i++] = tokens[index++];
				
		}
		if((pidArray[commandNumber++] = fork()) == 0){
			int returnVal = execvp(command[0], command);
			if(returnVal == -1){
				perror("Error ");
				exit(1);
			}
			exit(0);
		}
		for(i=0;command[i]!=NULL;i++){
			free(command[i]);
		}
		free(command);	
		if(tokens[index] != NULL){
			index++;
		}		
	}
	int i = 0;
	int flag = 1;
	//check if parent, flag remains 1 for parent
	for(i = 0; i < commandNumber; i++){
		if(pidArray[i] == 0){
			flag = 0;
		}
	}
	//kill all children
	if(flag == 1){
		for(i = 0; i < commandNumber; i++){
			waitpid(pidArray[commandNumber], &status, 0);
		}
	}
	free(pidArray);
}

/*executes the file corresponding to the filename in tokens*/
void executeFile(char ** tokens){
	int status;
	if(fork() == 0){    	
		int returnVal = execvp(tokens[0], tokens);
		if(returnVal == -1){
			perror("Error ");
			exit(1);
		}
		exit(0);
	}
	else {
      	wait(NULL);
 	}
}

/*compares two strings, return one if true*/
int is_equal(char * s1, char * s2){
	int index = 0;
	while((s1[index] != '\0') && (s2[index] != '\0')){
		if(s1[index] != s2[index]){
			return 0;	
		}
		index++;
	}
	if((s1[index] == '\0') && (s2[index] == '\0')){
		return 1;
	}
}

/*the operation of the run command, takes one argument*/
void runCommand(char * s){
	FILE * fp;
	char** command;
    char * line = NULL;
  	size_t len = 0;
   	ssize_t read;
	int status;
   	fp = fopen(s, "r");
    while ((read = getline(&line, &len, fp)) != -1) {
		//read the next line from the file and executing it
    	command = tokenize(line);
		int status;
		if(fork() == 0){    	
			int returnVal = execvp(command[0], command);
			if(returnVal == -1){
				perror("Error ");
				exit(1);
			}
			exit(0);
		}
		else {
          	wait(&status);
			if(status != 0){
				break;
			}
     	}	
	}	
}

/*the operation of the cd command, takes one argument*/
int cdCommand(char *path){

    int indicator=chdir(path);
   
    if(indicator==-1)
        perror("chdir");
       
    return indicator;

}

/*the operation of core command, take a file path as input*/
void cronCommand(char *commandFile){
	FILE * fp;
	char** command;
	char** tokens;
    char * line = NULL;
  	size_t len = 0;
   	ssize_t read;
	int status;
   	fp = fopen(commandFile, "r");
	while ((read = getline(&line, &len, fp)) != -1) {
	 	tokens = tokenize(line);
		int status;
		time_t t = time(NULL);	
		command = (char **) malloc(MAXLINE*sizeof(char**));
		if(fork() == 0){
			int flag = 1;
			int count = 1;
			while(1){
				if(count == 1){
					count = 0;
					flag = 1;
					struct tm tm = *localtime(&t);    	
					int min=-1, hour=-1, date=-1, month=-1, week=-1;
					if (!is_equal(tokens[0], "*\0")){
						min=atoi(tokens[0]);
					}
					if (!is_equal(tokens[1], "*\0")){
						hour=atoi(tokens[1]);
					}
					if (!is_equal(tokens[2], "*\0")){
						date=atoi(tokens[2]);
					}
					if (!is_equal(tokens[3], "*\0")){
						month=atoi(tokens[3]);
					}
					if (!is_equal(tokens[4], "*\0")){
						week=atoi(tokens[4]);
					}
					int i = 0;
					int index = 5;
					while(tokens[index] != NULL){
						command[i++] = tokens[index++];
					}
					if((tm.tm_min==min || is_equal(tokens[0],"*\0"))
						&& (tm.tm_hour==hour || is_equal(tokens[1],"*\0"))
						&& (tm.tm_mday==date || is_equal(tokens[2],"*\0"))
						&& (tm.tm_mon==month || is_equal(tokens[3],"*\0"))
						&& (tm.tm_wday==week || is_equal(tokens[4],"*\0"))
						&& (flag == 1)){
						flag = 0;
						if(fork() == 0){    	
							int returnVal = execvp(command[0], command);
							if(returnVal == -1){
								perror("Error ");
								exit(1);
							}
							exit(0);
						}
					}
					if(sleep(60) == 0){
						count = 1;
					}
				}
			}
			exit(0);
		}
	}
}

/*the tokenizer function takes a string of chars and forms tokens out of it*/
char ** tokenize(char* input){
	int i;
	int doubleQuotes = 0;
	
	char *token = (char *)malloc(1000*sizeof(char));
	int tokenIndex = 0;

	char **tokens;
	tokens = (char **) malloc(MAXLINE*sizeof(char**));
 
	int tokenNo = 0;
	
	for(i =0; i < strlen(input); i++){
		char readChar = input[i];
		
		if (readChar == '"'){
			doubleQuotes = (doubleQuotes + 1) % 2;
			if (doubleQuotes == 0){
				token[tokenIndex] = '\0';
				if (tokenIndex != 0){
					tokens[tokenNo] = (char*)malloc(sizeof(token));
					strcpy(tokens[tokenNo++], token);
					tokenIndex = 0; 
				}
			}
		}
		else if (doubleQuotes == 1){
			token[tokenIndex++] = readChar;
		}
		else if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(sizeof(token));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
		}
		else{
			token[tokenIndex++] = readChar;
		}
	}
	
	if (doubleQuotes == 1){
		token[tokenIndex] = '\0';
		if (tokenIndex != 0){
			tokens[tokenNo] = (char*)malloc(sizeof(token));
			strcpy(tokens[tokenNo++], token);
		}
	}
	
	return tokens;
}
