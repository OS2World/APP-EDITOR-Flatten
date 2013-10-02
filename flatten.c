/*

flatten.c - Flatten a folded file

*/

/*...sincludes:0:*/
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#if defined(O16) || defined(OS2) || defined(WIN32)
#include "io.h"
#endif
#include "ere.h"
#include "types.h"
#include "mdep.h"

/*...vtypes\46\h:0:*/
/*...vmdep\46\h:0:*/
/*...e*/

static char progname[] = "flatten";

/*...suseful stuff:0:*/
#define	TERM ':'

/*...sfatal:0:*/
static void fatal(const char *fmt, ...)
	{
	va_list	vars;
	char s[256+1];
	va_start(vars, fmt);
	vsprintf(s, fmt, vars);
	va_end(vars);
	fprintf(stderr, "%s: %s\n", progname, s);
	exit(1);
	}
/*...e*/
/*...swarning:0:*/
static void warning(const char *fmt, ...)
	{
	va_list	vars;
	char s[256+1];
	va_start(vars, fmt);
	vsprintf(s, fmt, vars);
	va_end(vars);
	fprintf(stderr, "%s: %s\n", progname, s);
	}
/*...e*/
/*...susage:0:*/
static void usage(void)
	{
	fprintf(stderr, "usage: %s [-f] [-F] [-v] [-V] [-l] [-a|-A] [-r] [-i inifn] [-d lang] {fn}\n", progname);
	fprintf(stderr, "flags: -f        display contents of open folds\n");
	fprintf(stderr, "       -F        display contents of all folds (implies -f)\n");
	fprintf(stderr, "       -v        display contents of open virtual folds\n");
	fprintf(stderr, "       -V        display contents of all virtual folds (implies -v)\n");
	fprintf(stderr, "       -l        disable output of fold lines\n");
	fprintf(stderr, "       -a -A     highlight output in colour, or don't (ANSI)\n");
	fprintf(stderr, "       -r        show reserved comment start rules first\n");
	fprintf(stderr, "       -i inifn  specify initialisation file to scan for language info\n");
	fprintf(stderr, "       -d lang   override default language (used if can't guess the language\n");
	fprintf(stderr, "                 from the name, or if reading stdin, default is \"top\")\n");
	fprintf(stderr, "       fn        filename(s), no extension assumed (- for stdin)\n");
	exit(1);
	}
/*...e*/
/*...sget_line:0:*/
static AE_BOOLEAN get_line(char *line, FILE *fp)
	{
	int len;
	if ( fgets(line, WIDE+1, fp) == NULL )
		return AE_FALSE;
	len = strlen(line);
	if ( len && line[len - 1] == '\n' )
		line[len - 1] = '\0';
	return AE_TRUE;
	}
/*...e*/
/*...sskip_space:0:*/
static const char *skip_space(const char *s)
	{
	while ( *s && isspace(*s) )
		s++;
	return s;
	}
/*...e*/
/*...sscan_num:0:*/
static const char *scan_num(const char *from, unsigned *to)
	{
	for ( *to = 0; isdigit(*from); from++ )
		*to = (unsigned) (*to * 10 + (*from - '0'));
	return from;
	}
/*...e*/
/*...sscan_quoted_str:0:*/
static const char *scan_quoted_str(const char *from, char *to)
	{
	if ( *from++ != '\"' )
		return NULL;

	while ( *from && *from != '\"' )
		if ( *from == '\\' )
			/* Escaped the meaning */
			{
			if ( *(++from) )
				*to++ = *from++;
			else
				return NULL;
			}
		else
			*to++ = *from++;
	if ( !*from )
		return NULL;

	*to = '\0';

	return ++from;
	}
/*...e*/
/*...sskimming:0:*/
#define	DELIM	-1

static const char *skim_char(const char *from, int *to)
	{
	if ( *from == TERM )
		{
		*to = DELIM;
		from++;
		}
	else if ( *from != '\\' )
		*to = *from++;
	else
		{
		unsigned ch;
		from = scan_num(++from, &ch) + 1;
		*to = (int) ch;
		}
	return from;
	}

