#DEBUG=-ggdb
LIBS=-L/usr/lib -lxml2
CFLAGS=-I/usr/include/libxml2 -o svnloganal -Wall $(DEBUG)
CC=gcc

svnloganal: logAnalyze.c
	$(CC) $(OBJECTS) $(LIBS) $(CFLAGS) logAnalyze.c pathtree.c stack.c

clean:
	rm -rf svnloganal
