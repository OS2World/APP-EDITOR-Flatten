/*

mdep.c - Machine dependent parts of Andys Editor, 32 bit OS/2 version

*/

/*...sincludes:0:*/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <io.h>
#define INCL_BASE
#undef NULL
#include <os2.h>
#include <process.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef FLATTEN
#include "th.h"
#endif
#include "types.h"

/*...vtypes\46\h:0:*/
/*...e*/

/*...sfile handling:0:*/
#define isslash(c) ((c)=='/'||(c)=='\\')
/*...sget_cur_drv:0:*/
static char cur_drv;

static void get_cur_drv(void)
	{
	ULONG ulDriveNum, ulDriveMap;

	DosQueryCurrentDisk(&ulDriveNum, &ulDriveMap);
	cur_drv = (char) (ulDriveNum + 'a' - 1);
	}
/*...e*/
/*...sexpand_fn:0:*/
void expand_fn(const char *s, char *fn)
	{
	static char env_name[WIDE+1], *result;
	int i, j;

	i = 0;
	while ( *s )
		if ( *s == '$' && s[1] == '(' )
			/* $( means environment variable */
			{
			j = 0;
			while ( *s && *s != ')' )
				env_name[j++] = *s++;
			env_name[j] = '\0';
			if ( *s )
				/* $(ENV) is complete */
				{
				s++;
				strupr(env_name);
				if ( (result = getenv(env_name + 2)) == NULL )
					result = "";
				}
			else
				result = env_name;
			if ( i + (j = strlen(result)) > WIDE )
				break;
			memcpy(fn + i, result, j);
			i += j;
			}
		else
			{
			if ( i + 1 > WIDE )
				break;
			fn[i++] = *s++;
			}
	fn[i] = '\0';
	}
/*...e*/
/*...strunc_pos:0:*/
/*
When truncating the filename, we must be sure we are not replacing the . in
a pathname component instead of the . in the filename. To do this, we ensure
that / is before any . we wish to stomp.
*/

static char * trunc_pos(char *fn)
	{
	register char *p, *q;

	if ( (p = strrchr(fn, '.')) != NULL )
		if ( ( (q = strrchr(fn, '/')) == NULL ) || q < p )
			return p;
	return NULL;
	}
/*...e*/
/*...scmp_os_fn:0:*/
int cmp_os_fn(const char *fn1, const char *fn2)
	{
	return strcmpi(fn1, fn2);
	}
/*...e*/
/*...sget_match_fn:0:*/
void get_match_fn(const char *os_fn, char *match_fn)
	{
	strcpy(match_fn, os_fn);
	strlwr(match_fn);
	}
/*...e*/
/*...sget_os_fn:0:*/
/*
Given a filename, return a pointer to it modified (if necessary)
so as to be suitable for the operating system used. If the
filename is not suitable then return NULL.
This should be called to generate filenames for use by all the
other routines for file handling.
*/

const char * get_os_fn(const char *fn)
	{
	static char os_fn[WIDE+1];
	char *q = os_fn;
	const char *p;

	if ( isslash(fn[0]) && isslash(fn[1]) )
		; /* UNC name, such as \\SERVER\PATH\FILE.EXT, so leave it. */
	else if ( isalpha(fn[0]) && fn[1] == ':' )
		; /* Got drive, such as D:\PATH\FILE.EXT, so leave it. */
	else
		/* Prepend D:, giving D:PATH\FILE.EXT or D:\PATH\FILE.EXT. */
		{
		if ( strlen(fn) > WIDE - 2 )
			return NULL;
		*q++ = cur_drv;
		*q++ = ':';
		}

	for ( p = fn; *p; p++ )
		{
		if ( *p < ' ' || *p > '~' )
			return NULL;
		if ( strchr("?*<>|=[]", *p) != NULL )
			return NULL;
		*q++ = (char) ( ( *p != '\\' ) ? *p : '/' );
		}
	*q = '\0';

	return os_fn;
	}
/*...e*/
/*...sget_ae_fn:0:*/
/*
The area of great debate on the OS/2 version.
Finding the initialisation file.
*/

#define	USE_PATH

