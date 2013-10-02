/*

mdep.c - Machine dependent parts of Andys Editor, UNIX versions

This particular file is for UNIX variants.
To specify a Linux version, define LINUX.
To specify a IBM AIX version, define AIX.
To specify a HP-UX version, defined HP.
To specify a Sun workstation, define preprocessor symbol SUN.
To specify a Microsoft Xenix version define XNX.
To specify a S/390 USS version define S390USS.
Otherwise, generic UNIX is assumed.

This code uses the environment variables below if defined :-
	SHELL	to chose shell to use for child processes
	HOME	to look for Andys Editor files

*/

/*...sincludes:0:*/
#ifdef HP
#define	_INCLUDE_POSIX_SOURCE /* for getpwnam etc. */
#define	_INCLUDE_XOPEN_SOURCE /* for S_IFDIR etc. */
extern char *mktemp(char *);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>

#ifndef FLATTEN
#include "th.h"
#endif
#include "types.h"

/*...vtypes\46\h:0:*/
/*...e*/

static const char *ae;

/*...sfilename stuff:0:*/
/*...strunc_pos:0:*/
/*
When truncating the filename, we must be sure we are not replacing the . in
a pathname component instead of the . in the filename. To do this, we ensure
that / is before any . we wish to stomp.
*/

static char *trunc_pos(char *fn)
	{
	register char *p, *q;

	if ( (p = strrchr(fn, '.')) != NULL )
		if ( ( (q = strrchr(fn, '/')) == NULL ) || q < p )
			return p;
	return NULL;
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
/*...scmp_os_fn:0:*/
int cmp_os_fn(const char *fn1, const char *fn2)
	{
	return strcmp(fn1, fn2);
	}
/*...e*/
/*...sget_match_fn:0:*/
void get_match_fn(const char *os_fn, char *match_fn)
	{
	strcpy(match_fn, os_fn);
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

const char *get_os_fn(const char *fn)
	{
	static char os_fn[WIDE+1];
	char *q = os_fn;
	const char *p = fn;

	if ( p[0] == '~' )
		{
		const char *end, *homedir = NULL;
		char userid[WIDE+1];
		if ( (end = strchr(fn, '/' )) == NULL &&
		     (end = strchr(fn, '\\')) == NULL )
			end = fn + strlen(fn);
		memcpy(userid, p+1, end-(p+1));
		userid[end-(p+1)] = '\0';
		if ( userid[0] == '\0' )
			homedir = getenv("HOME");
		else
			{
			struct passwd *pw;
			if ( (pw = getpwnam(userid)) != NULL )
				homedir = pw->pw_dir;
			}
		if ( homedir != NULL )
			{
			if ( strlen(homedir) > WIDE )
				return NULL;
			strcpy(q, homedir);
			q += strlen(q);
			p = end;
			}
		}

	for ( ; *p; p++ )
		{
#ifdef EBCDIC
		if ( !isprint(*p) )
#else
		if ( *p < ' ' || *p > '~' )
#endif
			return NULL;
		if ( q >= os_fn + WIDE )
			return NULL;
		*q++ = ( *p != '\\' ) ? *p : '/';
		}
	*q = '\0';

	if ( !strncmp(os_fn, "/dev/", 5) )
		return NULL;

	return os_fn;
	}
/*...e*/
/*...sget_ae_fn:0:*/
/*...sfile_exists:0:*/
static AE_BOOLEAN file_exists(const char *fn)
	{
	struct stat buf;
	FILE *fp;
	if ( stat(fn, &buf) == -1 )
		return AE_FALSE;
	if ( !S_ISREG(buf.st_mode) )
		return AE_FALSE;
	if ( (fp = fopen(fn, "r")) == 0 )
		return AE_FALSE;
	fclose(fp);
	return AE_TRUE;
	}
/*...e*/
/*...slook_for:0:*/
static const char *look_for(const char *fn, const char *env)
	{
	char *var;
	static char path[2000+1];
	static char full_fn[WIDE+1];

	if ( file_exists(fn) )
		return fn;

	if ( (var = getenv(env)) != 0 )
		/* Search along environment variable */
		{
		char *elem;
		strncpy(path, var, 2000);
		for ( elem  = strtok(path, ":");
		      elem != 0;
		      elem  = strtok(0, ":") )
			{
			int len;

			strcpy(full_fn, elem);
			len = strlen(full_fn);
			if ( len && full_fn[len - 1] != '/' )
				strcat(full_fn, "/");
			strcat(full_fn, fn);
			if ( file_exists(full_fn) )
				return full_fn;
			}
		}

	return 0;
	}
/*...e*/

const char *get_ae_fn(const char *argv0)
	{
	static char ae_fn[WIDE+1];
	const char *p;
	int len_ae;

	if ( (p = strrchr(argv0, '/')) != NULL )
		argv0 = p+1;
	strcpy(ae_fn, ae);
	len_ae = strlen(ae);
	if ( !len_ae || ae_fn[len_ae-1] != '/' )
		/* Will need to append a slash */
		strcat(ae_fn, "/");
	strcat(ae_fn, ".");
	strcat(ae_fn, argv0);
	strcat(ae_fn, "rc");
	if ( file_exists(ae_fn) )
		return ae_fn;

	strcpy(ae_fn, argv0);
	if ( (p = look_for(ae_fn, "PATH")) != NULL )
		strcpy(ae_fn, p);
	strcat(ae_fn, ".ini");
	return ae_fn;
	}
/*...e*/
/*...sget_bak_fn:0:*/
/*
Return the backup filename or NULL if the backup
is pointless. ie: do not backup a .bak file!
*/

static char bak_str[] = ".bak";

const char *get_bak_fn(const char *fn)
	{
	static char bak_fn[WIDE+1], *bak_fn_past_slash;
	char *p;

	strcpy(bak_fn, fn);
	if ( (bak_fn_past_slash = strrchr(bak_fn, '/')) != NULL )
		bak_fn_past_slash++;
	else
		bak_fn_past_slash = bak_fn;

	if ( *bak_fn_past_slash == '\0' )
		return NULL;

	if ( (p = trunc_pos(bak_fn_past_slash + 1)) != NULL )
		if ( strcmp(p, bak_str) )
			strcpy(p, bak_str);
		else
			return NULL;
	else
		strcat(bak_fn_past_slash, bak_str);
	return bak_fn;
	}
/*...e*/
/*...sget_nest_fn:0:*/
/*
fn_nested is either an appendind string or "".
If its "" then return "".
fn_main is either a valid OS filename or "".
If its "" then return OS filename of fn_nested.
If fn_nested starts with a "/" then return OS fn of fn_nested.
Otherwise fn_nested is to be appended to fn_main.
*/

const char *get_nest_fn(const char *fn_main, const char *fn_nested)
	{
	static char p[WIDE+1];
	char *q;

	if ( !fn_nested[0] )
		return "";

	if ( !fn_main[0] )
		return get_os_fn(fn_nested);

	if ( fn_nested[0] == '/' || fn_nested[0] == '\\' || fn_nested[0] == '~' )
		return get_os_fn(fn_nested);

	strcpy(p, fn_main);
	if ( (q = strrchr(p, '/')) != NULL )
		q++;
	else
		q = p;

	if ( (q - p) + strlen(fn_nested) > WIDE )
		return NULL;

	strcpy(q, fn_nested);

	return get_os_fn(p);
	}
/*...e*/
/*...sunlink_file:0:*/
AE_BOOLEAN unlink_file(const char *fn)
	{
	return !unlink(fn); /* $$$ For now */
	}
/*...e*/
/*...srename_file:0:*/
AE_BOOLEAN rename_file(const char *old_fn, const char *new_fn)
	{
#if XNX
	return !link(old_fn, new_fn) && !unlink(old_fn);
#else
	return !rename(old_fn, new_fn);
#endif
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

	if ( stat(fn, &buf) == -1 || (buf.st_mode & S_IFDIR) )
		return -1;
	return buf.st_mode;
	}

void set_filemode(const char *fn, int mode)
	{
	chmod(fn, mode);
	}

AE_BOOLEAN is_readonly(const char *fn)
	{
	struct stat buf;

	if ( stat(fn, &buf) == -1 || (buf.st_mode & S_IFDIR) )
		return AE_FALSE; /* Assume its not */
	return (buf.st_mode & S_IWUSR) == 0;
	}
/*...e*/
/*...sfopen_file:0:*/
FILE *fopen_file(const char *fn, const char *mode)
	{
	return fopen(fn, mode);
	}
/*...e*/
/*...sfopen_tmp_file:0:*/
/* This function has been created to plug a classic security hole.
   Without the use of mkstemp, a file is created and may be written over with
   a symlink pointing at some critical file, then we read or write over it. */

FILE *fopen_tmp_file(char *fn, const char *mode)
	{
	static char template[14];
	strcpy(template, "/tmp/aeXXXXXX");
#ifdef LINUX
	{
	int fd;
	if ( (fd = mkstemp(template)) == -1 )
		return NULL;
	/* fd is an open file with 0600 mode */
	strcpy(fn, template);
	return fdopen(fd, mode);
	}
#else
	if ( mktemp(template) == NULL )
		return NULL;
	strcpy(fn, template);
	return fopen(fn, mode);
#endif
	}
/*...e*/
/*...e*/
#ifndef FLATTEN
/*...skey names:0:*/
KEY key_tab[] =
	{
					/* ^@ not allowed */
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

	{"Left",	K_LEFT,		AE_FALSE},
	{"Right",	K_RIGHT,	AE_FALSE},
	{"Up",		K_UP,		AE_FALSE},
	{"Down",	K_DOWN,		AE_FALSE},
	{"Home",	K_HOME,		AE_FALSE},
	{"End",		K_END,		AE_FALSE},
	{"PgUp",	K_PGUP,		AE_FALSE},
	{"PgDn",	K_PGDN,		AE_FALSE},
	{"Ins",		K_INSERT,	AE_FALSE},
	{"Del",		K_DELETE,	AE_FALSE},

	{"~Left",	K_SHIFT_LEFT,	AE_FALSE},
	{"~Right",	K_SHIFT_RIGHT,	AE_FALSE},
	{"~Up",		K_SHIFT_UP,	AE_FALSE},
	{"~Down",	K_SHIFT_DOWN,	AE_FALSE},
	{"~Home",	K_SHIFT_HOME,	AE_FALSE},
	{"~End",	K_SHIFT_END,	AE_FALSE},
	{"~PgUp",	K_SHIFT_PGUP,	AE_FALSE},
	{"~PgDn",	K_SHIFT_PGDN,	AE_FALSE},

	{"^Left",	K_CTRL_LEFT,	AE_FALSE},
	{"^Right",	K_CTRL_RIGHT,	AE_FALSE},
	{"^Up",		K_CTRL_UP,	AE_FALSE},
	{"^Down",	K_CTRL_DOWN,	AE_FALSE},
	{"^Home",	K_CTRL_HOME,	AE_FALSE},
	{"^End",	K_CTRL_END,	AE_FALSE},
	{"^PgUp",	K_CTRL_PGUP,	AE_FALSE},
	{"^PgDn",	K_CTRL_PGDN,	AE_FALSE},
	{"^Ins",	K_CTRL_INSERT,	AE_FALSE},
	{"^Del",	K_CTRL_DELETE,	AE_FALSE},
	{"^Grey+",	K_CTRL_G_PLUS,	AE_FALSE},
	{"^Grey-",	K_CTRL_G_MINUS,	AE_FALSE},
	{"^Grey*",	K_CTRL_G_STAR,	AE_FALSE},
	{"^Grey/",	K_CTRL_G_SLASH,	AE_FALSE},

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
	{"@Grey+",	K_ALT_G_PLUS,	AE_FALSE},
	{"@Grey-",	K_ALT_G_MINUS,	AE_FALSE},
	{"@Grey*",	K_ALT_G_STAR,	AE_FALSE},
	{"@Grey/",	K_ALT_G_SLASH,	AE_FALSE},

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

	{NULL,		0,		AE_FALSE},	/* Terminator */

	};
/*...e*/
/*...schild processes:0:*/
#define	FD_STDIN  0
#define	FD_STDOUT 1
#define	FD_STDERR 2

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

static const char *shell_to_use(void)
	{
	const char *p;
	return (p = getenv("SHELL")) != NULL ? p : "sh";
	}

AE_BOOLEAN shell(const char *command)
	{
	int tty, result;

	tty = open("/dev/tty", O_RDWR);

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

	return result != -1;
	}

AE_BOOLEAN xfilter(const char *command, const char *in_fn, const char *out_fn)
	{
	int fd_in, fd_out, result;

	/* Open input and output files */

	fd_in  = open(in_fn,  O_RDONLY);
	fd_out = creat(out_fn, 0600);
				/* Read and write owner only */

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
	if ( (ae = getenv("HOME")) == NULL )
		ae = ".";
	}

void mdep_deinit(void)
	{
	}
/*...e*/
