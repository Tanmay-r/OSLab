#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <string>
#include <map>
#include <sstream>
#include <set>
#include <csignal>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstdio>
#include <cstdlib>
#include <iterator>

#define SHMSZ    27


using namespace std;

set <string> emails;
map <string, int> childProcesses;

void handler(int signum){
	int shmid;
	char *shm, *s;
	shmid = shmget(1234, SHMSZ, 0666);
	shm = (char*)shmat(shmid, NULL, 0);
	s = shm;
	char c = s[0];

	//Add email
	if(c == '0'){
		int i = 1;
		string email="";
		c = s[i];		
		while(c != '\0'){
			email += c ;
			i++;
			c = s[i];			
		}				
		email += '\0';

		int loc;
		loc=email.find("@");
		string username=email.substr(0,loc);
		string domain=email.substr(loc+1);

		set<string>::iterator it = emails.find(email);
		if (it != emails.end()){
			string message = "Child process ";
			message += domain.substr(0, domain.size() - 1);
			message += " - Email address already exists.";
			s = shm;

			int i = 0;
			*s++ = '5';
			char c = message[i];
			while(c != '\0'){
				i++;
				*s++ = c;
				c = message[i];
			}
			*s++ = '\0';						
			s = NULL;
			kill(getppid(), SIGUSR1);
		}
		else{
			emails.insert(email);
			string message = "Child process ";
			message += domain.substr(0, domain.size() - 1);
			message += " - Email address ";
			message += email.substr(0, email.size() - 1);
			message += " added successfully.";
			s = shm;
			int i = 0;
			*s++ = '5';
			char c = message[i];
			while(c != '\0'){
				i++;
				*s++ = c;
				c = message[i];
			}			
			*s++ = '\0';
			s = NULL;
			kill(getppid(), SIGUSR1);
		}
	}

	//Delete email
	else if(c == '1'){
		int i = 1;
		string email="";
		c = s[i];		
		while(c != '\0'){
			email += c ;
			i++;
			c = s[i];
			
		}						
		email += '\0';

		int loc;
		loc=email.find("@");
		string username=email.substr(0,loc);
		string domain=email.substr(loc+1);

		set<string>::iterator it = emails.find(email);

		if (it != emails.end()){
			emails.erase(it);
			string message = "Child process - child ";
			message += domain.substr(0, domain.size() - 1);
			message += " deleted ";
			message += email.substr(0, email.size() - 1);
			message += ".";
			s = shm;
			int i = 0;
			*s++ = '5';
			char c = message[i];
			while(c != '\0'){
				i++;
				*s++ = c;
				c = message[i];
			}		
			*s++ = '\0';				
			s = NULL;
			kill(getppid(), SIGUSR1);
		}
		else{
			string message = "Parent process - child ";
			message += domain.substr(0, domain.size() - 1);
			message += " could not find the email address ";
			message += email.substr(0, email.size() - 1);
			s = shm;
			int i = 0;
			*s++ = '5';
			char c = message[i];
			while(c != '\0'){
				i++;
				*s++ = c;
				c = message[i];
			}		
			*s++ = '\0';				
			s = NULL;
			kill(getppid(), SIGUSR1);
		}
	}

	//Search email
	else if(c == '2'){
		int i = 1;
		string email="";
		c = s[i];		
		while(c != '\0'){
			email += c ;
			i++;
			c = s[i];
			
		}						
		email += '\0';

		int loc;
		loc=email.find("@");
		string username=email.substr(0,loc);
		string domain=email.substr(loc+1);

		set<string>::iterator it = emails.find(email);
		if (it != emails.end()){
			string message = "Child process - child ";
			message += domain.substr(0, domain.size() - 1);
			message += " found the email address ";
			message += email.substr(0, email.size() - 1);
			message += ".";
			s = shm;
			int i = 0;
			*s++ = '5';
			char c = message[i];
			while(c != '\0'){
				i++;
				*s++ = c;
				c = message[i];
			}		
			*s++ = '\0';				
			s = NULL;
			kill(getppid(), SIGUSR1);
		}
		else{
			string message = "Parent process - child ";
			message += domain.substr(0, domain.size() - 1);
			message += " could not find the email address ";
			message += email.substr(0, email.size() - 1);
			s = shm;
			int i = 0;
			*s++ = '5';
			char c = message[i];
			while(c != '\0'){
				i++;
				*s++ = c;
				c = message[i];
			}						
			*s++ = '\0';
			s = NULL;
			kill(getppid(), SIGUSR1);
		}
	}

	//delete domain
	else if(c == '3'){
		for (set<string>::iterator it=emails.begin(); it!=emails.end(); ++it){
			cout << *it << endl;
				
		} 	
		int i = 1;
		string email="";
		c = s[i];		
		while(c != '\0'){
			email += c ;
			i++;
			c = s[i];
			
		}						
		email += '\0';

		string message = "Parent process - Domain ";
		message += email.substr(0, email.size() - 1);
		message += " with pid ";
		int pid = getpid();
		cout << pid << endl;
		char* temp;
		sprintf(temp, "%d", pid);
		string str(temp);
		cout<<temp<<" "<<str<<endl;
		message += str;

		message += " deleted.";
		s = shm;
		i = 0;
		*s++ = '5';
		char c = message[i];
		while(c != '\0'){
			i++;
			*s++ = c;
			c = message[i];
		}
		*s++ = '\0';
		s = NULL;
		kill(getppid(), SIGUSR1);
		_exit(0);
	}

	//delete domain to quit
	else if(c == '4'){
		for (set<string>::iterator it=emails.begin(); it!=emails.end(); ++it){
			cout << *it << endl;
		}   
		s = NULL;
		_exit(0);		
	}

	//parent handler
	else if(c == '5'){
		s = shm;
		int i = 1;
		string message="";
		c = s[i];		
		while(c != '\0'){
			message += c ;
			i++;
			c = s[i];
			
		}						
		message += '\0';
		cout << message << endl;
		s = shm;
		*s++ = '7';
		*s++ = '\0';
		s = NULL;
	}
}