#ifdef USE_DosSearchPath	/* OS/2 mode only */
/*...sget_ae_fn:0:*/
const char * get_ae_fn(const char *argv0)
	{
	static char full_fn[WIDE+1];
	char ae_fn[WIDE+1];
	const char *p;
	char *q;

	/* Only interested in filename part, without any drive or path */

	if ( (p = strrchr(argv0, '\\')) != NULL )
		p++;
	else if ( argv0[0] && argv0[1] == ':' )
		p = argv0 + 2;
	else
		p = argv0;

	strcpy(ae_fn, p);
	if ( (q = trunc_pos(ae_fn)) != NULL )
		*q = '\0';
	strcat(ae_fn, ".ini");

	/* Search along the PATH for the file, doing current dir first */

	DosSearchPath(3, "PATH", ae_fn, full_fn, WIDE+1);
	return full_fn;
	}
/*...e*/
#endif
#ifdef USE_PATH			/* Works in Bound apps and OS/2 only */
/*...sget_ae_fn:0:*/
/*
argv0 = "D:\PATH\NAME.EXE" in DOS only app.
argv0 = "NAME" in BOUND app running DOS.
argv0 = ? in OS/2 only app.
argv0 = ? in BOUND app running OS/2.
*/

/*...slook_for:0:*/
/*...sfile_exists:0:*/
static AE_BOOLEAN file_exists(const char *fn)
	{
	FILE *fp;
	if ( (fp = fopen(fn, "r")) == NULL )
		return AE_FALSE;
	fclose(fp);
	return AE_TRUE;
	}
/*...e*/

static const char * look_for(const char *fn, const char *env)
	{
	char *var;
	static char path[WIDE+1];
	static char full_fn[WIDE+1];

	if ( file_exists(fn) )
		return fn;

	if ( (var = getenv(env)) != NULL )
		/* Search along environment variable */
		{
		char *elem;

		strncpy(path, var, WIDE);
		for ( elem  = strtok(path, ";");
		      elem != NULL;
		      elem  = strtok(NULL, ";") )
			{
			int	len;

			strcpy(full_fn, elem);
			len = strlen(full_fn);
			if ( len && full_fn[len - 1] != '\\' )
				strcat(full_fn, "\\");
			strcat(full_fn, fn);
			if ( file_exists(full_fn) )
				return full_fn;
			}
		}

	return NULL;
	}
/*...e*/

const char * get_ae_fn(const char *argv0)
	{
	static char ae_fn[WIDE+1];
	const char *p;
	char *q;

	/* Only interested in filename part, without any drive or path */

	if ( (p = strrchr(argv0, '\\')) != NULL )
		p++;
	else if ( argv0[0] && argv0[1] == ':' )
		p = argv0 + 2;
	else
		p = argv0;

	/* Append EXE to the end and look for it along path */

	strcpy(ae_fn, p);
	if ( (q = trunc_pos(ae_fn)) != NULL )
		*q = '\0';
	strcat(ae_fn, ".exe");

	if ( (p = look_for(ae_fn, "PATH")) != NULL )
		strcpy(ae_fn, p);

	strcpy(trunc_pos(ae_fn), ".ini");

	return ae_fn;
	}
/*...e*/
#endif
/*...e*/
/*...sget_bak_fn:0:*/
/*
Return the backup filename or NULL if the backup
is pointless. ie: do not backup a .bak file!
*/

const char * get_bak_fn(const char *fn)
	{
	static char bak_str[] = ".bak";
	static char bak_fn[WIDE+1];
	register char *p;

	strcpy(bak_fn, fn);
	if ( (p = trunc_pos(bak_fn)) != NULL )
		if ( strcmpi(p, bak_str) )
			strcpy(p, bak_str);
		else
			return NULL;
	else
		strcat(bak_fn, bak_str);
	return bak_fn;
	}
/*...e*/
/*...sget_nest_fn:0:*/
/*
fn_nested is an appending string or "".
If its "" then return "".
fn_main is an OS filename or "".
If its "" then return OS filename of fn_nested.
If fn_nested is on a different drive then return OS filename of fn_nested.
If fn_nested is absolute then return OS filename of fn_nested.
Else combine to form compound filename.
*/

