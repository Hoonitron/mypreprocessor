#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"buffer.h"
void initbuf(BUF *b){
	b->cap=64;
	b->len=0;
	b->data=malloc(b->cap);
	if(b->data==0){
		printf("out of memory\n");
		exit(1);
	}
	b->data[0]='\0';
}
void growbuf(BUF *b,int extra){
	if(b->len+extra+1<=b->cap)
		return;
	while(b->len+extra+1>b->cap)
		b->cap=b->cap*2;
	b->data=realloc(b->data,b->cap);
	if(b->data==0){
		printf("out of memory\n");
		exit(1);
	}
}
void addchar(BUF *b,char c){
	growbuf(b,1);
	b->data[b->len]=c;
	b->len++;
	b->data[b->len]='\0';
}
void addn(BUF *b,char *s,int n){
	growbuf(b,n);
	memcpy(b->data+b->len,s,n);
	b->len=b->len+n;
	b->data[b->len]='\0';
}
void addstr(BUF *b,char *s){
	addn(b,s,strlen(s));
}
void freebuf(BUF *b){
	free(b->data);
	b->data=0;
	b->len=0;
	b->cap=0;
}
char *copystr(char *s,int n){
	char *r;
	r=malloc(n+1);
	if(r==0){
		printf("out of memory\n");
		exit(1);
	}
	memcpy(r,s,n);
	r[n]='\0';
	return r;
}
