/* updaterevision.c
 *
 * Public domain. This program uses the svnversion command to get the
 * repository revision for a particular directory and writes it into
 * a header file so that it can be used as a project's build number.
 */

#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
// [BB] New #includes.
#include <sys/stat.h>

int main(int argc, char **argv)
{
	char *name;
	char currev[64], lastrev[64], run[256], *rev;
	unsigned long urev;
	FILE *stream = NULL;
	int gotrev = 0, needupdate = 1;
	// [BB] Are we working with a SVN checkout?
	int svnCheckout = 0;

	if (argc != 3)
	{
		fprintf (stderr, "Usage: %s <repository directory> <path to svnrevision.h>\n", argv[0]);
		return 1;
	}

	// [BB] Try to figure out whether this is a SVN or a Hg checkout.
	{
		struct stat st;
		char filename[1024];
		sprintf ( filename, "%s/.svn/entries", argv[1] );
		if ( stat ( filename, &st ) == 0 )
			svnCheckout = 1;
		// [BB] If stat failed we have to manually clear errno, otherwise the code below doesn't work.
		else
			errno = 0;
	}

	// Use svnversion to get the revision number. If that fails, pretend it's
	// revision 0. Note that this requires you have the command-line svn tools installed.
	// [BB] Depending on whether this is a SVN or Hg checkout we have to use the appropriate tool.
	if ( svnCheckout )
		sprintf (run, "svnversion -cn %s", argv[1]);
	else
		sprintf (run, "hg identify -n"); 
	if ((name = tempnam(NULL, "svnout")) != NULL)
	{
#ifdef __APPLE__
		// tempnam will return errno of 2 even though it is successful for our purposes.
		errno = 0;
#endif
		if((stream = freopen(name, "w+b", stdout)) != NULL &&
		   system(run) == 0 &&
#ifndef __FreeBSD__
		   errno == 0 &&
#endif
		   fseek(stream, 0, SEEK_SET) == 0 &&
		   fgets(currev, sizeof currev, stream) == currev &&
		   (isdigit(currev[0]) || (currev[0] == '-' && currev[1] == '1')))
		{
			gotrev = 1;
		}
	}
	if (stream != NULL)
	{
		fclose (stream);
		remove (name);
	}
	if (name != NULL)
	{
		free (name);
	}

	if (!gotrev)
	{
		fprintf (stderr, "Failed to get current revision: %s\n", strerror(errno));
		strcpy (currev, "0");
		rev = currev;
	}
	else
	{
		rev = strchr (currev, ':');
		if (rev == NULL)
		{
			rev = currev;
		}
		else
		{
			rev += 1;
		}
	}

	// [BB] Workaround to keep the old SVN numbering on Hg repos: Increase the revision number
	// we parsed by the offset between the Hg and the SVN repo version. Will be removed
	// once we decide to ditch SVN completely.
	if ( gotrev && ( svnCheckout == 0 ) )
	{
		unsigned long hgOffSet = 158;
		char *endptr;
		unsigned long parsedRev = strtoul(rev, &endptr, 10);
		unsigned int localChanges = ( *endptr == '+' );
		sprintf ( rev, "%d%s", parsedRev + hgOffSet, localChanges ? "M" : "" );
	}

	stream = fopen (argv[2], "r");
	if (stream != NULL)
	{
		if (!gotrev)
		{ // If we didn't get a revision but the file does exist, leave it alone.
			fprintf( stderr, "No revision found.\n" );
			fclose (stream);
			return 0;
		}
		// Read the revision that's in this file already. If it's the same as
		// what we've got, then we don't need to modify it and can avoid rebuilding
		// dependant files.
		if (fgets(lastrev, sizeof lastrev, stream) == lastrev)
		{
			if (lastrev[0] != '\0')
			{ // Strip trailing \n
				lastrev[strlen(lastrev) - 1] = '\0';
			}
			if (strcmp(rev, lastrev + 3) == 0)
			{
				needupdate = 0;
			}
		}
		fclose (stream);
	}

	if (needupdate)
	{
		stream = fopen (argv[2], "w");
		if (stream == NULL)
		{
			return 1;
		}
		urev = strtoul(rev, NULL, 10);
		fprintf (stream,
"// %s\n"
"//\n"
"// This file was automatically generated by the\n"
"// updaterevision tool. Do not edit by hand.\n"
"\n"
"#define SVN_REVISION_STRING \"%s\"\n"
"#define SVN_REVISION_NUMBER %lu\n",
			rev, rev, urev);
		fclose (stream);
		fprintf (stderr, "%s updated to revision %s.\n", argv[2], rev);
	}
	else
	{
		fprintf (stderr, "%s is up to date at revision %s.\n", argv[2], rev);
	}

	return 0;
}
