CFLAGS= -Wall -ggdb3 -Wextra -std=c99 #debug
#CFLAGS= -O3 #for release
LIBS= -lm
NAME=filter
NAME2=tblcompile
CC=gcc
SRCS1=filter.c
SRCS2=tblcompile.c	
SPLINTARGS= +posixstrictlib -retvalint
HDRS= $(SRCS:.c=.h)
OBJS= $(SRCS:.c=.o)

all: filter tblcompile

tblcompile: tblcompile.c tblcompile.h
	$(CC) $(CFLAGS) $(LIBS) -o $@ tblcompile.c
filter: filter.c filter.h
	$(CC) $(CFLAGS) $(LIBS) -o $@ filter.c

#utility targets
clean:
	@-rm *~ *.o $(NAME) 2> /dev/null
lint: all
	@-splint $(SPLINTARGS) $(SRCS) $(HDRS)

#hack to get flymake mode to work with emacs 22
.FLYMAKE-HACK: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)
