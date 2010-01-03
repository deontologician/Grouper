CFLAGS = -std=gnu99 -pipe -combine
DEBUG_CFLAGS= -ggdb3 -Wall -Wextra -DDEBUG #debug
PROFILE_CFLAGS = -O3 -march=core2 -ggdb3
RELEASE_CFLAGS= -O3 -march=core2
LIBS= -lm -lpthread -ltcmalloc
PROF_LIBS = -lprofiler
NAMES=bitmask bitmask.profile bitmask.debug h1 h1.debug h1.profile makerules 
CC=gcc

all: $(NAMES)

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

lex.yy.c: regex.l regex.tab.h
	flex regex.l
	@echo "Patching flex file to avoid warnings..."
	perl -pwli -e 's/^(\t\t)int( n; \\)$$/$$1size_t$$2/ if /^#define YY_INPUT/ .. /^#endif/' $@
regex.tab.c: printing.h regex.y
	bison -d regex.y
regex.tab.h: regex.tab.c
	touch regex.tab.h
h1: heur1.c regex.tab.c lex.yy.c printing.c printing.h util.h
	$(CC) $(CFLAGS) $(RELEASE_CFLAGS) $(LIBS) heur1.c printing.c -o $@ -lfl
h1.debug: h1
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) $(LIBS) heur1.c printing.c -o $@ -lfl
h1.profile: h1
	$(CC) $(CFLAGS) $(PROFILE_CFLAGS) $(LIBS) heur1.c printing.c -o $@ -lfl

#utility targets
clean:
	@-rm *~ *.o $(NAMES) *.tab.c lex.yy.c 2> /dev/null

#hack to get flymake mode to work with emacs 23
.FLYMAKE-HACK: check-syntax
check-syntax:
	$(CC) $(CFLAGS) -fsyntax-only $(CHK_SOURCES)