const char * get_nest_fn(const char *fn_main, const char *fn_nested)
	{
	static char p[WIDE+1];
	char *q;

	if ( !fn_nested[0] )
		return "";

	if ( !fn_main[0] )
		return get_os_fn(fn_nested);

	if ( isslash(fn_nested[0]) )
		/* Could be absolute name, like \PATH\FILE.EXT,
		   or a UNC name like \\SERVER\PATH\FILE.EXT */
		return get_os_fn(fn_nested);

	if ( isalpha(fn_nested[0]) && fn_nested[1] == ':' )
		/* Is something like D:PATH\FILE.EXE or D:\PATH\FILE.EXT. */
		return get_os_fn(fn_nested);

	/* fn_nested is relative and is on the same drive */

	strcpy(p, fn_main);
	if ( (q = strrchr(p, '/')) != NULL )
		q++;
	else
		q = p + 2;

	if ( (q - p) + strlen(fn_nested) > WIDE )
		return NULL;

	strcpy(q, fn_nested);
	
	return get_os_fn(p);
	}
/*...e*/
/*...sunlink_file:0:*/
AE_BOOLEAN unlink_file(const char *fn)
	{
	struct stat buf;

	if ( stat(fn, &buf) != -1          &&      /* Can stat file       */
	     (buf.st_mode & S_IFDIR ) == 0 &&      /* Is not a directory  */
	     (buf.st_mode & S_IWRITE) == 0 )       /* Can't be written to */
		chmod(fn, S_IREAD|S_IWRITE);       /* Make it writable    */

	return !unlink(fn);			   /* Now can delete it!  */
	}
/*...e*/
/*...srename_file:0:*/
AE_BOOLEAN rename_file(const char *old_fn, const char *new_fn)
	{
	return !rename(old_fn, new_fn);
	}
/*...e*/
/*...sfilemode stuff:0:*/
/*
Get the filemode, return -1 if no file or
named item is not a file. ie: a directory.
*/

int get_filemode(const char *fn)
	{
	struct stat buf;

	if ( stat(fn, &buf) == -1 || (buf.st_mode & S_IFDIR) != 0 )
		return -1;
	return buf.st_mode;
	}

void set_filemode(const char *fn, int mode)
	{
	chmod(fn, mode&(S_IREAD|S_IWRITE));
	}

AE_BOOLEAN is_readonly(const char *fn)
	{
	struct stat buf;

	if ( stat(fn, &buf) == -1 )
		return AE_FALSE;
	return (buf.st_mode & S_IWRITE) == 0;
	}
/*...e*/
/*...sfopen_file:0:*/
FILE * fopen_file(const char *fn, const char *mode)
	{
	FILE *fp;

	if ( (fp = fopen(fn, mode)) == NULL )
		return NULL;

/*...sreject non\45\regular files:8:*/
{
ULONG ulHandType, ulFlags;

DosQueryHType((HFILE) fileno(fp), &ulHandType, &ulFlags);
if ( (ulHandType & 0x00ff) != 0x0000 )
	{
	fclose(fp); return NULL;
	}
}
/*...e*/

	return fp;
	}
/*...e*/
/*...sfopen_tmp_file:0:*/
FILE *fopen_tmp_file(char *fn, const char *mode)
	{
	const char *f;
	if ( (f = tmpnam(NULL)) == NULL )
		return NULL;
	strcpy(fn, f);
	return fopen(fn, mode);
	}
/*...e*/
/*...e*/
#ifndef FLATTEN
/*...skey_tab:0:*/
/*
Here are the special keys you can bind to.
*/

