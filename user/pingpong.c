#include<user/user.h>
#include<kernel/types.h>
#define BUF_SIZE 10
int main(int argc,char *argv[])
{
	if(argc!=1)
	{
		printf("need one argrue");
		exit();
	}
	int parent_fd[2];
	int child_fd[2];
	pipe(parent_fd);
	pipe(child_fd);
	char ping[10]="ping";
	char pong[10]="pong";
	char buffer[BUF_SIZE];
	int this_id;
	if(fork()==0)   //child thread
	{
		this_id=getpid();
		read(parent_fd[0],buffer,BUF_SIZE);
		printf("%d: received %s\n",this_id,buffer);
		write(child_fd[1],pong,strlen(pong));
		exit();
	}
	this_id=getpid();
	write(parent_fd[1],ping,strlen(ping));
	read(child_fd[0],buffer,BUF_SIZE);
	printf("%d: received %s\n",this_id,buffer);
	exit();
	//parent thread
}
