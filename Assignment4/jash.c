#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <signal.h>
#include <sys/types.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>


#define MAXLINE 1000
#define DEBUG 0

//global variable used to store the pid of the process(inside a child, it is zero)
pid_t pid;

//pids of the hidden children
pid_t *children;

//prompt ready variables
int ready;

//declarations
char ** tokenize(char*);
int is_equal(char * s1, char * s2);
void runCommand(char * s);
int cdCommand(char *s);
void executeFile(char ** tokens, int inCounter, char * infilename, int outCounter, char * outfilename, int appendFlag);
void handler(int signum);
void sigchldHandler(int signum);
void cronCommand(char *commandFile);
void parallelCommand(char **tokens);
void pipeCommand(char** tokens);
void exitCommand();

int main(int argc, char** argv){
	//Setting the signal interrupt to its default function. 
	signal(SIGINT, handler);
	signal(SIGCHLD, sigchldHandler);

	children = (pid_t *)malloc(1000*sizeof(pid_t));
	children[0] = -1;

	//get the pid of the current process
	pid = getpid();

	ready = 1;

	//Allocating space to store the previous commands.
	int numCmds = 0;
	char **cmds = (char **)malloc(1000 * sizeof(char *));

	char input[MAXLINE];
	char** tokens;
	char** command;

	int notEOF = 1;
	int i;

	//variables to store information about I/O redirection
	int inCounter=0 , outCounter=0, pipeCounter = 0, appendFlag=0;
	char * infilename;
	char * outfilename;

	FILE* stream = stdin;
	
	
	while(notEOF) { 
		if (ready == 1){ 
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
		command = (char **) malloc(MAXLINE*sizeof(char**));

		//Check for input and output redirection
		int k=0;
		int flag = 1;
		int pipeFlag = 0;
		for(k=0;tokens[k]!=NULL;k++){
			if( is_equal(tokens[k], "<\0")){
				inCounter++;
				if(inCounter == 1){
					infilename = tokens[k + 1];
				}
				flag = 0;
			}
			if( is_equal(tokens[k], ">\0")){
				outCounter++;
				if(outCounter == 1){
					if(is_equal(tokens[k+1], ">\0")){
						//if appending is done while output redirection
						appendFlag=1;
						outfilename=tokens[k + 2];
						outCounter++;
						k++;
					}
					else
						outfilename = tokens[k + 1];
				}
				
				flag = 0;
			}
			if(is_equal(tokens[k], "|\0")){
				pipeFlag = 1;
			}
			if(flag == 1){
				command[k] = tokens[k];
			}
		}
		
		//if piping is used
		if(pipeFlag == 1){
			pipeCommand(tokens);
		}

		else {
			if(inCounter > 1){
				perror("More than one input redirections.");
			}
			else if(outCounter > 2 || (appendFlag == 0 && outCounter == 2)){
				perror("More than one output redirections.");
			}
			else{
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
					parallelCommand(command);
				}
				else if(is_equal(tokens[0], "exit\0")){
					exitCommand();
				}
				else{
					executeFile(command, inCounter, infilename, outCounter, outfilename, appendFlag);
				}
			}
		}
		inCounter = 0;
		outCounter = 0;
		appendFlag = 0;
	}
  
  
	printf("Print and deallocate %s\n", tokens[0]);
	// Freeing the allocated memory	
	for(i=0;tokens[i]!=NULL;i++){
		free(tokens[i]);
	}
	free(tokens);
	for(i=0;command[i]!=NULL;i++){
		free(command[i]);
	}
	free(command);
	return 0;
}

/*Signal handler for sigchild*/
void sigchldHandler(int signum){
	pid_t pid;
  	pid = wait(NULL);
  	
  	int k = 0;
  	for(k = 0; children[k] != -1; k++){
  		if(pid == children[k]){
  			children[k] = -2;
  			printf("Pid %d exited. ", pid);
  			printf("\n");
  			printf("$ ");
  			fflush(stdout);
  			break;
  		}
  	}
  	ready = 1;
}

/*Signal handler that handles SIGINT*/
void handler(int signum){
	if(pid == 0){
		exit(1);
	}
}

