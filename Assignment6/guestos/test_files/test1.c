#include <stdio.h>
#include <time.h>
int main() {

	clock_t begin, end;
	double time_spent;

	
/* here, do your time-consuming job */
	syscall(401,10);

	int i = 0;
	int a;
	begin = clock();
	for(i=0;i<100000;i++){
		a++;
	}
	end = clock();
	time_spent = (double)(end - begin) / CLOCKS_PER_SEC;
	printf("instruction slice 10: %f\n",time_spent);
	return 0;
}