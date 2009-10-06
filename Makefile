CFLAGS= -Wall -Werror -ggdb3 -Wextra -std=gnu99 -DDEBUG #debug
#CFLAGS= -O3 -std=gnu99#for release
LIBS= -lm
NAME=tblcompile
CC=gcc
SRCS1=tblcompile.c	
SPLINTARGS= +posixstrictlib -retvalint
HDRS= $(SRCS:.c=.h)
OBJS= $(SRCS:.c=.o)

all: tblcompile TAGS

tblcompile: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	$(CC) $(CFLAGS) $(LIBS) -o $@ tblcompile.c printing.c

TAGS: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@-etags.emacs $?

#utility targets
clean:
	@-rm *~ *.o $(NAME) 2> /dev/null

#hack to get flymake mode to work with emacs 23
.FLYMAKE-HACK: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)