int main(void) {
	signal(SIGUSR1, handler);

	cout << "parent process: "<< getpid() << "\n";
		
	string query = "";
	string email = "";
	string domain ="";
	string username ="";
	int loc=0;
	
	//Create shared memory
	int shmid;
	char *shm;
	shmid = shmget(1234, SHMSZ, IPC_CREAT | 0666);
	shm = (char*)shmat(shmid, NULL, 0);
	char *s;
	while(1){

		cin >> query;
		if(!query.compare("Quit")){
			for (map <string, int>::iterator it=childProcesses.begin(); it!=childProcesses.end(); ++it){
				s = shm;
				int i = 0;
				*s++ = '4';
				kill(it->second, SIGUSR1);
			}
			break;
		}
		else if(!query.compare("delete_domain")){
			cin >> email;
			//delete domain
			map <string, int>::iterator it=childProcesses.find(domain);
			if(it!=childProcesses.end()){
				s = shm;
				int i = 0;
				*s++ = '3';
				char c = email[i];
				while(c != '\0'){
					i++;
					*s++ = c;
					c = email[i];
				}			
				*s++ = '\0';			
				s = NULL;
				kill(childProcesses[domain], SIGUSR1);
				bool flag = true;
				while(flag){
					s = shm;
					if(s[0] == '7'){
						flag = false;
					}
				}
			}
			else{
				cout << "Parent process - Domain does not exist." << endl;
			}
		}
		else{	
			cin >> email;
			loc=email.find("@");
			username=email.substr(0,loc);
			domain=email.substr(loc+1);
			
			if(!query.compare("add_email")){
				//add email
				map <string, int>::iterator it=childProcesses.find(domain);
				if(it!=childProcesses.end()){
					s = shm;
					int i = 0;
					char c = email[i];
					*s++ = '0';
					while(c != '\0'){
						i++;
						*s++ = c;
						c = email[i];
					}		
					*s++ = '\0';
					s = NULL;
					kill(childProcesses[domain], SIGUSR1);
					bool flag = true;
					while(flag){
						s = shm;
						if(s[0] == '7'){
							flag = false;
						}
					}
				}
				else{
					int cpid = fork();
					if(cpid == 0) {
						while(1){
							int k = 0;
						}					
					}
					else if(cpid > 0){
						childProcesses[domain] = cpid;

						//Adding email id
						s = shm;
						int i = 0;

						*s++ = '0';
						char c = email[i];
						while(c != '\0'){
							*s++ = c;
							i++;
							c = email[i];
						}			
						*s++ = '\0';			
						s = NULL;
						kill(childProcesses[domain], SIGUSR1);
						bool flag = true;
						while(flag){
							s = shm;
							if(s[0] == '7'){
								flag = false;
							}
						}
					}
				}
			}
			else if(!query.compare("delete_email")){
				//delete email
				map <string, int>::iterator it=childProcesses.find(domain);
				if(it!=childProcesses.end()){
					s = shm;
					int i = 0;
					char c = email[i];
					*s++ = '1';
					while(c != '\0'){
						i++;
						*s++ = c;
						c = email[i];
					}					
					*s++ = '\0';	
					s = NULL;
					kill(childProcesses[domain], SIGUSR1);
					bool flag = true;
					while(flag){
						s = shm;
						if(s[0] == '7'){
							flag = false;
						}
					}
				}
				else{
					cout << "Parent process - Domain does not exist." << endl;
				}

			}
			else if(!query.compare("search_email")){
				//search email
				map <string, int>::iterator it=childProcesses.find(domain);
				if(it!=childProcesses.end()){
					s = shm;
					int i = 0;
					*s++ = '2';
					char c = email[i];
					while(c != '\0'){
						i++;
						*s++ = c;
						c = email[i];
					}					
					*s++ = '\0';	
					s = NULL;
					kill(childProcesses[domain], SIGUSR1);
					bool flag = true;
					while(flag){
						s = shm;
						if(s[0] == '7'){
							flag = false;
						}
					}
				}
				else{
					cout << "Parent process - Domain does not exist." << endl;
				}
			}	
		}			
	}
	return 0;
}
