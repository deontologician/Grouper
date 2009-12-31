CFLAGS = -std=gnu99 -pipe -combine
DEBUG_CFLAGS= -ggdb3 -Wall -Wextra -DDEBUG #debug
PROFILE_CFLAGS = -O3 -march=core2 -ggdb3
RELEASE_CFLAGS= -O3 -march=core2
LIBS= -lm -lpthread -ltcmalloc -lfl
PROF_LIBS = -lprofiler
NAME=tblcompile
CC=gcc

all: bitmask bitmask.profile bitmask.debug h1 h1.debug h1.profile makerules 

bitmask.profile: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@echo Making profile...
	$(CC) $(CFLAGS) $(PROFILE_CFLAGS) $(LIBS) $(PROF_LIBS) -o $@ tblcompile.c printing.c

bitmask.debug: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@echo Making debug...
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(LIBS) -o $@ tblcompile.c printing.c

bitmask: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@echo Making release...
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) $(LIBS) -o $@ tblcompile.c printing.c

makerules: makerules.c
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) makerules.c -o $@

lex.yy.c: regex.l
	flex regex.l
regex.tab.c: lex.yy.c printing.h regex.y
	bison regex.y
h1.debug: regex.c regex.tab.c printing.c printing.h util.h
	$(CC) $(CFLAGS) $(PROFILE_FLAGS) $(LIBS) regex.c printing.c -o $@ 

#utility targets
clean:
	@-rm *~ *.o $(NAME) 2> /dev/null

#hack to get flymake mode to work with emacs 23
.FLYMAKE-HACK: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)
