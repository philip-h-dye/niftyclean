/* utilities.c -- generic functions for niftyclean
 *
 * Written by Jay Laefer and Mike Darweesh
 *
 * (C) Copyright 1989, by Jay Laefer and Mike Darweesh
 * All Rights Reserved.
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The authors are not responsible for any damage caused by
 * this program.
 */
#include "niftyclean.h"

/* errorh() takes an error level and an error message.  It prints
   out an appropriate error (with the provided message).  If the
   level is anything but WARNING, the program exits.
   */
void 
errorh (int level, char *message)
{
    switch (level) {
    case WARNING:
	fputs("Warning",stderr);
	break;
    case INTERNAL:
	fputs("Internal Error",stderr);
	break;
    case FATAL:
	fputs("Fatal Error",stderr);
	break;
    default:
	errorh(INTERNAL, "Error in error handling");
    }
    
    fputs(": ", stderr);
    fputs(message, stderr);
    if (putc('\n', stderr) == EOF || (level & (INTERNAL | FATAL))) {
	fputs("Program Terminated.\n", stderr);
	exit(level);
    }
}

/* getfirstchar() takes a stream and returns the first character
   that is neither a space nor a tab.
   */
int 
getfirstchar (FILE *stream)
{
    int c = ' ', b = ' ';
    
    while ((c == ' ') || (c == '\t'))
      c = getc(stream);

    if (c == EOF)
      c = '\n';

    if (c != '\n')
      while ((b != '\n') && (b != EOF))
	b = getc(stream);
    
    return(c);
}

/* parse_time() takes a string and converts it to an int.
   The int must be less than MAXTIME.  It is converted
   from days to seconds before being returned.
   */
int 
parse_time (char *word)
{
    int timeout = 0;
    char *c, errmsg[128];

    for (c = word; *c != '\0'; c++) {
	if ((*c < '0') || (*c > '9'))
	  errorh(FATAL, "Non-number in time argument.");
	timeout = 10*timeout + *c - '0';
	if (timeout > MAXTIME) {
	    strcpy(errmsg, "Time-out period must be no greater than ");
	    strcat(errmsg, MAXTIMESTR);
	    strcat(errmsg, " days.");
	    errorh(FATAL, errmsg);
	}
    }

    return(timeout * 86400);	/* Seconds/day */
}