static const char *skim_str(const char *from, char *to, int len)
	{
	int c;
	from = skim_char(from, &c);
	while ( c != DELIM && len-- )
		{
		*to++ = (char) c;
		from = skim_char(from, &c);
		}
	while ( c != DELIM )
		from = skim_char(from, &c);
	*to = '\0';
	return from;
	}

static const char *skim_num(const char *from, unsigned *to)
	{
	return scan_num(from, to) + 1;
	}
/*...e*/
/*...spad:0:*/
static void pad(int n)
	{
	while ( n > 8 )
		{
		putchar('\t'); n -= 8;
		}
	while ( n-- )
		putchar(' ');
	}
/*...e*/
/*...sdequote_vfold_fn:0:*/
static void dequote_vfold_fn(const char *from, char *to)
	{
	from = skip_space(from);
	while ( *from && *from != ' ' && *from != '\t' )
		{
		if ( *from == '\'' && from[1] )
			++from;
		*to++ = *from++;
		}
	*to = '\0';
	}	
/*...e*/
/*...e*/
/*...scolour_\42\:0:*/
/* Here we abstract away the platform specific details of colour hilighting. */

static AE_BOOLEAN colour = AE_FALSE;

#ifdef WIN32
#include <io.h>
#define	WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef WIN32_LEAN_AND_MEAN
static HANDLE hConOut;
static CONSOLE_SCREEN_BUFFER_INFO csbiSaved;
static CHAR_INFO chi;
#endif

static AE_BOOLEAN colour_autodetect(void)
	{
#if defined(O16) || defined(OS2) || defined(WIN32)
	if ( isatty(fileno(stdout)) )
		return AE_TRUE;
#elif defined(UNIX)
	if ( isatty(fileno(stdout)) )
		{
		const char *term;
		if ( (term = getenv("TERM")) != NULL )
			if ( !memcmp(term, "vt", 2)    ||
			     !memcmp(term, "xterm", 5) ||
			     !strcmp(term, "linux")    ||
			     !strcmp(term, "aixterm")  ||
			     !strcmp(term, "hft")      )
				return AE_TRUE;
		}
#endif
	return AE_FALSE;
	}

static void colour_init(void)
	{
	if ( colour )
#ifdef WIN32
		{
		hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
		GetConsoleScreenBufferInfo(hConOut, &csbiSaved);
		SetConsoleTextAttribute(hConOut,
			FOREGROUND_RED  |
			FOREGROUND_GREEN|
			FOREGROUND_BLUE );
		}
#else
		fputs("\033[37m", stdout); /* White */
#endif
	}

static void colour_normal(void)
	{
	if ( colour )
#ifdef WIN32
		SetConsoleTextAttribute(hConOut,
			FOREGROUND_RED  |
			FOREGROUND_GREEN|
			FOREGROUND_BLUE );
#else
		fputs("\033[37m", stdout); /* White */
#endif
	}

static void colour_tag(void)
	{
	if ( colour )
#ifdef WIN32
		SetConsoleTextAttribute(hConOut,
			FOREGROUND_RED  |
			FOREGROUND_GREEN);
#else
		fputs("\033[33m", stdout); /* Yellow */
#endif
	}

static void colour_term(void)
	{
#ifdef WIN32
	if ( colour )
		SetConsoleTextAttribute(hConOut, csbiSaved.wAttributes);
#endif
	}
/*...e*/
/*...smain:0:*/
static AE_BOOLEAN opt_f = AE_FALSE;
static AE_BOOLEAN opt_F = AE_FALSE;
static AE_BOOLEAN opt_v = AE_FALSE;
static AE_BOOLEAN opt_V = AE_FALSE;
static AE_BOOLEAN opt_l = AE_FALSE;
static const char *ini_fn = NULL;
static const char *def_lang = "top";

/*...src_st_of:0:*/
typedef struct
	{
	char name   [WIDE+1];
	char rc_st  [WIDE+1];
	char rc_end [WIDE+1];
	char reg_exp[WIDE+1];
	} LOOKUP;

#define	MAX_LOOKUPS 100

static LOOKUP *lookups[MAX_LOOKUPS];

static int n_lookups = 0;

