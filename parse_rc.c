/* parse_rc.c--rcfile parser for niftyclean
 *
 * Written by Jay Laefer and Mike Darweesh
 * (C) Copyright 1989 by Jay Laefer and Mike Darweesh
 * All Rights Reserved.
 * Permission is granted to copy, modify, and use this as long
 * as this notice remains intact.  This is a nifty program.
 * (C) Copyright 1991- by Charles Swiger
 */

#include "niftyclean.h"
#include <stdlib.h>

static struct globtype *globlist = NULL;
static struct batchtype *batchlist = NULL;
static struct excl_dirtype *excl_dirlist = NULL;

/* Here is the default list of regex's to terminate; should move this to a 
 * system-wide config file and not hardcode it within the binary.
 */

char *defaults[] = { 
    "core",
    "*~",
    ".*~",
    "*.BAK",
    ".*.BAK",
    "*.CKP",
    ".*.CKP",
    "*.NEW",
    ".*.NEW",
    "#*#",
    ".emacs_[0-9]*",
    "dead.letter",
    "*.otl",
    ".*.otl",
    "*.backup",
    ".*.backup",
    NULL
};

char *def_excl_dirs[] = {
    ".MESSAGES",
    NULL
};

char *objects[] = {
    "*.o",
    "*.pyc",
    "*.pyo",
    NULL
};

/* do_default_rc() goes through the array of default globs and adds each item
 * to the global list of patterns.  It then goes through the array of default
 * directories and adds them to the list of directories to be excluded.
 */
static void 
do_default_rc(void)
{
    char **def_ptr;
    
    if (!(flag & NOGLOB)) {
	def_ptr = defaults;
        while (*def_ptr) {
            add_glob(*def_ptr++);
        }
    }

    if (!(flag & NOEXCL)) {
	def_ptr = def_excl_dirs;
        while (*def_ptr) {
            add_glob(*def_ptr++);
        }
    }
}

void 
do_rc(void)
{
    FILE *fp;
    char *home, rcfile[MAXPATHLEN], line[1024], **def_ptr;
    int noglob, noexcl;

    noglob = flag & NOGLOB;
    noexcl = flag & NOEXCL;

    if (flag & OBJECTS) {
	def_ptr = objects;
        printf("in objects, def_ptr = %s\n", *def_ptr);
        while (*def_ptr) {
            add_glob(*def_ptr++);
        }
    }
       
    /* return if we don't want the defaults */
    if(noglob && noexcl)
        return;

    /* get home directory */
    home = getenv("HOME");
    if (home == NULL) {
	do_default_rc();
	return;
    }
    
    if ((strlen(home) + strlen(RCFILE)) > MAXPATHLEN)
        errorh(FATAL,"Pathlength to ~/.cleanrc too long");
    
    strcpy(rcfile, home);
    strcat(rcfile, "/");
    strcat(rcfile, RCFILE);
    
    /* open .cleanrc */
    if ((fp = fopen(rcfile, "r")) == NULL) {
	do_default_rc();
	return;
    }
    
    /* read patterns from file */
    while (fgets(line, MAXPATHLEN, fp) != NULL) {
	int len, c;
	
	len = strlen(line) - 1;
	if (line[len] == '\n')
            line[len] = '\0';
	else
            while (((c = getc(fp)) != EOF) && (c != '\n'));

	/* check for comment or blank line */
	if ((line[0] != '#') && (line[0] != '\0')) {
	    if (line[0] == '!') {
		if (!noexcl)
                    add_excl_dir(line + 1);
	    } else if (!noglob) {
		if ((line[0] == '\\') &&
		    ((line[1] == '#') || (line[1] == '!') || (line[1] == '\\')))
                    add_glob(line + 1);
		else
                    add_glob(line);
	    }
	}
    }		   
    
    if (fclose(fp))
        errorh(WARNING,"Problem closing ~/.cleanrc");
}

/* find_match() takes a string and goes through the list of patterns until it
 * either finds a match or runs out of patterns.
 */
static int
find_match (char *file)
{
    struct globtype *temp;
    
    temp = globlist;
    while (temp) {
	if (match(file, temp->glob))
            return 1;
	temp = temp->next;
    }
    return 0;
}

/* dofile() takes a directory and a filename and acts based on whether FORCE,
 * BATCH, or INTERACTIVE mode is on.
 */
