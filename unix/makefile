#
# Flatten
#
# UNIX
#

ERE =		../../ere

ifdef AIXxlc
CC =		cc
CFLAGS =	-c -DUNIX -DAIX
else
ifdef AIX
CC =		gcc
CFLAGS =	-c -DUNIX -DAIX
else
ifdef HP
CC =		cc
CFLAGS =	-c -DUNIX -DHP    -Aa
else
ifdef SUN
CC =		gcc
CFLAGS =	-c -DUNIX -DSUN
else
ifdef SUNcc
CC =		cc
CFLAGS =	-c -DUNIX -DSUN   -Xa
else
ifdef XNX
CC =		cc
CFLAGS =	-c -DUNIX -DXNX
else
CC =		cc
CFLAGS =	-c -DUNIX -DLINUX
endif
endif
endif
endif
endif
endif

.SUFFIXES:	.o .c

.c.o:
		$(CC) $(CFLAGS) -I$(ERE) -DFLATTEN $*.c

#

flatten:	flatten.o mdep.o $(ERE)/ere.a 
		$(CC) -o flatten flatten.o mdep.o $(ERE)/ere.a 
		strip flatten

flatten.o:	flatten.c mdep.h types.h

mdep.o:		mdep.c mdep.h types.h

# Clean

clean:
		@rm -f *.o

# Install

IDIR =		/usr/local/bin

install:
		cp flatten $(IDIR)
		chmod 755 $(IDIR)/flatten

#

package:
		zip -q -r flatten *
