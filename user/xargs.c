#include"user/user.h"
#define BUF_SIZE 512
int main(int argc,char* argv[])
{	
	char buffer[BUF_SIZE];
	char new_argv[32][32];
	char *pointer_argv[32];
	int argv_cnt=0;
	while(strlen(gets(buffer,BUF_SIZE)))
	{
		for(argv_cnt=0;argv_cnt<argc-1;argv_cnt++)
		{
			strcpy(new_argv[argv_cnt],argv[argv_cnt+1]);
		}
		int cur1=0,cur2=0;
		while(buffer[cur2]!='\n'&&buffer[cur2]!='\r')
		{
			if(buffer[cur1]==' ')
			{
				cur2=0;
				argv_cnt++;
			}
			new_argv[argv_cnt][cur2++]=buffer[cur1++];
		}
		argv_cnt++;
		for(int i=0;i<argv_cnt;i++) pointer_argv[i]=new_argv[i];
		if(fork()==0)
		{
			exec(new_argv[0],pointer_argv);
			exit();
		}
		wait();
		memset(new_argv,0,sizeof(new_argv));
		if(argv_cnt==0)
		{
			printf("mother fucker argv wrong\n");
			exit();
		}
	}
	exit();
}