int 
dofile (char *dir, char *file)
{
    char    unlinkerr[MAXPATHLEN + 100], c;
    
    if (find_match(file)) {	/* if the file matches a pattern */
	char fullpath[MAXPATHLEN];
	
	strcpy(fullpath, dir);
	strcat(fullpath, "/");
	strcat(fullpath, file);
	
	if (flag & FORCE)
            (void)unlink(fullpath);
	else if (flag & BATCH)
            add_batch(fullpath);
	else {
	    fputs("Remove ", stdout);
	    fputs(fullpath, stdout);
	    fputs(" y/n/s [y]: ", stdout);
	    c = getfirstchar(stdin);
	    /* kill file if user wants it killed */
	    switch (c) {
              case 'y':
              case 'Y':
              case '\n':
		if (unlink(fullpath)) {
		    strcpy(unlinkerr, "Could not remove: ");
		    strcat(unlinkerr, fullpath);
		    errorh(WARNING, unlinkerr);
		} else {
		    fputs("Removed: ", stdout);
		    puts(fullpath);
		}
		break;
              case 's':
              case 'S':
		skip = 1;
		puts("Skipping this directory...");
		break;
	    }
	}
	return 1;
    } else
        return 0;
}

/* add_batch() takes the full pathname to a file that is slated for deletion.
 * It then adds that to the globab linked list of such files.
 */
void 
add_batch (char *path)
{
    struct batchtype *batchptr;
    batchptr = (struct batchtype *) malloc((sizeof (struct batchtype)
                                            + strlen(path)));
    if (batchptr == NULL)
        errorh(FATAL, "Malloc failed in add_batch()");
    
    strcpy(batchptr->path, path);    
    batchptr->next = batchlist;    
    batchlist = batchptr;
}

/* dobatch() runs through the list of files slated for deletion and asks the
 * user if he wants to delete them.
 */
void 
dobatch (void)
{
    struct batchtype *temp;
    char unlinkerr[MAXPATHLEN + 100];
    int c;
    
    if (!batchlist) {
	puts("No non-nifty files found.");
	return;
    }
    
    puts("\nThe following are non-nifty files:");
    for (temp = batchlist; temp; temp = temp->next)
        puts(temp->path);
    
    fputs("Delete them y/n [y]: ", stdout);
    c = getfirstchar(stdin);
    
    if ((c = ((c == 'y') || (c == 'Y') || (c == '\n'))))
        puts("Deleting files...");
    else
        puts("Files not deleted.");
    
    /* delete the files and free the memory */
    while (batchlist) {
	if (c && unlink(batchlist->path)) {
	    strcpy(unlinkerr, "Could not remove: ");
	    strcat(unlinkerr, batchlist->path);
	    errorh(WARNING,unlinkerr);
	}
	temp = batchlist;
	batchlist = batchlist->next;
	free(temp);
    }
}

/* add_glob() takes a word, malloc's space for a structure, and adds it to the
 * front of the list of patterns to be matched.
 */
void 
add_glob (char *word)
{
    struct globtype *globptr;

    if (0)
        printf("add_glob(): word = %s\n", word);
    
    globptr = (struct globtype *)malloc((sizeof (struct globtype)) + strlen(word));
    if (globptr == NULL)
        errorh(FATAL, "Malloc failed in add_glob()");
    
    strcpy(globptr->glob, word);
    
    globptr->next = globlist;
    
    globlist = globptr;
}

/* add_excl_dir() takes a word, malloc's space for a structure, and adds it to
 * the front of the list of directories to be excluded from the traversal.
 */
void 
add_excl_dir (char *word)
{
    struct excl_dirtype *excl_dirptr;
    if ((excl_dirptr = (struct excl_dirtype *)malloc((sizeof (struct excl_dirtype)) + strlen(word))) == NULL)
        errorh(FATAL, "Malloc failed in add_excl_dir()");

    strcpy(excl_dirptr->excl_dir, word);

    excl_dirptr->next = excl_dirlist;

    excl_dirlist = excl_dirptr;
}

/* check_excl_list() takes a directory name and checks to see if it is on the
 * list of directories to be excluded.  If so, a 0 is returned.  Otherwise,
 * return 1.
 */
int 
check_excl_list (char *dir)
{
    struct excl_dirtype *temp;

    /* Move down the linked list that starts with excl_dirlist */
    for (temp = excl_dirlist; temp; temp = temp->next)
        if (!strcmp(temp->excl_dir, dir))
            return 0;		/* Found */

    return 1;
}
