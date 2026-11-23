#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"macro_handler.h"
#define MAXP 10
#define NAMELEN 64
#define MAXHIDE 64
typedef struct macro{
	char name[NAMELEN];
	char params[MAXP][NAMELEN];
	int nparams;
	int isfunc;
	char *body;
	struct macro *next;
}MAC;
MAC *head=0;
void expand(char *s,BUF *out,char *hide[],int nhide);
int isidstart(char c){
	if((c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_')
		return 1;
	return 0;
}
int isidchar(char c){
	if(isidstart(c)||(c>='0'&&c<='9'))
		return 1;
	return 0;
}
int isdig(char c){
	if(c>='0'&&c<='9')
		return 1;
	return 0;
}
int isspc(char c){
	if(c==' '||c=='\t'||c=='\r'||c=='\n')
		return 1;
	return 0;
}
int idlen(char *s){
	int n;
	if(isidstart(s[0])==0)
		return 0;
	n=0;
	while(isidchar(s[n]))
		n++;
	return n;
}
int numlen(char *s){
	int n;
	n=0;
	while(isidchar(s[n])||s[n]=='.')
		n++;
	return n;
}
int literallen(char *s){
	char q;
	int i;
	q=s[0];
	i=1;
	while(s[i]!='\0'){
		if(s[i]=='\\'&&s[i+1]!='\0')
			i=i+2;
		else if(s[i]==q)
			return i+1;
		else
			i++;
	}
	return i;
}
char *skipblank(char *s){
	while(*s==' '||*s=='\t')
		s++;
	return s;
}
char *trimcopy(char *s,int n){
	while(n>0&&isspc(*s)){
		s++;
		n--;
	}
	while(n>0&&isspc(s[n-1]))
		n--;
	return copystr(s,n);
}
int samename(char *a,char *name,int len){
	if(strncmp(a,name,len)!=0)
		return 0;
	if(a[len]!='\0')
		return 0;
	return 1;
}
MAC *findmac(char *name,int len){
	MAC *t;
	t=head;
	while(t){
		if(samename(t->name,name,len))
			return t;
		t=t->next;
	}
	return 0;
}
int paramindex(MAC *m,char *name,int len){
	int i;
	for(i=0;i<m->nparams;i++){
		if(samename(m->params[i],name,len))
			return i;
	}
	return -1;
}
void deletemac(char *name,int len){
	MAC *del,*prev;
	del=head;
	prev=0;
	while(del){
		if(samename(del->name,name,len)){
			if(prev==0)
				head=del->next;
			else
				prev->next=del->next;
			free(del->body);
			free(del);
			return;
		}
		prev=del;
		del=del->next;
	}
}
int getparams(char **cursor,MAC *m){
	char *p;
	int n;
	p=skipblank(*cursor+1);
	if(*p==')'){
		*cursor=p+1;
		return 0;
	}
	while(1){
		p=skipblank(p);
		n=idlen(p);
		if(n==0||n>=NAMELEN||m->nparams>=MAXP)
			return -1;
		memcpy(m->params[m->nparams],p,n);
		m->params[m->nparams][n]='\0';
		m->nparams++;
		p=skipblank(p+n);
		if(*p==','){
			p++;
		}
		else if(*p==')'){
			*cursor=p+1;
			return 0;
		}
		else{
			return -1;
		}
	}
}
int adddefine(char *def){
	MAC *new;
	char *p;
	int n;
	n=idlen(def);
	if(n==0||n>=NAMELEN)
		return -1;
	new=malloc(sizeof(MAC));
	if(new==0){
		printf("out of memory\n");
		exit(1);
	}
	memcpy(new->name,def,n);
	new->name[n]='\0';
	new->nparams=0;
	new->isfunc=0;
	new->body=0;
	p=def+n;
	if(*p=='('){
		new->isfunc=1;
		if(getparams(&p,new)<0){
			free(new);
			return -1;
		}
	}
	new->body=trimcopy(p,strlen(p));
	deletemac(def,n);
	new->next=head;
	head=new;
	return 0;
}
void undefmacro(char *name){
	int n;
	n=idlen(name);
	if(n>0)
		deletemac(name,n);
}
int ismacro(char *name){
	int n;
	n=idlen(name);
	if(n==0)
		return 0;
	if(findmac(name,n)==0)
		return 0;
	return 1;
}
void freemacros(){
	MAC *del;
	del=head;
	while(del){
		head=del->next;
		free(del->body);
		free(del);
		del=head;
	}
}
int collectargs(char *s,char *args[],int *count){
	int i,depth,start,n,k;
	i=1;
	depth=1;
	start=1;
	n=0;
	while(s[i]!='\0'){
		if(s[i]=='"'||s[i]=='\''){
			i=i+literallen(s+i);
			continue;
		}
		if(s[i]=='('){
			depth++;
		}
		else if(s[i]==')'){
			depth--;
			if(depth==0){
				if(n>=MAXP)
					break;
				args[n]=trimcopy(s+start,i-start);
				n++;
				*count=n;
				return i+1;
			}
		}
		else if(s[i]==','&&depth==1){
			if(n>=MAXP)
				break;
			args[n]=trimcopy(s+start,i-start);
			n++;
			start=i+1;
		}
		i++;
	}
	for(k=0;k<n;k++)
		free(args[k]);
	return 0;
}
int ishidden(char *name,char *hide[],int nhide){
	int i;
	for(i=0;i<nhide;i++){
		if(strcmp(hide[i],name)==0)
			return 1;
	}
	return 0;
}
void expandwith(char *text,MAC *m,BUF *out,char *hide[],int nhide){
	char *next[MAXHIDE];
	int i;
	if(nhide>=MAXHIDE){
		addstr(out,text);
		return;
	}
	for(i=0;i<nhide;i++)
		next[i]=hide[i];
	next[nhide]=m->name;
	expand(text,out,next,nhide+1);
}
void substitute(MAC *m,char *args[],BUF *out){
	char *b;
	int i,n,k;
	b=m->body;
	i=0;
	while(b[i]!='\0'){
		if(b[i]=='"'||b[i]=='\''){
			n=literallen(b+i);
			addn(out,b+i,n);
			i=i+n;
		}
		else if(isdig(b[i])){
			n=numlen(b+i);
			addn(out,b+i,n);
			i=i+n;
		}
		else if(isidstart(b[i])){
			n=idlen(b+i);
			k=paramindex(m,b+i,n);
			if(k>=0)
				addstr(out,args[k]);
			else
				addn(out,b+i,n);
			i=i+n;
		}
		else{
			addchar(out,b[i]);
			i++;
		}
	}
}
int callfunc(MAC *m,char *s,BUF *out,char *hide[],int nhide){
	char *args[MAXP];
	int nargs,i,used,valid;
	BUF e,body;
	nargs=0;
	used=collectargs(s,args,&nargs);
	if(used==0)
		return 0;
	valid=0;
	if(nargs==m->nparams)
		valid=1;
	if(m->nparams==0&&nargs==1&&args[0][0]=='\0')
		valid=1;
	if(valid==0){
		printf("macro %s expects %d argument(s) but got %d\n",m->name,m->nparams,nargs);
		for(i=0;i<nargs;i++)
			free(args[i]);
		return 0;
	}
	if(m->nparams>0){
		for(i=0;i<nargs;i++){
			initbuf(&e);
			expand(args[i],&e,hide,nhide);
			free(args[i]);
			args[i]=e.data;
		}
	}
	initbuf(&body);
	substitute(m,args,&body);
	expandwith(body.data,m,out,hide,nhide);
	freebuf(&body);
	for(i=0;i<nargs;i++)
		free(args[i]);
	return used;
}
void expand(char *s,BUF *out,char *hide[],int nhide){
	int i,j,n,used;
	MAC *m;
	i=0;
	while(s[i]!='\0'){
		if(s[i]=='"'||s[i]=='\''){
			n=literallen(s+i);
			addn(out,s+i,n);
			i=i+n;
		}
		else if(isdig(s[i])){
			n=numlen(s+i);
			addn(out,s+i,n);
			i=i+n;
		}
		else if(isidstart(s[i])){
			n=idlen(s+i);
			m=findmac(s+i,n);
			if(m==0||ishidden(m->name,hide,nhide)){
				addn(out,s+i,n);
				i=i+n;
			}
			else if(m->isfunc==0){
				expandwith(m->body,m,out,hide,nhide);
				i=i+n;
			}
			else{
				j=i+n;
				used=0;
				while(s[j]==' '||s[j]=='\t')
					j++;
				if(s[j]=='(')
					used=callfunc(m,s+j,out,hide,nhide);
				if(used==0){
					addn(out,s+i,n);
					i=i+n;
				}
				else{
					i=j+used;
				}
			}
		}
		else{
			addchar(out,s[i]);
			i++;
		}
	}
}
void expandline(char *line,BUF *out){
	expand(line,out,0,0);
}
