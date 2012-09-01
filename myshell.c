#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include<sys/types.h>
#include<signal.h>
#include<wait.h>
#include<fcntl.h>

/**************************************************************/

void execute_external(char input[],char *arr[]);
void changedir(char *arr[]);
void prompt();
void parse(char input[],char *arr[]);
void printhist();
void histn(int x);
void exec_histn(int x,char input[],char *arr[],pid_t main_pid);
void execmain(char input[],char *arr[],pid_t main_pid);
void child_handler(int signum);
void signal_handler(int signum);

/***************************************************************/

typedef struct node
{
	char name[1000];
	int status;
	int pid;
}node;

node hist[1000];
node back[1000];
node all[1000];

int parse_count=0;
int glob_count=1;
int back_count=0;
int all_count=0;
int flag=0;

char *MYHOME;

int main ()
{
	signal(SIGINT,SIG_IGN);
	signal(SIGINT,signal_handler);
	signal(SIGCHLD,SIG_IGN);
	signal(SIGCHLD,child_handler);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGTSTP,signal_handler);
	signal(SIGQUIT,SIG_IGN);
	signal(SIGQUIT,signal_handler);

	MYHOME=getenv("PWD");
	pid_t main_pid=getpid();
	while(1)
	{
		prompt();
		int i=0,spaces=0,temp=0;
		char input[1000]={'\0'},ch,space_removed_input[1000]={0};
		char *arr[1000];
		scanf("%c",&ch);
		while(ch!='\n')
		{
			if(ch=='&')
			{
				scanf("%c",&ch);
				flag=1;
				i--;
				continue;
			}
			input[i++]=ch;
			scanf("%c",&ch);
		}
		while(input[spaces]==' '|| input[spaces]=='\t')
			spaces++;
		while(input[temp]!='\0')
			space_removed_input[temp++]=input[spaces++];
		input[i]='\0';
		space_removed_input[temp]='\0';
		strcpy(input,space_removed_input);
		if(strcmp(hist[glob_count-1].name,input)!=0)
			strcpy(hist[glob_count++].name,input);
		char *input2=(char*)malloc(sizeof(strlen(input)+1));
		strcpy(input2,input);
		if(strcmp(input2,"\0"))
		{
			parse(input2,arr);
			execmain(input,arr,main_pid);
		}
		parse_count=0;
		flag=0;
	}
	glob_count=1;
	back_count=0;
	return 0;
}

void execmain(char input[],char *arr[],pid_t main_pid)
{
	int counter=0;
	while(arr[counter]!=NULL)
	{
		if(!strcmp(arr[counter],"&"))	
		{
			arr[counter]=NULL;
			flag=1;
			break;
		}
		else
			counter++;
	}
	if (strcmp(arr[0],"quit")==0)
		exit(0);
	else if(arr[0]==NULL)
		prompt();
	else if(strcmp(arr[0],"cd")==0)
		changedir(arr);
	else if(strcmp(arr[0],"hist")==0)
		printhist();
	else if((strcmp(arr[0],"pid")==0) && parse_count-1==1)
		printf("./a.out process id: %d\n",main_pid);
	else if(strncmp(arr[0],"hist",4)==0 && arr[0][4]!='\0')
	{
		char *start=(char*)malloc(sizeof(strlen(start)+1));
		start=arr[0]+4;
		start[strlen(start)]='\0';
		histn(atoi(start));
	}
	else if(strncmp(arr[0],"!hist",5)==0 )
	{
		char *start=(char*)malloc(sizeof(strlen(start)+1));
		start=arr[0]+5;
		start[strlen(start)]='\0';
		exec_histn(atoi(start),input,arr,main_pid);
	}
	else if(strcmp(arr[0],"pid")==0 && strcmp(arr[1],"current")==0)
	{
		int i=0;
		printf("List of currently executing processes spawned from this shell:\n");
		for(i=0;i<back_count;i++)
			if(back[i].status==1)
				printf("command name: %s process id: %d\n",back[i].name,back[i].pid);
	}
	else if(strcmp(arr[0],"pid")==0 && strcmp(arr[1],"all")==0)
	{
		int i=0;
		printf("List of all processes spawned from this shell:\n");
		for(i=0;i<all_count;i++)
			printf("command name: %s process id: %d\n",all[i].name,all[i].pid);
	}
	
	else
	{
		if(flag==1)
			strcat(input,"&");
		execute_external(input,arr);
	}
	return;
}

/**************************************************************/

