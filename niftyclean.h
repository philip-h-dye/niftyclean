/* niftyclean.h -- global defines for niftyclean
 *
 * Written by Jay Laefer and Mike Darweesh
 *
 * (C) Copyright 1989, by Jay Laefer and Mike Darweesh
 * All Rights Reserved.
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The authors are not responsible for any damage caused by
 * this program.
 * (C) Copyright 1991- by Charles Swiger
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <time.h>
#include <unistd.h>

/*
 * prototypes
 */

/* main.c */
extern int main(int argc, char **argv);
extern int flag;		/* Flag containing all switches */
extern int skip;		/* Flag for interactive skip directory */
extern time_t minimum_age;      /* Latest file creation date */

/* match.c */
extern int match(char *s, char *p);

/* parse_rc.c */
extern void do_rc(void);
extern void add_batch(char *path);
extern int dofile(char *dir, char *file);
extern void dobatch(void);
extern void add_glob(char *word);
extern void add_excl_dir(char *word);
extern int check_excl_list(char *dir);

/* traverse.c */
extern void traverse(char *d, int leaf, int onvice);
#ifdef VICE
extern int init_volume_list(char *);
extern void free_volume_list(void);
#endif

/* utilities.c */
extern void errorh(int level, char *message);
extern int getfirstchar(FILE *stream);
extern int parse_time(char *word);

/* error values */
#define WARNING         0x1
#define INTERNAL        0x2
#define FATAL           0x4

/* constants for switch flags */
#ifdef VICE
/* #define	BACKUP		0x1 */
/* #define	READONLY	0x2 */
#endif

#define	READWRITE	0x4
#define	NOGLOB		0x8
#define	ADDGLOB		0x10
#define NOEXCL		0x20
#define ADDEXCL		0x40
#define	BATCH		0x80
#define	FORCE		0x100
#define	INTERACTIVE	0x200
#define	FLAT		0x400
#define	OBJECTS		0x800
#define	QUIET		0x1000
#define TIME		0x2000

#define	MAXPATTERNSIZE	1024	/* largest glob pattern */
#define MAXSIZE		2048	/* largest block size for pioctl() */
#define MAXTIME		365	/* limit maximum time to one year */
#define MAXTIMESTR	"365"	/* so we can avoid sprintf() */

#define	RCFILE		".cleanrc"

/* acceptable switches */
#ifdef VICE
/* #define ARGSTRING "abe:E:filoqrt:Vwx:X:" */
#define ARGSTRING "be:E:filoqt:Vwx:X:"
#else
#define ARGSTRING "be:E:filoqt:Vx:X:"
#endif

/* structure definitions */
struct globtype {
    struct globtype *next;
    char glob[1];
};

struct excl_dirtype {
    struct excl_dirtype *next;
    char excl_dir[1];
};

struct batchtype {
    struct batchtype *next;
    char path[1];
};

struct dirtype {
    int len;
    int leaf;
    struct dirtype *next;
    char subdir[1];
};

#ifdef VICE
struct voltype {
    struct voltype *next;
    long Vid;
};
#endif
