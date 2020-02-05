#
#  ----------------------------------------------------
#  vsm - vector space model data similarity
#  ----------------------------------------------------
#
#  Copyright (c) 2008 Jason Bittel <jason.bittel@gmail.com>
#

CC		= gcc
CCFLAGS		= -Wall -O3 -funroll-loops -ansi
DEBUGFLAGS	= -Wall -g -DDEBUG -ansi
LIBS		= -lm
PROG		= vsm
FILES		= main.c index.c stem.c

all: $(PROG)

$(PROG): $(FILES)
	$(CC) $(CFLAGS) -o $(PROG) $(FILES) $(LIBS)

debug: $(FILES)
	$(CC) $(DEBUGFLAGS) -o $(PROG) $(FILES) $(LIBS)

clean:
	rm -f $(PROG)
