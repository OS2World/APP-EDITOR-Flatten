/*							

mdep.h - Interface to machine specifics of Andys Editor

*/

extern void PASFUNC expand_fn(const char *s, char *fn);
extern int PASFUNC cmp_os_fn(const char *fn1, const char *fn2);
extern void PASFUNC get_match_fn(const char *os_fn, char *match_fn);
extern const char * PASFUNC get_os_fn(const char *fn);
extern const char * PASFUNC get_ae_fn(const char *argv0);
extern const char * PASFUNC get_bak_fn(const char *fn);
extern const char * PASFUNC get_nest_fn(const char *fn_main, const char *fn_nested);
extern AE_BOOLEAN PASFUNC unlink_file(const char *fn);
extern AE_BOOLEAN PASFUNC rename_file(const char *old_fn, const char *new_fn);
extern int PASFUNC get_filemode(const char *fn);
extern void PASFUNC set_filemode(const char *fn, int mode);
extern AE_BOOLEAN PASFUNC is_readonly(const char *fn);
extern FILE * PASFUNC fopen_file(const char *fn, const char *mode);
extern FILE * PASFUNC fopen_tmp_file(char *fn, const char *mode);

#ifndef FLATTEN
extern KEY key_tab[];

extern AE_BOOLEAN PASFUNC shell(const char *cmd);
extern AE_BOOLEAN PASFUNC xfilter(const char *cmd, const char *fi, const char *fo);
#endif

extern void PASFUNC unhog(void);

extern void PASFUNC mdep_init(void);
extern void PASFUNC mdep_deinit(void);
