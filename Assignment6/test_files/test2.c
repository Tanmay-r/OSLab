#include <stdio.h>
#include <time.h>
int main() {

	clock_t begin, end;
	double time_spent;

	
/* here, do your time-consuming job */
	syscall(402,0,5,1,1,3);
	
	return 0;
}