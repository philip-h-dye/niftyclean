/* traverse.c -- traverse directory structures recursively
 *
 * Written by Jay Laefer
 *
 * (C) Copyright 1989, by Jay Laefer and Mike Darweesh
 * All Rights Reserved.
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The authors are not responsible for any damage caused by
 * this program.
 */
#include "niftyclean.h"
#include <sys/stat.h>
#include <dirent.h>
#include <sys/dirent.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>

#ifdef VICE
#include <afs/param.h>
#include <sys/ioctl.h>
#include <afs/venus.h>
#include <afs/afsint.h>

static struct voltype	*vollist = NULL; /* List of volumes crossed so far */
#endif

extern void *malloc();
extern void errorh();
extern int check_excl_list();
extern int errno;

extern int flag;		/* Flag containing all switches */
extern int skip;		/* Flag for interactive skip directry */
extern int minimum_age;		/* Latest file creation date */

void traverse(), add_dir(), scan_dirs();

#ifdef VICE
void free_volume_list(), add_volume();
int init_volume_list();
#endif

/* traverse() takes a directory, opens it for reading and lstat's
   all the files in the directory.  It calls add_dir() on each
   subdirectory, and dofile() on each file.  It calls scan_dirs()
   after closing the directory in preparation for recursing.
   */
void 
traverse (
    char *d,
    int leaf,			/* nonzero if known to be leaf directory */
    int onvice			/* nonzero iff on vice. */
)
{
    DIR *dirp;
    int d_len;
    int dostat;
    
    skip = 0;
    d_len = strlen(d);		/* The length of the directory name */
    if ((d_len > 1) && (d[d_len - 1] == '/'))
      d[d_len - 1] = '\0';
    
    if (!(flag & (FORCE | QUIET))) {
	fputs("Entering: ", stdout);
	puts(d);
    }
    
    dostat = ((flag & TIME) ||
	      (!onvice && !leaf) ||
	      (onvice && (flag & READWRITE)));

    /* open the directory for reading */
    if (dirp = opendir(d)) {
	struct dirtype *head = NULL; /* The head of the subdir list */
	struct dirent *d_struct;
	
	while ((d_struct = readdir(dirp)) && (!skip)) {
	    /* Scan through the directory */
	    int f_len;
	    char *file;
	    
	    /* quick references */
	    file = d_struct->d_name;
	    f_len = strlen(file);
	    
	    /* check for exceeding MAXPATHLEN */
	    if (d_len + f_len >= MAXPATHLEN) {
		errorh(WARNING, "filename too long");
		errorh(WARNING, d);
		
		/* check for "." and ".." */
	    } else if ((f_len > 2) || (file[0] != '.') || ((file[1] != '.') && (file[1] != '\0'))) {
		/* Prep for lstat() */
		char f_name[MAXPATHLEN];
		struct stat stbuf;
		
		strcpy(f_name, d);
		strcat(f_name, "/");
		strcat(f_name, file);
		
		/* stat the file if we have to */
		if (dostat && (lstat(f_name, &stbuf) == -1)) continue;

		if ((onvice && (d_struct->d_ino & 1)) ||
		    (dostat && ((stbuf.st_mode & S_IFMT) == S_IFDIR))) {
		    if (!(flag & FLAT))
		      /* it's a dir, add it to the list */
		      add_dir(&head, file, f_len, dostat &&(stbuf.st_nlink<=2));

		} else if (!(flag & TIME) || (minimum_age > stbuf.st_mtime)) {
		    /* Pass it to dofile().
		       We check to see if the TIME flag is set,
		       and, if so, make sure the file is older than
		       then minimum_age */
		      (void)dofile(d, file);
		}
	    }
	}
	closedir(dirp);
	
	/* finished reading dir, now check out subdirs */
	if (!(flag & FLAT) && head)
	  scan_dirs(head, d, d_len);
	
    } else
      errorh(WARNING, "Couldn't open directory");
}

/* add_dir() takes a double pointer to the head of the list (so it can
   be altered), the name of the subdirectory, and the length of the
   subdir.  It malloc's a structure and space for the name, then it
   adds the structure to the front of the list that begins with *headp.
   */
void 
add_dir (struct dirtype **headp, char *file, int len, int leaf)
{
    struct dirtype *dirptr;
    
    if ((dirptr = (struct dirtype *)malloc((sizeof(struct dirtype)) + len)) == NULL)
      errorh(FATAL, "Malloc failed in add_dir()");
    
    /* copy subdir info into the structure */
    strcpy(dirptr->subdir, file);
    dirptr->len = len;
    dirptr->leaf = leaf;
    
    dirptr->next = *headp;
    
    *headp = dirptr;
}

/* scan_dirs() takes the head of the list of subdirs, the
   full path leading to them, and the length of that path.
   In the Vice version, we try to stat each subdir as a
   mount point.  This way, we make sure we don't go through
   any unwanted volumes, nor do we enter the same volume
   twice.  In the non-Vice version, we just call traverse
   on each subdir.  Either way, we free() the memory as we go.
   */
