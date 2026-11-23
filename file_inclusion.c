#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"buffer.h"
#include"file_inclusion.h"
int exists(char *path){
	FILE *fp;
	fp=fopen(path,"r");
	if(fp==0)
		return 0;
	fclose(fp);
	return 1;
}
char *readfile(char *path){
	FILE *fp;
	BUF b;
	char chunk[1024];
	int n;
	fp=fopen(path,"rb");
	if(fp==0)
		return 0;
	initbuf(&b);
	while((n=fread(chunk,1,sizeof(chunk),fp))>0)
		addn(&b,chunk,n);
	fclose(fp);
	return b.data;
}
char *getname(char *rest){
	char end;
	char *stop;
	if(rest[0]=='<')
		end='>';
	else if(rest[0]=='"')
		end='"';
	else
		return 0;
	stop=strchr(rest+1,end);
	if(stop==0||stop==rest+1)
		return 0;
	return copystr(rest+1,stop-rest-1);
}
char *findfile(char *name,char *current){
	char *slash;
	int dirlen;
	BUF b;
	if(name[0]!='/'){
		slash=strrchr(current,'/');
		if(slash!=0){
			dirlen=slash-current+1;
			initbuf(&b);
			addn(&b,current,dirlen);
			addstr(&b,name);
			if(exists(b.data))
				return b.data;
			freebuf(&b);
		}
	}
	if(exists(name))
		return copystr(name,strlen(name));
	return 0;
}