static AE_BOOLEAN matches(const char *pattern, const char *reg_exp)
	{
	AE_BOOLEAN yes;
	ERE *ere;
	const char *os_fn;
	int end, rc;

	if ( (os_fn = get_os_fn(pattern)) == NULL )
		return AE_FALSE;	/* Filename not decent, ignore */
	if ( (ere = ere_compile(reg_exp, 0, &rc)) == NULL )
		return AE_FALSE;	/* Awkward error, assume no match */

	yes = (end = ere_match(ere, EREMF_ANY, os_fn, 0, NULL)) != -1 &&
	       end == (int) strlen(os_fn);

	ere_discard(ere);
	return yes;
	}

static const char *rc_st_of(const char *fn)
	{
	int i;
	const char *os_fn = get_os_fn(fn);

	for ( i = 0; i < n_lookups; i++ )
		if ( matches(os_fn, lookups[i]->reg_exp) )
			return lookups[i]->rc_st;

	for ( i = 0; i < n_lookups; i++ )
		if ( !strcmp(def_lang, lookups[i]->name) )
			return lookups[i]->rc_st;

	fatal("cannot find default language \"%s\"", def_lang);
	return NULL;
	}

static void show_rules(void)
	{
	int i;

	printf("\n");
	printf("Name         Cmt.Start    Cmt.End      Regular Expression\n");
	printf("----         ---------    -------      ------------------\n");
	for ( i = 0; i < n_lookups; i++ )
		printf("%-12s %-12s %-12s %s\n",
			lookups[i]->name,
			lookups[i]->rc_st,
			lookups[i]->rc_end,
			lookups[i]->reg_exp);

	printf("\n%d languages, default language is \"%s\"\n",
		n_lookups, def_lang);
	}
/*...e*/
/*...sread_ini_file:0:*/
/*...sadd_definition:0:*/
static void add_definition(const char *line)
	{
	LOOKUP *lookup;

	if ( (lookup = (LOOKUP *) malloc(sizeof(LOOKUP))) == NULL )
		fatal("out of memory");

	line = skip_space(line);
	if ( strncmp(line, "language_create", 15) || !isspace(line[15]) )
		return; /* Is not a language definition line */

	line = skip_space(line + 16);
	if ( (line = scan_quoted_str(line, lookup->name)) == NULL )
		fatal("bad language name in initalisation file");

	line = skip_space(line);
	if ( (line = scan_quoted_str(line, lookup->rc_st)) == NULL )
		fatal("bad reserved Cmt.Start in initalisation file");

	line = skip_space(line);
	if ( (line = scan_quoted_str(line, lookup->rc_end)) == NULL )
		fatal("bad reserved Cmt.End in initalisation file");

	line = skip_space(line);
	if ( (line = scan_quoted_str(line, lookup->reg_exp)) == NULL )
		fatal("bad regular expression in initalisation file");

	lookups[n_lookups++] = lookup;
	}
/*...e*/

static AE_BOOLEAN read_ini_file(const char *ini_fn)
	{
	FILE *fp;
	char line[WIDE+1];
	if ( (fp = fopen(ini_fn, "r")) == NULL )
		return AE_FALSE;
	while ( get_line(line, fp) )
		if ( line[0] != ';' )
			add_definition(line);
	fclose(fp);
	return AE_TRUE;
	}
/*...e*/
/*...sini_file_logic:0:*/
static void ini_file_logic(const char *argv0)
	{
	const char *os_fn;

	if ( ini_fn != NULL )
		{
		if ( (os_fn = get_os_fn(ini_fn)) == NULL )
			fatal("bad user initialisation filename %s", ini_fn);

		if ( read_ini_file(os_fn) )
			return;

		warning("can't find user initialisation file %s", os_fn);
		}

	if ( (os_fn = get_ae_fn(argv0)) == NULL )
		fatal("bad default initialisation file name %s", argv0);

	if ( read_ini_file(os_fn) )
		return;

	os_fn = get_ae_fn("ae");

	if ( read_ini_file(os_fn) )
		return;

	fatal("can't find default or AE's initialisation file");
	}
/*...e*/
/*...sfold_line:0:*/
static void fold_line(int indent, const char *prefix, const char *tag)
	{
	if ( !opt_l )
		{
		pad(indent);
		colour_tag();
		fputs(prefix, stdout);
		fputs(tag, stdout);
		colour_normal();
		putchar('\n');
		}
	}
