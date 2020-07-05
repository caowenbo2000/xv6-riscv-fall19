#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
static char *pattern;
char*
fmtname(char *path)
{
  static char buf[DIRSIZ+1];
  char *p;
	
  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
  return buf;
}

void
find(char *path)
{
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;

  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
		if(strcmp(fmtname(path),pattern)==0)
		{
			printf("%s\n",path);
		}
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
			//printf("%s %s %d\n",buf,fmtname(buf),strlen(buf));
			if(strcmp(fmtname(buf),pattern)==0)
			{
      	printf("%s\n",buf);
      }
			if(st.type==T_DIR&&strcmp(fmtname(buf),".")&&strcmp(fmtname(buf),".."))
			{
				find(buf);
			}
		}
    break;
  }
  close(fd);
}

int
main(int argc, char *argv[])
{
	pattern=argv[2];
  if(argc != 3){
		printf("Usage: 1 find dir 2 pattern string\n");
    exit();
  }
  find(argv[1]);  //find int this dir
  exit();
}
