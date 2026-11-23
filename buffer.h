#ifndef BUFFER_H
#define BUFFER_H
typedef struct buffer{
	char *data;
	int len;
	int cap;
}BUF;
void initbuf(BUF *b);
void growbuf(BUF *b,int extra);
void addchar(BUF *b,char c);
void addn(BUF *b,char *s,int n);
void addstr(BUF *b,char *s);
void freebuf(BUF *b);
char *copystr(char *s,int n);
#endif
