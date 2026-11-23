#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include"buffer.h"
#include"macro_handler.h"
#include"preprocessor.h"
char *outname(char *input){
	char *slash,*base,*dot;
	int stem;
	BUF b;
	slash=strrchr(input,'/');
	if(slash)
		base=slash+1;
	else
		base=input;
	dot=strrchr(base,'.');
	if(dot&&dot!=base)
		stem=dot-input;
	else
		stem=strlen(input);
	initbuf(&b);
	addn(&b,input,stem);
	addstr(&b,".i");
	return b.data;
}
int main(int argc,char *argv[]){
	char *oname;
	FILE *fp;
	int status;
	if(argc!=2){
		printf("usage: %s inputfile.c\n",argv[0]);
		return 1;
	}
	oname=outname(argv[1]);
	if(strcmp(oname,argv[1])==0){
		printf("output file would overwrite the input file\n");
		free(oname);
		return 1;
	}
	fp=fopen(oname,"w");
	if(fp==0){
		printf("cannot create %s\n",oname);
		free(oname);
		return 1;
	}
	status=preprocess(argv[1],fp);
	if(fclose(fp)!=0)
		status=-1;
	if(status!=0)
		remove(oname);
	freemacros();
	free(oname);
	if(status==0)
		return 0;
	return 1;
}
