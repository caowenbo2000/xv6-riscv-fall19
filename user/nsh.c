#include"user/user.h"
#include"kernel/fcntl.h"
#define BUF_SIZE 512
#define MAX_ARG 10
//#define DEBUG
/*
features:
exe cmd input and output
use pipe with |
use argv
use redirection  with <>
*/
/*
2 types:pipe and exec
1 is exe
2 is pipe
*/
struct cmd
{
	int type;
	int left; //left node
	int right; //right node
	char * arg[MAX_ARG];
	char * earg[MAX_ARG];
	char *in_s;
	char *end_in_s;
	char *out_s;
	char *end_out_s;
}mycmd[20];
int cnt=0;

char firstch(char **ps,char *es);
void getword(char **sp,char *es,char **startp,char **endp);
char *find(char *s,char *e,char ch);
int parsecmd(char *s,char *es);
int parseexecmd(char *s,char *es);
void runcmd(int num);
void handle_buffer();

int main()
{
	char buffer[BUF_SIZE];
	while(1)
	{
		cnt=0;
		memset(buffer,0,sizeof(buffer));
		memset(mycmd,0,sizeof(mycmd));
		printf("@ ");
		gets(buffer,BUF_SIZE);
		if(*buffer==0)
		{
			//fprintf(2,"fuck, i cant read\n");
			exit(0);
		}
		char *first = buffer;
		char *end = buffer + strlen(buffer);
		end--;
		*end='\0';
		#ifdef DEBUG
		fprintf(2,"the cmd is: %s",buffer);
		#endif
		if(buffer[0]=='c'&&buffer[1]=='d'&&buffer[2]==' ')
		{
			first++;
			first++;
			firstch(&first,end);
			char *tmp_first;
			char *tmp_end;
			getword(&first,end,&tmp_first,&tmp_end);
			*tmp_end='\0';
			chdir(tmp_first);
		}
		parsecmd(first,end);
		#ifdef DEBUG
		fprintf(2,"cnt:%d\n",cnt);
		#endif
		handle_buffer();
		runcmd(1);
	}
}
char firstch(char **ps,char *es)
{
	char *s = *ps;
	while(s<es&&*s==' ') s++;
	*ps = s;
	return *s;
}
void getword(char **sp,char *es,char **startp,char **endp)
{
	char *s=*sp;
	char *start=*startp;
	while(*s==' '||*s=='<'||*s=='>') s++;
	start = s;
	while(s<es)
	{
		if(*s=='|'||*s==' '||*s=='>'||*s=='<'||*s=='\0')
		{
			*startp=start;
			*endp=s;
			*sp=s;
			return;	
		}
		s++;
	}
	*startp=start;
	*endp=s;
	*sp=s;
	return;
}
char *find(char *s,char *e,char ch)
{
	while(s<=e)
	{
		if(*s==ch)
		{
			return s;
		}
		s++;
	}
	return 0;
}
int parsecmd(char *s,char *es)
{
	char *pos;
	if(find(s,es,'|'))
	{
		pos=find(s,es,'|');
		cnt++;
		int tmp=cnt;
		mycmd[tmp].type=2;
		pos--;
		mycmd[tmp].left=parsecmd(s,pos);
		pos++;
		pos++;
		mycmd[tmp].right=parsecmd(pos,es);
		return cnt;
	}
	else
	{
		return parseexecmd(s,es);
	}
}
int parseexecmd(char *s,char *es)
{
	int argc=0;
	char *first,*end;
	cnt++;
	mycmd[cnt].type=1;
	while(s<es)
	{
		#ifdef DEBUG
		fprintf(2,"%d %d\n",s,es);
		#endif
		char ch=firstch(&s,es);
		//if(ch=='\0') return cnt;
		if(ch=='<')
		{
			getword(&s,es,&first,&end);
			mycmd[cnt].in_s=first;
			mycmd[cnt].end_in_s=end;
			continue;
		}
		if(ch=='>')
		{	
			getword(&s,es,&first,&end);
			mycmd[cnt].out_s=first;
			mycmd[cnt].end_out_s=end;
			continue;
		}
		getword(&s,es,&first,&end);
		#ifdef DEBUG
		fprintf(2,"%d %d\n",s,es);
		#endif
		mycmd[cnt].arg[argc]=first;
		mycmd[cnt].earg[argc]=end;
		#ifdef DEBUG
		fprintf(2,"first:%d end:%d\n",first,end);
		#endif
		argc++;
	}
	return cnt;
}

void runcmd(int num)
{
	if(mycmd[num].type==1)
	{
		if(fork()==0)
		{
			if(mycmd[num].in_s)//redir stdin
			{
				#ifdef DEBUG
				fprintf(2,"stdin redir\n");
				#endif
				close(0);
				int fd=open(mycmd[num].in_s,O_RDONLY);
				dup(fd);
			}
			if(mycmd[num].out_s)
			{
				#ifdef DEBUG
				fprintf(2,"stdout redir\n");
				#endif
				close(1);
				int fd=open(mycmd[num].out_s,O_WRONLY|O_CREATE);
				dup(fd);
			}
			#ifdef DEBUG
			fprintf(2,"none redir\n");
			fprintf(2,"exe:%s\n",mycmd[num].arg[0]);
			#endif
			if(exec(mycmd[num].arg[0],mycmd[num].arg)==-1)
			{
				fprintf(2,"Danm , exe failed\n");
			}
		}
		wait(0);
	}
	else
	{
		int fd[2];
		pipe(fd);
		if(fork()==0)
		{
			#ifdef DEBUG
				fprintf(2,"left:%d\n",mycmd[num].left);
			#endif
			close(1);
			close(fd[0]);
			dup(fd[1]);
			runcmd(mycmd[num].left);
			exit(0);
		}
		wait(0);
		if(fork()==0)
		{
			#ifdef DEBUG
				fprintf(2,"right:%d\n",mycmd[num].right);
			#endif
			close(0);
			close(fd[1]);
			dup(fd[0]);
			runcmd(mycmd[num].right);
			exit(0);
		}
		close(fd[0]);
		close(fd[1]);
		wait(0);
	}
}
void handle_buffer()
{
	for(int i=1;i<=cnt;i++)
	{
		if(mycmd[i].type==2)continue;
		int j=0;
		while(mycmd[i].earg[j])
		{
			#ifdef DEBUG
				fprintf(2,"j:%d\n",j);
				fprintf(2,"%s\n",mycmd[i].arg[j]);
			#endif
			*mycmd[i].earg[j]='\0';
			j++;
		}
		if(mycmd[i].in_s)
		{
			*mycmd[i].end_in_s='\0';
		}
		if(mycmd[i].out_s)
		{
			*mycmd[i].end_out_s='\0';
		}
	}
	return;
}
/*
ls a|grep x|grep a|grep d
|<>a \0
*/