/*...e*/
/*...sflatten_fold:0:*/
static void flatten_file(const char *, FILE *, unsigned, const char *);

static void flatten_fold(
	const char *this_fn,
	FILE *fp,
	unsigned base_indent,
	unsigned indent,
	AE_BOOLEAN listing,
	const char *rc_st
	)
	{
	char line[WIDE+1];
	const char *s;
	AE_BOOLEAN open;
	FILE *fp_nested;
	unsigned nested_indent;
	int len_rc_st = strlen(rc_st);
	while ( get_line(line, fp) )
		/* Got a line */
		{
		if ( strncmp(line, rc_st, len_rc_st) )
			{
			if ( listing )
				{
				pad(base_indent + indent); puts(line);
				}
			}
		else
			/* Is a special line */
			{
			s = line + len_rc_st;
			open = AE_FALSE;
			switch ( *s++ )
				{
/*...sS and S:32:*/
case 'S':
	open = AE_TRUE;
case 's':
	{
	static char tag[WIDE+1];
	s = skim_str(s, tag, WIDE);
	skim_num(s, &nested_indent);					
	if ( (open && opt_f) || opt_F )
		/* Recurse on fold */
		{
		if ( listing )
			fold_line(base_indent + nested_indent, "{{{ ", tag);
		flatten_fold(this_fn, fp, base_indent, nested_indent, listing, rc_st);
		if ( listing )
			fold_line(base_indent + nested_indent, "}}}", "");
		}
	else
		{
		if ( listing )
			fold_line(base_indent + nested_indent, "... ", tag);
		flatten_fold(this_fn, fp, base_indent, nested_indent, AE_FALSE, rc_st);
		}
	}
	break;
/*...e*/
/*...sV and v:32:*/
case 'V':
	open = AE_TRUE;
case 'v':
	{
	static char tag[WIDE+1];
	s = skim_str(s, tag, WIDE);
	skim_num(s, &nested_indent);
	if ( (open && opt_v) || opt_V )
		/* Will recurse */
		{
		if ( listing )
			{
			static char vfn[WIDE+1];
			static char fn[WIDE+1];
			const char *full_fn;
			char nested_fn[WIDE+1];
			fold_line(base_indent + nested_indent, ">>> ", tag);
			dequote_vfold_fn(tag, vfn);
			expand_fn(vfn, fn);
			if ( (full_fn = get_nest_fn(this_fn, fn)) == NULL )
				fatal("can't construct nested filename");
			strcpy(nested_fn, full_fn);
			if ( (fp_nested = fopen_file(nested_fn, "r")) == NULL )
				fatal("can't open file %s", full_fn);
			flatten_file(nested_fn, fp_nested, base_indent + nested_indent, rc_st_of(fn));
			fclose(fp_nested);
			fold_line(base_indent + nested_indent, "<<< ", this_fn);
			}
		}
	else
		{
		if ( listing )
			fold_line(base_indent + nested_indent, "::: ", tag);
		}
	}
	break;
/*...e*/
/*...sE and e:32:*/
case 'e':
case 'E':
	return;
/*...e*/
/*...sbad reserved comment:32:*/
default:
	fatal("bad reserved comment");
/*...e*/
				}
			}
		}
	fatal("corrupt folded file, too many 'start folds'");
	}
