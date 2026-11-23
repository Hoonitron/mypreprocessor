#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"buffer.h"
#include"comment_removal.h"
#include"file_inclusion.h"
#include"macro_handler.h"
#include"preprocessor.h"
#define MAXDEPTH 32
#define MAXC 64
typedef struct cond{
	int active;
	int taken;
	int pass;
}COND;
typedef struct condstack{
	COND items[MAXC];
	int top;
}CS;
int processfile(char *path,FILE *out,int depth);
void putline(FILE *out,char *line){
	fputs(line,out);
	fputc('\n',out);
}
char *matchdir(char *line,char *word){
	int n;
	n=strlen(word);
	while(*line==' '||*line=='\t')
		line++;
	if(*line!='#')
		return 0;
	line++;
	while(*line==' '||*line=='\t')
		line++;
	if(strncmp(line,word,n)!=0)
		return 0;
	if(isidchar(line[n]))
		return 0;
	line=line+n;
	while(*line==' '||*line=='\t')
		line++;
	return line;
}
int isdirective(char *line){
	while(*line==' '||*line=='\t')
		line++;
	if(*line=='#')
		return 1;
	return 0;
}
int isactive(CS *st){
	if(st->top==0)
		return 1;
	return st->items[st->top-1].active;
}
int pushcond(CS *st,int cond,int pass){
	int parent;
	if(st->top>=MAXC)
		return -1;
	parent=isactive(st);
	st->items[st->top].pass=pass;
	st->items[st->top].taken=cond;
	if(pass)
		st->items[st->top].active=parent;
	else
		st->items[st->top].active=parent&&cond;
	st->top++;
	return 0;
}
int handlecond(char *line,CS *st,FILE *out,char *path){
	char *rest;
	int neg,cond,parent;
	COND *t;
	neg=0;
	t=0;
	if(st->top>0)
		t=&st->items[st->top-1];
	rest=matchdir(line,"ifdef");
	if(rest==0){
		rest=matchdir(line,"ifndef");
		neg=1;
	}
	if(rest){
		cond=ismacro(rest);
		if(neg)
			cond=!cond;
		if(pushcond(st,cond,0)<0){
			printf("%s: conditionals nested too deeply\n",path);
			return -1;
		}
		return 1;
	}
	if(matchdir(line,"if")){
		if(pushcond(st,1,1)<0){
			printf("%s: conditionals nested too deeply\n",path);
			return -1;
		}
		if(isactive(st))
			putline(out,line);
		return 1;
	}
	if(matchdir(line,"elif")){
		if(t==0){
			printf("%s: #elif without #if\n",path);
		}
		else if(t->pass){
			if(t->active)
				putline(out,line);
		}
		else{
			printf("%s: #elif is not supported after #ifdef/#ifndef\n",path);
			t->active=0;
		}
		return 1;
	}
	if(matchdir(line,"else")){
		if(t==0){
			printf("%s: #else without #if\n",path);
		}
		else if(t->pass){
			if(t->active)
				putline(out,line);
		}
		else{
			parent=1;
			if(st->top>1)
				parent=st->items[st->top-2].active;
			t->active=parent&&!t->taken;
			t->taken=1;
		}
		return 1;
	}
	if(matchdir(line,"endif")){
		if(t==0){
			printf("%s: #endif without #if\n",path);
			return 1;
		}
		if(t->pass&&t->active)
			putline(out,line);
		st->top--;
		return 1;
	}
	return 0;
}
int doinclude(char *rest,char *line,char *path,FILE *out,int depth){
	char *name,*found;
	int status;
	name=getname(rest);
	if(name==0){
		printf("%s: malformed #include\n",path);
		putline(out,line);
		return 0;
	}
	found=findfile(name,path);
	if(found==0){
		printf("warning: %s not found, directive left as is\n",name);
		putline(out,line);
		free(name);
		return 0;
	}
	status=processfile(found,out,depth+1);
	free(found);
	free(name);
	return status;
}
int processline(char *line,char *path,FILE *out,int depth,CS *st){
	char *rest;
	int h;
	BUF e;
	h=handlecond(line,st,out,path);
	if(h<0)
		return -1;
	if(h>0)
		return 0;
	if(isactive(st)==0)
		return 0;
	rest=matchdir(line,"include");
	if(rest)
		return doinclude(rest,line,path,out,depth);
	rest=matchdir(line,"define");
	if(rest){
		if(adddefine(rest)<0)
			printf("%s: invalid #define ignored\n",path);
		return 0;
	}
	rest=matchdir(line,"undef");
	if(rest){
		undefmacro(rest);
		return 0;
	}
	if(isdirective(line)){
		putline(out,line);
		return 0;
	}
	initbuf(&e);
	expandline(line,&e);
	putline(out,e.data);
	freebuf(&e);
	return 0;
}
char *nextline(char *p,BUF *line){
	int first,dir,n;
	char *end,*q;
	first=1;
	dir=0;
	while(1){
		end=strchr(p,'\n');
		if(end)
			n=end-p;
		else
			n=strlen(p);
		if(n>0&&p[n-1]=='\r')
			n--;
		if(first){
			q=p;
			while(*q==' '||*q=='\t')
				q++;
			dir=(*q=='#');
			first=0;
		}
		if(dir&&n>0&&p[n-1]=='\\'){
			addn(line,p,n-1);
			if(end==0)
				return p+strlen(p);
			p=end+1;
			if(*p=='\0')
				return p;
			continue;
		}
		addn(line,p,n);
		if(end)
			return end+1;
		return p+strlen(p);
	}
}
int processfile(char *path,FILE *out,int depth){
	char *raw,*clean,*p;
	CS st;
	BUF line;
	int status;
	status=0;
	if(depth>MAXDEPTH){
		printf("includes nested too deeply at %s\n",path);
		return -1;
	}
	raw=readfile(path);
	if(raw==0){
		printf("cannot open %s\n",path);
		return -1;
	}
	clean=removecomments(raw);
	free(raw);
	st.top=0;
	p=clean;
	while(*p!='\0'){
		initbuf(&line);
		p=nextline(p,&line);
		if(processline(line.data,path,out,depth,&st)<0)
			status=-1;
		freebuf(&line);
	}
	if(st.top!=0)
		printf("warning: unterminated conditional in %s\n",path);
	free(clean);
	return status;
}
int preprocess(char *path,FILE *out){
	return processfile(path,out,0);
}