/*Function called to exit all the commands*/
void exitCommand(){
	int k = 0;
	for(k=0; children[k] != -1; k++){
		if(children[k] != -2){
			printf("Pid %d killed. \n", pid);
			kill(children[k], SIGKILL);
		}
	}
	free(children);
	exit(0);
}

/*Function called when piping is used*/
void pipeCommand(char** tokens){
	char ** command1;
	char ** command2;
	command1 = (char **) malloc(MAXLINE*sizeof(char**));
	command2 = (char **) malloc(MAXLINE*sizeof(char**));
	int status;
	int k=0; 
	int l=0;
	int flag = 1;
	for(k=0; tokens[k]!=NULL; k++){
		if(flag == 0){
			command2[l] = tokens[k];
			l++;
		}
		if( is_equal(tokens[k], "|\0")){
			flag = 0;
			l = 0;
		}
		if(flag == 1){
			command1[k] = tokens[k];
		}
		
	}

    int fd[2];
    pid_t pid;
    pid_t child1 ,child2;
    int     result;
    //Creating a pipe
    result = pipe (fd);

    if (result < 0) {
        //failure in creating a pipe
        perror("Pipe could not be created.");
        exit (1);
    }

    if((child1 =fork()) == 0){
    	signal(SIGINT, SIG_DFL);
    	close(fd[0]);
    	dup2(fd[1], 1);
      	

      	int returnVal = execvp(command1[0], command1);
      	if(returnVal == -1){
			perror("Error ");
			exit(1);
		}
      	exit(0);
    }
    if((child2 =fork()) == 0){
		signal(SIGINT, SIG_DFL);
		close(fd[1]);
		dup2(fd[0], 0);

	    
      	int returnVal = execvp(command2[0], command2);
      	if(returnVal == -1){
			perror("Error ");
			exit(1);
		}
      	exit(0);
    }
    if((child1 != 0) && (child2 != 0)){
    	close(fd[0]);
    	close(fd[1]);
    	waitpid(child1, &status, 0);
    	waitpid(child2, &status, 0);    	
    }
    int i = 0;
    //Free up the allocated memory
    for(i=0;command1[i]!=NULL;i++){
		free(command1[i]);
	}
	free(command1);
	for(i=0;command2[i]!=NULL;i++){
		free(command2[i]);
	}
	free(command2);
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
void executeFile(char ** tokens, int inCounter, char * infilename, int outCounter, char * outfilename, int appendFlag){
	int status;
	int k=0;
	int backgroundFlag = 0;
	char ** command = (char **) malloc(MAXLINE*sizeof(char**));
	int j = 0;
	for(k=0;tokens[k]!=NULL;k++){
		if( is_equal(tokens[k], "&\0")){
			backgroundFlag = 1;
		}
		else{
			command[j++] = tokens[k];
		}
	}
	pid_t chpid;
	if((chpid = fork()) == 0){
		if((inCounter == 0) && (outCounter == 0)){
			int returnVal = execvp(command[0], command);
			if(returnVal == -1){
				perror("Error ");
				exit(1);
			}
		}
		else if((inCounter == 1) && (outCounter == 0)){
			int file = open(infilename, O_RDONLY);
			if(file == -1){
				perror("Error while opening the input file.");
				exit(1);
			}
			else{
				dup2(file, 0);
				int returnVal = execvp(command[0], command);
				close(file);
				if(returnVal == -1){
					perror("Error ");
					exit(1);
				}
			}
		}
		else if((inCounter == 0) && (outCounter == 1)){
			int file = open(outfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
			if(file == -1){
				perror("Error while writing the output file.");
				exit(1);
			}
			else{
				dup2(file, 1);
				int returnVal = execvp(command[0], command);
				close(file);
				if(returnVal == -1){
					perror("Error ");
					exit(1);
				}
			}
		}
		else if((inCounter == 1) && (outCounter == 1)){
			int infile = open(infilename, O_RDONLY);
			int outfile = open(outfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
			if(infile == -1){
				perror("Error while reading the input file.");
				exit(1);
			}
			if(outfile == -1){
				perror("Error while writing the output file.");
				exit(1);
			}
			else{
				dup2(outfile, 1);
				dup2(infile, 0);
				int returnVal = execvp(command[0], command);
				close(infile);
				close(outfile);
				if(returnVal == -1){
					perror("Error ");
					exit(1);
				}
			}
		}
		else if((inCounter == 0) && (appendFlag == 1)){
			int file = open(outfilename, O_RDWR | O_APPEND |O_CREAT, S_IRUSR | S_IWUSR);
			if(file == -1){
				perror("Error while writing the output file.");
				exit(1);
			}
			else{
				dup2(file, 1);
				int returnVal = execvp(command[0], command);
				close(file);
				if(returnVal == -1){
					perror("Error ");
					exit(1);
				}
			}


		}
		else if((inCounter == 1) && (appendFlag == 1)){
			int infile = open(infilename, O_RDONLY);
			int outfile = open(outfilename, O_RDWR | O_APPEND |O_CREAT, S_IRUSR | S_IWUSR);
			if(infile == -1){
				perror("Error while reading the input file.");
				exit(1);
			}
			if(outfile == -1){
				perror("Error while writing the output file.");
				exit(1);
			}
			else{
				dup2(outfile, 1);
				dup2(infile, 0);
				int returnVal = execvp(command[0], command);
				close(infile);
				close(outfile);
				if(returnVal == -1){
					perror("Error ");
					exit(1);
				}
			}
		}
		exit(0);
	}
	else {
		if(backgroundFlag == 1){
			for(k=0; children[k] != -1; k++){
				if(children[k] == -2){
					children[k] = chpid;
				}
			}
			if(children[k] == -1){
				children[k] = chpid;
				children[k+1] = -1;
			}
			backgroundFlag = 0;
		}
		else{
			ready = 0;
			while(ready == 0){}
		}
		int i = 0;
		//Free up the memory
		for(i=0;command[i]!=NULL;i++){
			free(command[i]);
		}
		free(command);
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
	char** tokens;
    char * line = NULL;
    char *infilename, *outfilename;
  	size_t len = 0;
   	ssize_t read;
	int status;
   	fp = fopen(s, "r");
   	if(fp == NULL){
   		perror("");

   	}
   	else{
	    while ((read = getline(&line, &len, fp)) != -1) {
			//read the next line from the file and executing it
	    	tokens = tokenize(line);
	    	// Calling the tokenizer function on the input line    
			command = (char **) malloc(MAXLINE*sizeof(char**));
			//Check for input and output redirection
			int inCounter=0 , outCounter=0, pipeCounter = 0, appendFlag=0;
			int k=0;
			int flag = 1;
			int pipeFlag = 0;
			for(k=0;tokens[k]!=NULL;k++){
				if( is_equal(tokens[k], "<\0")){
					inCounter++;
					if(inCounter == 1){
						infilename = tokens[k + 1];
					}
					flag = 0;
				}
				if( is_equal(tokens[k], ">\0")){
					outCounter++;
					if(outCounter == 1){
						if(is_equal(tokens[k+1], ">\0")){
							appendFlag=1;
							outfilename=tokens[k + 2];
							outCounter++;
							k++;
						}
						else
							outfilename = tokens[k + 1];
					}
					
					flag = 0;
				}
				if(is_equal(tokens[k], "|\0")){
					pipeFlag = 1;
				}
				if(flag == 1){
					command[k] = tokens[k];
				}
			}
			if(pipeFlag == 1){
				pipeCommand(tokens);
			}
			else{
				executeFile(command, inCounter, infilename, outCounter, outfilename, appendFlag);
			}
			inCounter=0;
			outCounter=0;
			pipeCounter = 0;
			appendFlag=0;	
		}	
		fclose(fp);
	}
	int i = 0;
	//Free up the memory
	for(i=0;command[i]!=NULL;i++){
		free(command[i]);
	}
	free(command);
	for(i=0;tokens[i]!=NULL;i++){
		free(tokens[i]);
	}
	free(tokens);
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
	int i,j;
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
		else if (readChar == ' ' || readChar == '\n' || readChar == '\t' || readChar=='<' || readChar=='|' || readChar=='>'){
			token[tokenIndex] = '\0';
			if (tokenIndex != 0){
				tokens[tokenNo] = (char*)malloc(sizeof(token));
				strcpy(tokens[tokenNo++], token);
				tokenIndex = 0; 
			}
			if(readChar == '<' || readChar == '|' || readChar == '>' || readChar == '&'){
				token[0] = readChar;
				token[1] = '\0';
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

