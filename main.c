/* main.c -- argument parsing, etc. for niftyclean
 *
 * Written by Jay Laefer and Mike Darweesh
 *
 * (C) Copyright 1989, by Jay Laefer and Mike Darweesh
 * All Rights Reserved.
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The authors are not responsible for any damage caused by
 * this program.
 *
 * <<Program log moved to ChangeLog>>
 * Copyright (c) 1991- Charles Swiger
 */

#include "niftyclean.h"

#ifdef VICE
extern void free_volume_list();
extern int init_volume_list();
#endif

int flag = 0;			/* Global flag for switches */
int minimum_age;		/* Latest file creation date */

/* vers() prints out the version number of the program
 */
static void 
vers (void)
{
    puts("NiftyClean:\n");
    puts("Version 3.2     (2003/7/4)  by Charles Swiger <chuck@pkix.net>");
    puts("Version 2.6     (1991/1/1)  by Charles Swiger <cs4w@cmu.edu>");
    puts("Version 1.0-2.5 (1989/2/20) by Jay Laefer and Mike Darweesh\n");
    puts("No Warranty Implied Or Given.  Use At Your Own Risk.");
    puts("Caveat Hacktor.  :-)  This is a Nifty Program.");
    exit(0);
}

/* usage() prints out the proper usage of the program */
static void 
usage (void)
{
#ifdef VICE
    puts("Usage: clean [-bifloqVw] [-t <days>] [-eE <glob>] [-xX <dir>] [<dir> ...]");
#else
    puts("Usage: clean [-bifloqV] [-t <days>] [-eE <glob>] [-xX <dir>] [<dir> ...]");
#endif
    exit(-1);
}

/* main() parses switch arguments, sets the proper flags, selects
 * the starting directory, and begins the traversal
 */
int 
main (int argc, char **argv)
{
    char dir[MAXPATHLEN];	/* the dir name that we will start from */
    int	c, timeout;
    extern int optind, opterr;	/* stuff for getopt() */
    extern char *optarg;
    int onvice = 0;
    
    opterr = 0;
    
    /* parse the switches */
    while ((c = getopt(argc, argv, ARGSTRING)) != EOF) {
	switch (c) {
	    /* note that BATCH, INTERACTIVE, and FORCE
	       are mutually exclusive */
	case 'i':
	    if (flag & (BATCH | FORCE)) usage();
	    flag |= INTERACTIVE;
	    break;
	case 'f':
	    if (flag & (BATCH | INTERACTIVE)) usage();
	    flag |= FORCE;
	    break;
	case 'b':
	    if (flag & (FORCE | INTERACTIVE)) usage();
	    flag |= BATCH;
	    break;
	case 'o':
	    flag |= OBJECTS;
	    break;
	case 'l':
	    flag |= FLAT;
	    break;
	case 'q':
	    flag |= QUIET;
	    break;
	case 't':
	    if (flag & TIME) usage();
	    timeout = parse_time(optarg);
	    flag |= TIME;
	    break;
	case 'V':
	    vers();
	    break;
	    /* Take care of command-line glob by making a linked list of them.
	       Also, ADDGLOB and NOGLOB are mutually exclusive */
	case 'E':
	    if (flag & NOGLOB) usage();
	    flag |= ADDGLOB;
	    add_glob(optarg);
	    break;
	case 'e':
	    if (flag & ADDGLOB) usage();
	    flag |= NOGLOB;
	    add_glob(optarg);
	    break;
	    /* Also take care of command-line directory exclusion.
	       ADDEXCL and NOEXCL are mutually exclusive */
	case 'X':
	    if (flag & NOEXCL) usage();
	    flag |= ADDEXCL;
	    add_excl_dir(optarg);
	    break;
	case 'x':
	    if (flag & ADDEXCL) usage();
	    flag |= NOEXCL;
	    add_excl_dir(optarg);
	    break;
#ifdef VICE			/* mount point traversals */
/*	case 'a':                  commented out
	    flag |= BACKUP;
	    break;
	case 'r':                  commented out
	    flag |= READONLY;
	    break; */
	case 'w':
	    flag |= READWRITE;
	    break;
#endif
	default:
	    usage();
	    break;
	}
    }
    
    if (flag & TIME)		/* get current time, if necessary */
      minimum_age = time(0) - timeout;
    
    /* if not interactive, batch, or force, then set to batch */
    if (!(flag & (INTERACTIVE | BATCH | FORCE)))
      flag |= BATCH;
    
    do_rc();
    
    /* assign the approriate directory */
    if (argc == optind) {
	if (getwd(dir) == NULL)
	  errorh(FATAL,"Can't get working directory");
	
#ifdef VICE
	onvice = !init_volume_list(dir);
#endif
	
	traverse(dir,0,onvice);
    } else {
	while (optind != argc) {
	    strcpy(dir, argv[optind++]);
	    
#ifdef VICE
	    free_volume_list();
	    onvice = !init_volume_list(dir);
#endif
	    traverse(dir,0,onvice);
	}
    }
    
    if (flag & BATCH)
      dobatch();
    
    if (!(flag & FORCE))
      puts("\nHave a Nifty Day!");

    exit(0);
}
