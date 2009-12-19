/* declarations for getopt and envargs */

extern int opterr;
extern int optind;
extern int optopt;
extern char *optarg;

extern int pgis_getopt(int argc, char **argv, char *opts);
extern void envargs(int *pargc, char ***pargv, char *envstr);
