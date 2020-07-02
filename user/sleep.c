#include "kernel/types.h"
#include "user/user.h"
int main(int argc,char *argv[])
{
	if(argc!=2)
	{
		printf("Uage: 2 , time to sleep");
		exit();
	}
	int time = atoi(argv[1]);
	printf("%d",time);
	sleep(time);
	exit();
}
