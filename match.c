/* match.c -- globbing for niftyclean
 *
 * (C) Copyright 1989, by Jay Laefer and Mike Darweesh
 * Permission is granted to copy, modify, and use this as long
 * as this message remains intact.  This is a nifty program.
 * The authors are not responsible for any damage caused by
 * this program.
 * (C) Copyright 1991- by Charles Swiger
 */

static int amatch(char *as, char *ap);
static int umatch(char *s, char *p);

/* match() takes a string and a pattern.  If the string starts
 * with a period and the pattern does not, failure (0) is
 * returned.  Otherwise, return the value that amatch() returns.
 */
int 
match (char *s, char *p)
{
    if (*s == '.' && *p != '.')
      return 0;
    return(amatch(s, p));
}

/* amatch() takes a string and a pattern.  It does some funky
 * globbing that I don't fully understand and returns 0 if the
 * match fails.
 */
static int 
amatch (char *as, char *ap)
{
    register char *s, *p;
    register int scc;
    int c, cc, ok, lc;
    
    s = as;
    p = ap;
    if ((scc = *s++))
      if ((scc &= 0177) == 0)
	scc = 0200;
    switch (c = *p++) {
	
    case '[':
	ok = 0;
	lc = 077777;
	while ((cc = *p++)) {
	    if (cc == ']') {
		if (ok)
		  return amatch(s, p);
		else
		  return 0;
	    } else if (cc == '-') {
		if (lc <= scc && scc <= (c = *p++))
		  ok++;
	    } else
	      if (scc == (lc = cc))
		ok++;
	}
	return 0;
	
    default:
	if (c != scc)
	  return 0;
	
    case '?':
	if (scc)
	  return(amatch(s, p));
	return 0;
	
    case '*':
	return(umatch(--s, p));
	
    case '\0':
	return(!scc);
    }
}

/* umatch() takes a string and a pattern.  If the pattern
 * is empty, return true (1).  Otherwise, call amatch()
 * until the string is empty or amatch() succeeds.
 */
static int 
umatch (char *s, char *p)
{
    if(*p == 0)
      return 1;
    while(*s)
      if (amatch(s++,p))
	return 1;
    return 0;
}

