CFLAGS = -std=gnu99 -pipe -combine
DEBUG_CFLAGS= -ggdb3 -Wall -Wextra -DDEBUG #debug
PROFILE_CFLAGS = -O3 -march=core2 -ggdb3
RELEASE_CFLAGS= -O3 -march=core2
LIBS= -lm -lpthread -ltcmalloc
PROF_LIBS = -lprofiler
NAME=tblcompile
CC=gcc

all: release profile debug makerules TAGS

profile: tblcompile.profile
tblcompile.profile: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@echo Making profile...
	$(CC) $(CFLAGS) $(PROFILE_CFLAGS) $(LIBS) $(PROF_LIBS) -o tblcompile.profile tblcompile.c printing.c

debug: tblcompile.debug
tblcompile.debug: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@echo Making debug...
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(LIBS) -o tblcompile.debug tblcompile.c printing.c

release: tblcompile
tblcompile: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@echo Making release...
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) $(LIBS) -o tblcompile tblcompile.c printing.c

makerules: makerules.c
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) makerules.c -o makerules

TAGS: tblcompile.c tblcompile.h xtrapbits.h printing.c printing.h
	@-etags.emacs $?

#utility targets
clean:
	@-rm *~ *.o $(NAME) 2> /dev/null

#hack to get flymake mode to work with emacs 23
.FLYMAKE-HACK: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)