void parse(char input[],char *arr[])
{
	char *token=strtok(input," ");
	while(token!=NULL)
	{
		token[strlen(token)]='\0';
		arr[parse_count]=token;
		parse_count++;
		token=strtok(NULL," ");
	}
	arr[parse_count++]=token;
	return;
}

/***************************************************************/

void execute_external(char input[],char *arr[])
{
	pid_t p=fork();
	if(flag==0)
	{
		if(p<0)
		{
			perror("unable to fork");
			exit(1);
		}
		else if (p==0)
		{
			if(execvp(arr[0],arr)<0)
			{
				perror("command not found ... Is it in your path ?");
				_exit(0);
			}
		}
		else
		{
				all[all_count].pid=p;
				strcpy(all[all_count].name,input);
				all[all_count++].status=0;
				wait(NULL);
		}
	}
	else if (flag==1)
	{
		if(p<0)
		{
			perror("unable to fork");
			exit(1);
		}
		else if (p==0)
		{
			if(execvp(arr[0],arr)<0)
			{
				perror("maybe an in-built command");
				_exit(0);
			}
		}
		else
		{
			strcpy(back[back_count].name,input);
			back[back_count].pid=p;
			back[back_count++].status=1;

			all[all_count].pid=p;
			strcpy(all[all_count].name,input);
			all[all_count++].status=1;
		}
	}
}

/****************************************************************/

void prompt()
{	
	char *username,*hostname,*cwd=NULL,finalpath[1000];
	strcpy (finalpath,"~");
	username=getenv("USERNAME");
	hostname=getenv("HOSTNAME");
	cwd = getcwd(cwd,1000);
	char *path=strstr(cwd,MYHOME);
	path=path+strlen(MYHOME);
	strcat(finalpath,path);
	printf("%s@%s:%s> ",username,hostname,finalpath);
	return;
}

/****************************************************************/

void changedir(char *arr[])
{
	char *cwd=NULL;
	char currpath[1000],tokens[100];
	if( arr[1]!=NULL ){
		strcpy(tokens,arr[1]);
		tokens[strlen(arr[1])]=0;
	}
	int ret,len=0;
	if ((arr[1]==NULL) || (!strcmp(arr[1],"~")) || (!strcmp(arr[1],"~/")))
	{
		ret = chdir(MYHOME);
		if( ret!=0 )
			perror("chdir()");
		return;
	}
	cwd = getcwd(cwd,1000);
	len = strlen(cwd);
	strcpy(currpath,cwd);
	strcat(currpath,"/");
	len = len + strlen(tokens) +1;
	if( tokens!=NULL )
		strcat( currpath, arr[1]);
	currpath[len]=0;
	if (chdir(currpath) != 0)
		perror("chdir()");
	else
		return;
}

/****************************************************************/

void printhist()
{
	int i=0;
	for(i=1;i<glob_count;i++)
		printf("%d. %s\n",i,hist[i].name);
	return;
}

/****************************************************************/

void histn(int x)
{
	int i,j=1;
	if(glob_count-1>x)
		for(i=glob_count-x;i<=glob_count-1;i++)
			printf("%d. %s\n",j++,hist[i].name);
	else
		printhist();
}

/****************************************************************/

void exec_histn(int x,char input[],char *arr[],pid_t main_pid)
{
	parse_count=0;
	strcat(hist[x].name,"\0");
	parse(hist[x].name,arr);
	execmain(hist[x].name,arr,main_pid);
	return;
}

/****************************************************************/

void child_handler(int signum)
{
	int p,i=0,count=0;
	p = waitpid(WAIT_ANY, NULL, WNOHANG);
	for(i=0;i<back_count;i++)
	{
		if(back[i].pid==p)
		{
			int j=0;
			char temp[100];
			for(j=0;back[i].name[j]!='&';j++)
			{
				temp[count++]=back[i].name[j];
			}
			temp[count]='\0';
			fflush(stdout);
			printf("\n%s %d exited normally\n",temp,back[i].pid);
			prompt();
			fflush(stdout);
			back[i].status=0;
		}
	}
	signal(SIGCHLD, child_handler);
	return;
}

/*************************************************************************/
void signal_handler(int signum)
{
	if(signum==2 || signum==20 || signum ==3)
	{
		fflush(stdout);
		printf("\n");
		prompt();
		fflush(stdout);
		signal(SIGINT, signal_handler);
		signal(SIGQUIT, signal_handler);
		signal(SIGTSTP,signal_handler);
	}
	return;
}

