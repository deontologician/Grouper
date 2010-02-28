CFLAGS = -std=gnu99 -pipe -combine
DEBUG_CFLAGS = -ggdb3 -Wall -Wextra -DDEBUG
RELEASE_CFLAGS = -O3
FLLIBS = -lm -lpthread 
NAME = grouper
CC = gcc

all: release debug pol_gen

debug: $(NAME).debug
$(NAME).debug: grouper.c grouper.h xtrapbits.h printing.c printing.h
	@echo Making debug version...
	$(CC) $(FLLIBS) $(CFLAGS) $(DEBUG_CFLAGS) -o grouper.debug $(NAME).c printing.c

release: $(NAME)
$(NAME): $(NAME).c $(NAME).h xtrapbits.h printing.c printing.h
	@echo Making release version...
	$(CC) $(FLLIBS) $(CFLAGS) $(RELEASE_CFLAGS) -o $(NAME) $(NAME).c printing.c

pol_gen: pol_gen.c
	@echo Making policy generator...
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) $@.c -o $@

#utility targets
clean:
	@-rm *~ $(NAME) $(NAME).debug pol_gen 2> /dev/null

#This allows flymake mode to work with emacs 23
.PHONY: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)