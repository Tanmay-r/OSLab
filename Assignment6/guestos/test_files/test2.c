#include <stdio.h>
#include <time.h>
int main() {

	clock_t begin, end;
	double time_spent;
	char str2[12];
	
/* here, do your time-consuming job */
	syscall(402,9,str2,0,2);
	printf("test2 :%s\n",str2);
	
	return 0;
}