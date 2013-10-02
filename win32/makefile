#
# Flatten
#
# Win32
# Microsoft Visual C++ 6.0
#

ERE =		..\..\ere

CFLAGS =	/c /I$(ERE) /Oit /Gs /G4 /W3 /WX /DWIN32 /MT /DFLATTEN /nologo
LFLAGS =	/NOLOGO /DEBUG /INCREMENTAL:NO

.c.obj:
		cl $(CFLAGS) $*.c

#

flatten.exe:	flatten.obj mdep.obj $(ERE)\ere.lib
		link $(LFLAGS) /OUT:$@ $** setargv.obj

flatten.obj:	flatten.c mdep.h types.h

mdep.obj:	mdep.c mdep.h types.h

#

clean:
		-del *.obj