KEY	key_tab[] =
	{
							/* Not allowed ^@ */
		{"^A",		K_CTRL('A'),	AE_FALSE},
		{"^B",		K_CTRL('B'),	AE_FALSE},
		{"^C",		K_CTRL('C'),	AE_FALSE},
		{"^D",		K_CTRL('D'),	AE_FALSE},
		{"^E",		K_CTRL('E'),	AE_FALSE},
		{"^F",		K_CTRL('F'),	AE_FALSE},
		{"^G",		K_CTRL('G'),	AE_FALSE},
		{"^H",		K_CTRL('H'),	AE_FALSE},
		{"^I",		K_CTRL('I'),	AE_FALSE},
		{"^J",		K_CTRL('J'),	AE_FALSE},
		{"^K",		K_CTRL('K'),	AE_FALSE},
		{"^L",		K_CTRL('L'),	AE_FALSE},
		{"^M",		K_CTRL('M'),	AE_FALSE},
		{"^N",		K_CTRL('N'),	AE_FALSE},
		{"^O",		K_CTRL('O'),	AE_FALSE},
		{"^P",		K_CTRL('P'),	AE_FALSE},
		{"^Q",		K_CTRL('Q'),	AE_FALSE},
		{"^R",		K_CTRL('R'),	AE_FALSE},
		{"^S",		K_CTRL('S'),	AE_FALSE},
		{"^T",		K_CTRL('T'),	AE_FALSE},
		{"^U",		K_CTRL('U'),	AE_FALSE},
		{"^V",		K_CTRL('V'),	AE_FALSE},
		{"^W",		K_CTRL('W'),	AE_FALSE},
		{"^X",		K_CTRL('X'),	AE_FALSE},
		{"^Y",		K_CTRL('Y'),	AE_FALSE},
		{"^Z",		K_CTRL('Z'),	AE_FALSE},
							/* ^[ is escape */
		{"^\\",		K_CTRL('\\'),	AE_FALSE},
		{"^]",		K_CTRL(']'),	AE_FALSE},
		{"^^",		K_CTRL('^'),	AE_FALSE},
		{"^_",		K_CTRL('_'),	AE_FALSE},

		{"Del127",	K_DEL,		AE_FALSE},
	
		{"F1",		K_F1,		AE_FALSE},
		{"F2",		K_F2,		AE_FALSE},
		{"F3",		K_F3,		AE_FALSE},
		{"F4",		K_F4,		AE_FALSE},
		{"F5",		K_F5,		AE_FALSE},
		{"F6",		K_F6,		AE_FALSE},
		{"F7",		K_F7,		AE_FALSE},
		{"F8",		K_F8,		AE_FALSE},
		{"F9",		K_F9,		AE_FALSE},
		{"F10",		K_F10,		AE_FALSE},
		{"F11",		K_F11,		AE_FALSE},
		{"F12",		K_F12,		AE_FALSE},
	
		{"~F1",		K_SHIFT_F1,	AE_FALSE},
		{"~F2",		K_SHIFT_F2,	AE_FALSE},
		{"~F3",		K_SHIFT_F3,	AE_FALSE},
		{"~F4",		K_SHIFT_F4,	AE_FALSE},
		{"~F5",		K_SHIFT_F5,	AE_FALSE},
		{"~F6",		K_SHIFT_F6,	AE_FALSE},
		{"~F7",		K_SHIFT_F7,	AE_FALSE},
		{"~F8",		K_SHIFT_F8,	AE_FALSE},
		{"~F9",		K_SHIFT_F9,	AE_FALSE},
		{"~F10",	K_SHIFT_F10,	AE_FALSE},
		{"~F11",	K_SHIFT_F11,	AE_FALSE},
		{"~F12",	K_SHIFT_F12,	AE_FALSE},
	
		{"^F1",		K_CTRL_F1,	AE_FALSE},
		{"^F2",		K_CTRL_F2,	AE_FALSE},
		{"^F3",		K_CTRL_F3,	AE_FALSE},
		{"^F4",		K_CTRL_F4,	AE_FALSE},
		{"^F5",		K_CTRL_F5,	AE_FALSE},
		{"^F6",		K_CTRL_F6,	AE_FALSE},
		{"^F7",		K_CTRL_F7,	AE_FALSE},
		{"^F8",		K_CTRL_F8,	AE_FALSE},
		{"^F9",		K_CTRL_F9,	AE_FALSE},
		{"^F10",	K_CTRL_F10,	AE_FALSE},
		{"^F11",	K_CTRL_F11,	AE_FALSE},
		{"^F12",	K_CTRL_F12,	AE_FALSE},
	
		{"@F1",		K_ALT_F1,	AE_FALSE}, 
		{"@F2",		K_ALT_F2,	AE_FALSE}, 
		{"@F3",		K_ALT_F3,	AE_FALSE}, 
		{"@F4",		K_ALT_F4,	AE_FALSE}, 
		{"@F5",		K_ALT_F5,	AE_FALSE}, 
		{"@F6",		K_ALT_F6,	AE_FALSE}, 
		{"@F7",		K_ALT_F7,	AE_FALSE}, 
		{"@F8",		K_ALT_F8,	AE_FALSE}, 
		{"@F9",		K_ALT_F9,	AE_FALSE}, 
		{"@F10",	K_ALT_F10,	AE_FALSE},
		{"@F11",	K_ALT_F11,	AE_FALSE},
		{"@F12",	K_ALT_F12,	AE_FALSE},
	
		{"Left",	K_LEFT,		AE_FALSE},
		{"Right",	K_RIGHT,	AE_FALSE},
		{"Up",		K_UP,		AE_FALSE},
		{"Down",	K_DOWN,		AE_FALSE},
		{"PgUp",	K_PGUP,		AE_FALSE},
		{"PgDn",	K_PGDN,		AE_FALSE},
		{"Home",	K_HOME,		AE_FALSE},
		{"End",		K_END,		AE_FALSE},
		{"Ins",		K_INSERT,	AE_FALSE},
		{"Del",		K_DELETE,	AE_FALSE},
	
		{"^Left",	K_CTRL_LEFT,	AE_FALSE},
		{"^Right",	K_CTRL_RIGHT,	AE_FALSE},
		{"^Up",		K_CTRL_UP,	AE_FALSE},
		{"^Down",	K_CTRL_DOWN,	AE_FALSE},
		{"^PgUp",	K_CTRL_PGUP,	AE_FALSE},
		{"^PgDn",	K_CTRL_PGDN,	AE_FALSE},
		{"^Home",	K_CTRL_HOME,	AE_FALSE},
		{"^End",	K_CTRL_END,	AE_FALSE},
		{"^Ins",	K_CTRL_INSERT,	AE_FALSE},
		{"^Del",	K_CTRL_DELETE,	AE_FALSE},

		{"@Left",	K_ALT_LEFT,	AE_FALSE},
		{"@Right",	K_ALT_RIGHT,	AE_FALSE},
		{"@Up",		K_ALT_UP,	AE_FALSE},
		{"@Down",	K_ALT_DOWN,	AE_FALSE},
		{"@Home",	K_ALT_HOME,	AE_FALSE},
		{"@End",	K_ALT_END,	AE_FALSE},
		{"@PgUp",	K_ALT_PGUP,	AE_FALSE},
		{"@PgDn",	K_ALT_PGDN,	AE_FALSE},
		{"@Ins",	K_ALT_INSERT,	AE_FALSE},
		{"@Del",	K_ALT_DELETE,	AE_FALSE},
	
		{"~Tab",	K_SHIFT_TAB,	AE_FALSE},
		{"^Tab",	K_CTRL_TAB,	AE_FALSE},
		{"@Tab",	K_ALT_TAB,	AE_FALSE},

		{"^Grey+",	K_CTRL_G_PLUS,	AE_FALSE},
		{"^Grey-",	K_CTRL_G_MINUS,	AE_FALSE},
		{"^Grey/",	K_CTRL_G_SLASH,	AE_FALSE},
		{"^Grey*",	K_CTRL_G_STAR,	AE_FALSE},
		{"@Grey+",	K_ALT_G_PLUS,	AE_FALSE},
		{"@Grey-",	K_ALT_G_MINUS,	AE_FALSE},
		{"@Grey/",	K_ALT_G_SLASH,	AE_FALSE},
		{"@Grey*",	K_ALT_G_STAR,	AE_FALSE},
	
		{"@A",		K_ALT_A,	AE_FALSE},
		{"@B",		K_ALT_B,	AE_FALSE},
		{"@C",		K_ALT_C,	AE_FALSE},
		{"@D",		K_ALT_D,	AE_FALSE},
		{"@E",		K_ALT_E,	AE_FALSE},
		{"@F",		K_ALT_F,	AE_FALSE},
		{"@G",		K_ALT_G,	AE_FALSE},
		{"@H",		K_ALT_H,	AE_FALSE},
		{"@I",		K_ALT_I,	AE_FALSE},
		{"@J",		K_ALT_J,	AE_FALSE},
		{"@K",		K_ALT_K,	AE_FALSE},
		{"@L",		K_ALT_L,	AE_FALSE},
		{"@M",		K_ALT_M,	AE_FALSE},
		{"@N",		K_ALT_N,	AE_FALSE},
		{"@O",		K_ALT_O,	AE_FALSE},
		{"@P",		K_ALT_P,	AE_FALSE},
		{"@Q",		K_ALT_Q,	AE_FALSE},
		{"@R",		K_ALT_R,	AE_FALSE},
		{"@S",		K_ALT_S,	AE_FALSE},
		{"@T",		K_ALT_T,	AE_FALSE},
		{"@U",		K_ALT_U,	AE_FALSE},
		{"@V",		K_ALT_V,	AE_FALSE},
		{"@W",		K_ALT_W,	AE_FALSE},
		{"@X",		K_ALT_X,	AE_FALSE},
		{"@Y",		K_ALT_Y,	AE_FALSE},
		{"@Z",		K_ALT_Z,	AE_FALSE},
	
		{"@0",		K_ALT_0,	AE_FALSE},
		{"@1",		K_ALT_1,	AE_FALSE},
		{"@2",		K_ALT_2,	AE_FALSE},
		{"@3",		K_ALT_3,	AE_FALSE},
		{"@4",		K_ALT_4,	AE_FALSE},
		{"@5",		K_ALT_5,	AE_FALSE},
		{"@6",		K_ALT_6,	AE_FALSE},
		{"@7",		K_ALT_7,	AE_FALSE},
		{"@8",		K_ALT_8,	AE_FALSE},
		{"@9",		K_ALT_9,	AE_FALSE},
	
		{"@-",		K_ALT_MINUS,	AE_FALSE},
		{"@=",		K_ALT_EQUALS,	AE_FALSE},
	
		{NULL,		0,		AE_FALSE},	/* Terminator */
	};