void 
scan_dirs (struct dirtype *head, char *d, int d_len)
{
    int onvice = 0;
#ifdef VICE
    struct ViceIoctl blob1, blob2;
    char space1[MAXSIZE], space2[MAXSIZE];
    
    blob1.out_size = MAXSIZE;
    blob1.out = space1;
    
    blob2.in_size = 0;
    blob2.out_size = MAXSIZE;
    blob2.out = space2;
#endif
    
    while(head) {
	struct dirtype *temp;
	char dir[MAXPATHLEN];
	
	if ((d_len == 1) && (d[0] == '/'))
	  strcpy(dir, "/");
	else {
	    strcpy(dir, d);
	    strcat(dir, "/");
	}
	strcat(dir, head->subdir);
	
	temp = head;
	
#ifdef VICE
	bzero(space1, MAXSIZE);
	bzero(space2, MAXSIZE);
	blob1.in_size = head->len + 1;
	blob1.in = head->subdir;
	
	/* The pioctl VIOC_AFS_STAT_MT_PT  takes a path (d) and a file
	   (placed in blob.in) and determines if it is a mount point
	   (return value == 0).  It returns the name of the mounted
	   volume.  Because it is possible to mount % vs. # (named vs.
	   read-only preference), we must use the pioctl VIOCGETVOLSTAT
	   which stats the full directory to get the true name of the
	   mounted volume.  This pioctl tries to stat the directory
	   that's been passed in.  If it succeeds (return value == 0),
	   we get information about the volume the directory is located
	   in. */
	if (pioctl(d, VIOC_AFS_STAT_MT_PT, &blob1, 0) == 0) {
	    /* Now we have a mount point.  If we're not in the business
	       of traversing any mount points, don't bother to get the
	       name of the volume. */
	    /* if ((flag & (READWRITE | READONLY | BACKUP)) && */
	    if ((flag & READWRITE) &&
		(pioctl(dir, VIOCGETVOLSTAT, &blob2, 0) == 0)) {
		struct VolumeStatus *status;
		/* char *name; */
		/* int bkp, ro; */

		/* bkp = ro = 0; */

		status = (struct VolumeStatus *)space2;

		/* Get the volume name */
		/* name = space2 + (sizeof(struct VolumeStatus)); */

		/* Type is 1 for a read/write volume, 0 for backup and read-only */
		/* if (!status->Type) { */
		/* char *ext; */
		/* Get the last part of the volume name */
		/* ext = strrchr(name, '.'); */
		/* if (strcmp(++ext, "backup")) */
		/* ro = 1; */
		/* else */
		/* bkp = 1; */
		/* } */
		
		/* Check to make sure we want to go through this volume */
		/* if ((((flag & READWRITE) && status->Type) || */
		/* ((flag & READONLY) && ro) || */
		/* ((flag & BACKUP) && bkp)) */
		if (((flag & READWRITE) && status->Type)
		    && (check_volume_list(status->Vid))) {
		    add_volume(status->Vid);
		    traverse(dir,head->leaf, 1);
		    }
		}
	} else
#endif
	  if (check_excl_list(head->subdir)) {
#ifdef VICE
	      if ((pioctl(dir, VIOCGETVOLSTAT, &blob2, 0) == 0) ||
		  errno != EINVAL)
		onvice = 1;
#endif		
	      traverse(dir, head->leaf, onvice);
	  }
	
	head = head->next;
	free((char *)temp);
    }
}

#ifdef VICE			/* Vice-specific functions */

/* init_volume_list() takes the initial directory the program
 * starts with and sets up the list of volumes traversed.
 * If the pioctl() fails with EINVAL, then we're not on vice
 * and we return a non-zero status.
 */
int 
init_volume_list (char *dir)
{
    struct ViceIoctl blob;
    struct VolumeStatus *status;
    char space[MAXSIZE];
    
    /* prep for pioctl() */
    blob.in_size = 0;
    blob.out_size = MAXSIZE;
    blob.out = space;
    
    vollist = NULL;		/* Init the global variable */
    
    /* This pioctl tries to stat the directory thats been passed in.
       If it succeeds (return value == 0), we get information about
       the volume the directory is located on. */
    if (pioctl(dir, VIOCGETVOLSTAT, &blob, 0) == 0) {
	status = (struct VolumeStatus *)space;
	add_volume(status->Vid); /* Add the volume to the list */
    }
    else if (errno == EINVAL) {
	return 1;
    }
    return 0;	
}

/* add_volume() takes a volume id, allocates space for the structure,
   and adds the structure to the list of visited volumes.
   */
static void 
add_volume (long volume)
{
    struct voltype *newvol;
    
    if ((newvol = (struct voltype *)malloc(sizeof(struct voltype))) == NULL)
      errorh(FATAL, "Malloc() failed in add_volume()");

    newvol->Vid = volume;	/* Get the volume ID number */
    newvol->next = vollist;
    
    vollist = newvol;
}

/* check_volume_list() takes a volume id and checks to see if it's
   already been visited.  If so, a 0 is returned.  Otherwise, return 1.
   */
static 
check_volume_list (long volume)
{
    struct voltype *temp;
    
    /* Move down the linked list that starts with vollist */
    for (temp = vollist; temp; temp = temp->next)
      if (temp->Vid == volume)
	return(0);		/* Found */
    
    return(1);			/* Not found */
}

/* free_volume_list() frees the list of volumes headed by vollist
 */
void 
free_volume_list (void)
{
    struct voltype *temp;
    
    while (vollist) {
	temp = vollist;
	vollist = vollist->next;
	free(temp);
    }
}
#endif				/* End of Vice-specific functions */
