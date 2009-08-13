CFLAGS= -Wall -O -g -Wextra -lrt -lpthread -std=c99
NAME=filter
CC=gcc
SRCS=filter.c
SPLINTARGS= +posixstrictlib -retvalint
HDRS= $(SRCS:.c=.h)
OBJS= $(SRCS:.c=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
%.o: %.c %.h
	$(CC) $(CFLAGS) -c $*.c

#utility targets
clean:
	@-rm *~ *.o $(NAME) 2> /dev/null
lint: all
	@-splint $(SPLINTARGS) $(SRCS) $(HDRS)
memcheck: all
	@-valgrind ./$(NAME) fake.tbl

#hack to get flymake mode to work with emacs 22
.FLYMAKE-HACK: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)
