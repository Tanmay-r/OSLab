#include <iostream>
#include <unistd.h>
#include <sys/wait.h>


using namespace std;
 
void child1(void) {
	//...zombie
	int x = 0;
}

void child2(void) {
	//...sleep
	int a;
	cin >> a;
}

void child3(void) {
	//...runnable
	int x = 0;	
	while(1){
		x = x + 1;	
	}
}

int main(void) {
	cout << "parent process: "<< getpid() << "\n";

	if ( fork() == 0 ) {
		cout << "child1: " << getpid() << "\n";
		child1();
		cout << "exiting child1\n";
		_exit(0);
	}
	
	if ( fork() == 0 ) {
		cout << "child2: " << getpid() << "\n";
		child2();
		cout << "exiting child2\n";
		_exit(0);
	}

	if ( fork() == 0 ) {
		cout << "child3: " << getpid() << "\n";
		child3();
		cout << "exiting child3\n";
		_exit(0);
	}
	sleep(60);	//the child 1 will remain in zombie state for 60 seconds

	int count=0;
	while(count!=3){
		waitpid(-1,NULL,0);
		count++;
	}
	return 0;
}
