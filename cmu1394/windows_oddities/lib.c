/*
 * lib.c
 *
 * simulate missing lib.exe by calling link.exe /lib
 *
 * see http://www.blender3d.org/cms/Microsoft_VC___Toolkit_2003.276.0.html
 *
 * copied from http://sapdb.2scale.net/moin.cgi/MS_20C_2b_2b_20Toolkit
 *
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static int
findInPath (
    char * fullpath,
    const char * executable)
{
    const char * pathvar = getenv ("PATH");
    const char * dirstart = pathvar;
    const char * dirend;
    int          dirlen;
    FILE       * dummy;

    if (pathvar == NULL) {
        return 0;
    }
    while (dirstart [0] != '\0') {
        dirend = strchr (dirstart, ';');
        if (dirend == NULL) {
            dirlen = strlen (dirstart);
        }
        else {
            dirlen = dirend - dirstart;
        }
        sprintf (fullpath, "%.*s\\%s", dirlen, dirstart, executable);
        dummy = fopen (fullpath, "rb");
        if (dummy != NULL) {
            fclose (dummy);
            return 1;
        }
        if (dirend == NULL) {
            return 0;
        }
        dirstart = dirend + 1;
    }
    return 0;
}

int main (
    int argc,
    const char ** argv)
{
    char fullpath [255];
    char ** newargs;
    if (!findInPath (fullpath, "link.exe")) {
        fprintf (stderr, "lib.exe: link.exe not found in %PATH%\n");
        exit (2);
    }
    newargs = calloc (argc + 2, sizeof (char*));
    if (newargs == NULL) {
        fprintf (stderr, "lib.exe: allocation of arguments failed\n");
        exit (2);
    }
    memcpy (newargs + 2, argv + 1, (argc - 1) * sizeof (char *));
    newargs [0] = "link.exe";
    newargs [1] = "/lib";
    execv (fullpath, newargs);
}