/*...e*/
/*...sflatten_file:0:*/
static void flatten_file(
	const char *this_fn,
	FILE *fp,
	unsigned base_indent,
	const char *rc_st
	)
	{
	char line[WIDE+1];
	const char *s;
	unsigned indent = 0;
	AE_BOOLEAN open;
	FILE *fp_nested;
	int len_rc_st = strlen(rc_st);

	while ( get_line(line, fp) )
		/* Got a line */
		{
		if ( strncmp(line, rc_st, len_rc_st) )
			{
			pad(base_indent); puts(line);
			}
		else
			/* Is a special line */
			{
			s = line + len_rc_st;
			open = AE_FALSE;
			switch ( *s++ )
				{
/*...sS and s:32:*/
case 'S':
	open = AE_TRUE;
case 's':
	{
	static char tag[WIDE+1];
	s = skim_str(s, tag, WIDE);
	skim_num(s, &indent);					
	if ( (open && opt_f) || opt_F )
		/* Recurse on fold */
		{
		fold_line(base_indent + indent, "{{{ ", tag);
		flatten_fold(this_fn, fp, base_indent, indent, AE_TRUE, rc_st);
		fold_line(base_indent + indent, "}}}", "");
		}
	else
		{
		fold_line(base_indent + indent, "... ", tag);
		flatten_fold(this_fn, fp, base_indent, indent, AE_FALSE, rc_st);
		}
	}
	break;
/*...e*/
/*...sV and v:32:*/
case 'V':
	open = AE_TRUE;
case 'v':
	{
	static char tag[WIDE+1];
	s = skim_str(s, tag, WIDE);
	skim_num(s, &indent);
	if ( (open && opt_v) || opt_V )
		/* Will recurse */
		{
		static char vfn[WIDE+1];
		static char fn[WIDE+1];
		const char *full_fn;
		char nested_fn[WIDE+1];
		fold_line(base_indent + indent, ">>> ", tag);
		dequote_vfold_fn(tag, vfn);
		expand_fn(vfn, fn);
		if ( (full_fn = get_nest_fn(this_fn, fn)) == NULL )
			fatal("can't construct nested filename");
		strcpy(nested_fn, full_fn);
		if ( (fp_nested = fopen_file(nested_fn, "r")) == NULL )
			fatal("can't open file %s", full_fn);
		flatten_file(nested_fn, fp_nested, base_indent + indent, rc_st_of(fn));
		fclose(fp_nested);
		fold_line(base_indent + indent, "<<< ", this_fn);
		}
	else
		fold_line(base_indent + indent, "::: ", tag);
	}
	break;
/*...e*/
/*...sE and e\44\ not expected at this time:32:*/
case 'e':
case 'E':
	fatal("corrupt folded file, too many 'end folds'");
/*...e*/
/*...sbad reserved comment:32:*/
default:
	fatal("bad reserved comment");
/*...e*/
				}
			}
		}
	}
/*...e*/

int main(int argc, char *argv[])
	{
	int i;
	AE_BOOLEAN rules = AE_FALSE, dc = AE_TRUE;

	if ( argc == 1 )
		usage();

	for ( i = 1; i < argc; i++ )
		{
		if ( argv[i][0] != '-' )
			break;
		else if ( argv[i][1] == '-' )
			{ ++i; break; }
		else if ( argv[i][1] == '\0' )
			break;
		switch ( argv[i][1] )
			{
			case 'f': opt_f = AE_TRUE;			break;
			case 'F': opt_F = AE_TRUE;			break;
			case 'v': opt_v = AE_TRUE;			break;
			case 'V': opt_V = AE_TRUE;			break;
			case 'l': opt_l = AE_TRUE;			break;
			case 'r': rules = AE_TRUE;			break;
			case 'a': colour = AE_TRUE ; dc = AE_FALSE;	break;
			case 'A': colour = AE_FALSE; dc = AE_FALSE;	break;
			case 'i': if ( ++i == argc )
					usage();
				  ini_fn = argv[i];
				  break;
			case 'd': if ( ++i == argc )
					usage();
				  def_lang = argv[i];
				  break;
			default:  usage();				break;
			}
		}

	if ( dc )
		colour = colour_autodetect();

	mdep_init();
	colour_init();
	colour_normal();

	ini_file_logic(argv[0]);

	if ( rules )
		show_rules();

	for ( ; i < argc; i++ )
		if ( !strcmp(argv[i], "-") )
			flatten_file("", stdin, 0, rc_st_of(def_lang));
		else
			{
			char copy_fn[WIDE+1];
			FILE *fp;
			const char *fn = argv[i];
			const char *os_fn;
			if ( (os_fn = get_os_fn(fn)) == NULL )
				{
				colour_term();
				mdep_deinit();
				fatal("bad filename %s", fn);
				}
			strcpy(copy_fn, os_fn);
			if ( (fp = fopen(copy_fn, "r")) == NULL )
				{
				colour_term();
				mdep_deinit();
				fatal("can't open file %s", copy_fn);
				}
			flatten_file(copy_fn, fp, 0, rc_st_of(copy_fn));
			fclose(fp);
			}

	colour_term();
	mdep_deinit();

	return 0;
	}
/*...e*/