/*...e*/
/*...schild processes:0:*/
#define	FD_STDIN	0
#define	FD_STDOUT	1
#define	FD_STDERR	2

static int fd_stdin, fd_stdout, fd_stderr;

static void take_copies(void)
	{
	fd_stdin  = dup(FD_STDIN);
	fd_stdout = dup(FD_STDOUT);
	fd_stderr = dup(FD_STDERR);
	}

static void reconnect(void)
	{
	dup2(fd_stdin,  FD_STDIN);  close(fd_stdin);
	dup2(fd_stdout, FD_STDOUT); close(fd_stdout);
	dup2(fd_stderr, FD_STDERR); close(fd_stderr);
	}

static const char * shell_to_use(void)
	{
	char *p;
	return (p = getenv("COMSPEC")) != NULL ? p : "CMD";
	}

AE_BOOLEAN shell(const char *command)
	{
	int tty, result;

	/* Connect to console */

	tty = open("con", O_RDWR);	/* Both input and output */

	take_copies();

	dup2(tty, FD_STDIN);
	dup2(tty, FD_STDOUT);
	dup2(tty, FD_STDERR);

	/* Perform command */

	result = system( ( *command ) ? command : shell_to_use() );

	/* If not interactive shell then prompt for return */

	if ( *command )
		{
		char s[50];

		fputs("\npress return to return to ae ... ", stderr);
		fflush(stderr);
		fgets(s, sizeof(s), stdin);
		}

	reconnect();

	/* Close connection to console */

	close(tty);
	get_cur_drv();

	return result != -1;
	}

AE_BOOLEAN xfilter(const char *command, const char *in_fn, const char *out_fn)
	{
	int fd_in, fd_out, result;

	/* Open input and output files */

	fd_in  = open(in_fn, O_RDONLY);
	fd_out = creat(out_fn, 0600);
					/* Read and write permission, owner */

	take_copies();

	dup2(fd_in,  FD_STDIN);
	dup2(fd_out, FD_STDOUT);
	dup2(fd_out, FD_STDERR);

	/* Perform command */

	result = system( ( *command ) ? command : shell_to_use() );

	reconnect();

	/* Close connections to files */

	close(fd_in);
	close(fd_out);

	get_cur_drv();

	return result != -1;
	}
/*...e*/
#endif
/*...sunhog:0:*/
void PASFUNC unhog(void) {}
/*...e*/
/*...sinit and deinit:0:*/
void mdep_init(void)
	{
	get_cur_drv();
	}

void mdep_deinit(void)
	{
	}
/*...e*/
