#include"user/user.h"
#define BUF_SIZE 10
//void itoa(int number,char *buf)
//{
//	char tmp[BUF_SIZE];
//	int cnt=0;
//	while(number)
//	{
//		tmp[cnt]=number%10+'0';
//		number/=10;
//		cnt++;
//	}
//	for(int i=0;i<cnt;i++)
//	{
//		buf[0]=tmp[cnt-1-i];
//	}
//	return ;
//}
int main(int argc,char *argv[])
{
	if(argc!=1)
	{
		printf("no argrue");
		exit();
	}
	int buffer_num=0;
	int parent_fd[2];//communicate with parents
	int child_fd[2]; //communicate with child
	pipe(child_fd);
	int thread_num=2;
	if(fork()==0)
	{
		close(child_fd[0]);
		//first thread
		printf("prime 2\n");
		for(int i=3;i<=35;i++)
		{
			if(i%2)
			{
				write(child_fd[1],&i,sizeof(i));
			}
		}
		exit();
	}
	//	print p
	// loop:
	//     n = get a number from left neighbor
	//	     if (p does not divide n)
	//		         send n to right neighbor
	else
	{
		//while
		while(1)
		{//child thread
			parent_fd[0]=child_fd[0];
			close(child_fd[1]);
			pipe(child_fd);
			buffer_num=0;
			if(read(parent_fd[0],&buffer_num,sizeof(buffer_num))==0)
			{
				exit();
			}
			//printf("sta=%d %d\n",sta,parent_fd[0]);
			thread_num=buffer_num;
			//int pid=getpid();
			printf("prime %d\n",thread_num);
			if(fork())//parent thread
			{
				close(child_fd[0]);
				while(read(parent_fd[0],&buffer_num,sizeof(buffer_num))==4)
				{
					if(buffer_num%thread_num)
					{
						//pid=getpid();
						//printf("write buffer_num=%d thread_num=%d,pid=%d\n",buffer_num,thread_num,pid);
						write(child_fd[1],&buffer_num,sizeof(buffer_num));
					}
				}
				exit();
			}
		}
	}
	exit();
}
