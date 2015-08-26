/*
//	@(#) prcl.c - List C code  with intelligent line numbers
*/
# include	<stdio.h>
# include	<stdlib.h>
# include	<ctype.h>
# include	<string.h>

enum	{
	FALSE	= 0,
	TRUE	= !FALSE
};

enum	{
	ESCAPE	= '\\',
};

// Process escape sequences in strings
// eg (\\)*\" doesn't close a string but   (\\)*" does 

static	int	is_escaped (char line[], int i) {
	int	result	= FALSE;
	while (i>0 && line [i-1] == ESCAPE) {
		result	= !result;
		--i;
	}
	return 	result;
}

// If line contains one of these chars outside of a comment
// its 'interesting' and will have a line number prepended

static	int	is_interesting (int ch) {
	int	result	= isalnum (ch);
	return	result;
}

// Keep tables of opening and closing patterns for comments
// and strings.
// A table of uninteresting key words is kept by setting .close == 0

struct	sympair	{
	char*	open;
	char*	close;
};
typedef	struct	sympair	SYMPAIR;

/* quote styles */
static	SYMPAIR	QUOTES[]	= {
		{ "\"", "\"" },
		{ "'",  "'" }
};
enum	{
	NQUOTES	= (sizeof(QUOTES)/sizeof(QUOTES[0])),
};

/* comment styles */
static	SYMPAIR	COMMS[]	= {
		{ "/*", "*/" },
		{ "//", "\n" }
};
enum	{
	NCOMS	= (sizeof(COMMS)/sizeof(COMMS[0])),
};

/* Keywords on a line with only punctuations are uninteresting */
static	SYMPAIR	BORING[]	= {
	{"else", 0 },
	{"break", 0 },
	{"do", 0 },
	{"enum", 0 },
	{"struct", 0 },
	{"union", 0 },
};
enum	{
	NBORING	= (sizeof(BORING)/sizeof(BORING[0])),
};

//	If "pat" matches the start of "str"
//	return the length of pat.
//	eg match ("abc", "abcdef") == 3

static	int	match (char* pat, char* str) {
	int	result	= 0;
	size_t	len	= strlen (pat);
	if (strncmp (pat, str, len)==0) {
		result	= len;
	}
	return	result;
}

// Lookup "str" in "table" for a matching opening string
// if found return match length and set *closep to the closing string.
// eg
//   lookup (COMMS, NCOMMS,"/* XXXX */\n", &cclose) == 2 and cclose == "*/" 

static	int	lookup (SYMPAIR* table, int nelt, char* str, char** closep) {
	int	result	= 0;
	int	i	= 0;
	int	j	= nelt;
	while (i!=j) {
		result	= match (table[i].open, str);
		if (result > 0) {
			if (closep)
				*closep	= table[i].close;
			j	= i;
		}
		else	++i;
	}
	return	result;
}
static	int	cm_lookup (char* str, char** closep) {
	return	lookup (COMMS, NCOMS, str, closep);
}
static	int	qt_lookup (char* str, char** closep) {
	return	lookup (QUOTES, NQUOTES, str, closep);
}
static	int	bo_lookup (char* str) {
	return	lookup (BORING, NBORING, str, 0);
}
/* ---------------------------------------------------- */

static	void	process (FILE* input, FILE* output, char* name);

static	void	Usage (char* prog) {
	char*	t	= strchr (prog, '/');
	if (t)
		prog	= t+1;
	printf ("Usage: %s files...\n", prog);
	printf ("       list C files with intelligent line numbers.\n");
	exit (EXIT_FAILURE);
}
main (int argc, char* argv[]) {

	FILE*	input	= stdin;
	FILE*	output	= stdout;
	int	i	= 0;
	// Don't bother with getopt(3) just for -h/--help
	if (argc == 2 &&
		(strcmp (argv[1], "-h")==0 || strcmp (argv[1], "--help")==0))
			Usage (argv[0]);

	if (argc > 1) for (i=1; i < argc; ++i) {
		input	= fopen (argv[i], "r");
		if (input) {
			process (input, output, argv[i]);
			fclose (input);
		}
	}
	else	process (input, output, "");
	return	0;
}

static	void	process (FILE* input, FILE* output, char* name) {

	char	line [BUFSIZ];
	int	in_quote	= FALSE;
	int	in_comment	= FALSE;
	char*	cclose		= 0; // comment closing string
	char*	qclose		= 0; // quote closing string
	int	line_number	= 0;

	if (name[0] != '\0')
		fprintf (output, "%8.8s[%s]\n", "", name);

	while (fgets (line, sizeof(line), input) != 0) {
		int	no_num	= TRUE;	// line isn't interesting
		int	i	= 0;
		++line_number;
		
		while (line[i] != '\0') {
			int	advance	= 1;
			int	len	= 0;

			if (in_quote) {
				len	= match (qclose, &line [i]);
				if (len > 0) {
					if (!is_escaped (line, i)) {
						in_quote	= FALSE;
						advance	= len;
					}
				}
			}	
			// !in_quote
			else if (in_comment) {
				len	= match (cclose, &line [i]);
				if (len > 0) {
					in_comment	= FALSE;
					advance	= len;
				}
			}
			// !in_quote && !in_comment
			else if ((len = qt_lookup (&line[i], &qclose))>0) { 
				in_quote	= TRUE;
				advance	= len;
			}
			else if ((len = cm_lookup (&line[i], &cclose))>0) { 
				in_comment	= TRUE;
				advance	= len;
			}
			else if ((len = bo_lookup (&line[i]))>0) {
				advance	= len;
			}
			else if (is_interesting (line[i])) {
				no_num	= FALSE;
			}
			i	+= advance;
		}
		if (no_num) {
			fprintf (output, "%8.8s%s", "", line);
		}
		else	{
			fprintf (output, "%4d    %s", line_number, line);
		}
	}
}
