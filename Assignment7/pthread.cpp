#include <iostream>
#include <pthread.h>
using namespace std;

int a;

void* threadCall(void* arg){
	//cout << __LINE__ << endl;
	a = 17;
	cout << "a in thread = " << a << endl;
	pthread_exit(NULL);
}
int main(){
	cout << "a = " << a << endl;
	pthread_t thread;
	int retVal = pthread_create(&thread, NULL, threadCall, NULL);
	cout << "retVal = "<< retVal << endl;
	sleep(1);
	a = 23;
	cout << "a in main = " << a << endl;
	pthread_join(thread, NULL);
	pthread_exit(NULL);
}