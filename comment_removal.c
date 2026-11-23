#include<stdio.h>
#include"buffer.h"
#include"comment_removal.h"
void addnewline(BUF *b){
	while(b->len>0&&(b->data[b->len-1]==' '||b->data[b->len-1]=='\t'))
		b->len--;
	b->data[b->len]='\0';
	addchar(b,'\n');
}
int copyliteral(char *s,BUF *out){
	char q;
	int i;
	q=s[0];
	i=1;
	addchar(out,q);
	while(s[i]!='\0'&&s[i]!='\n'){
		addchar(out,s[i]);
		if(s[i]=='\\'&&s[i+1]!='\0'&&s[i+1]!='\n'){
			i++;
			addchar(out,s[i]);
		}
		else if(s[i]==q){
			return i+1;
		}
		i++;
	}
	return i;
}
char *removecomments(char *src){
	BUF out;
	int i;
	char c;
	initbuf(&out);
	i=0;
	while(src[i]!='\0'){
		c=src[i];
		if(c=='"'||c=='\''){
			i=i+copyliteral(src+i,&out);
		}
		else if(c=='/'&&src[i+1]=='/'){
			i=i+2;
			while(src[i]!='\0'&&src[i]!='\n')
				i++;
		}
		else if(c=='/'&&src[i+1]=='*'){
			i=i+2;
			while(src[i]!='\0'&&!(src[i]=='*'&&src[i+1]=='/')){
				if(src[i]=='\n')
					addnewline(&out);
				i++;
			}
			if(src[i]!='\0')
				i=i+2;
			addchar(&out,' ');
		}
		else if(c=='\n'){
			addnewline(&out);
			i++;
		}
		else{
			addchar(&out,c);
			i++;
		}
	}
	return out.data;
}
