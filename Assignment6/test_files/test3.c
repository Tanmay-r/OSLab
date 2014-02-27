#include <stdio.h>
#include <syscall.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>

int main() {

	char *str1 = "alok yadav";
	char str2[12];

	printf("%s\n%s\n",str1,str2);

	syscall(403,11,str1,0,0);
	syscall(402,9,str2,0,2);
	printf("try\n");
	printf("%s\n%s\n",str1,str2);

	exit(1);
	return 0;

}
