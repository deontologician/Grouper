# Copyright (c) 2009-2010 University of South Florida
# Created by Josh Kuhn, Jay Ligatti, Chris Gage
# All rights reserved.

# This file is part of Grouper.
#
#    Grouper is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Grouper is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with Grouper.  If not, see <http://www.gnu.org/licenses/>.

CFLAGS = -std=gnu99 -pipe -combine
DEBUG_CFLAGS = -ggdb3 -Wall -Wextra -DDEBUG
# -DNDEBUG compiles out assertions
RELEASE_CFLAGS = -O3 -march=core2 -DNDEBUG
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