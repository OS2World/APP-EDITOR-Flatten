#
# Flatten
#
# OS/2
# IBM VisualAge C++ 3.0, with CTC306, CTD302, CTU304
#

ERE =		..\..\ere

CWARNS =	/W3 /Wcmp+cnd+dcl+ord+par+use+
CFLAGS =	/C+ /I$(ERE) /G4 /Gd-e+m+ $(CWARNS) /DOS2 /DFLATTEN /Q+
LFLAGS =	/STACK:20480 /NOI /ALIGN:16 /NOLOGO /PMTYPE:VIO /NOE

.c.obj:
		icc $(CFLAGS) $*.c

#

flatten.exe:	flatten.obj mdep.obj $(ERE)\ere.lib
		ilink $(LFLAGS) /OUT:$@ setargv.obj $**

flatten.obj:	flatten.c mdep.h types.h

mdep.obj:	mdep.c mdep.h types.h

#

clean:
		-del *.obj 2> nul

#

IDIR =		c:\ae

install:
		@copy flatten.exe $(IDIR) > nul

#

package:
		zip -q -r -z flatten * < <<

Flatten a folded file utility
This ZIP file was made using InfoZIPs ZIP on a non-UNIX platform.
It contains a mixture of ASCII and binary files.
It was made without using -l or -ll, so ASCII files contain CR LF pairs.
So it is best to UnZIP this file on a non-UNIX platform.

<<
