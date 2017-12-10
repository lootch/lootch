/*
**      @(#) trim.c - space and tab manipulator
**      @(#) $Id: trim.c,v 1.6 2016/02/21 04:04:12 lucio Exp $
*/
/*
**      Removes all trailing spaces unconditionally;
**      translates tabs to equivalent spaces or reduces spaces to
**      the appropriate combination of tabs and spaces.
**      A tab is never output if its purpose can be fulfilled by
**      a single space (yes, it seems odd, but it has frustrated
**      me since 1991 - it's now fixed).
**
**      A couple of questions to be resolved in the new version:
**      (1) How does one trigger the "default" tab settings? This was
**          an empty argument when we did not use getopt().
**      Ans. It looks like we may have to specify an empty option
**      argument, by whatever means ('' or the next '-'?).
**      (2) More exhaustive testing is required.
**
** ==================================================================
**
**      $Logfile:$
**      $RCSfile: trim.c,v $
**      $Revision: 1.6 $
**      $Date: 2016/02/21 04:04:12 $
**      $Author: lucio $
**
** ==================================================================
**
**      $Log: trim.c,v $
**      Revision 1.6  2016/02/21 04:04:12  lucio
**      A few tangential mods/conflicts resolved.
**
**      Revision 1.5  2014/10/25 10:43:22  lucio
**      Usage scrappiness cleaned up.
**
**      Revision 1.4  2013/02/25 09:44:12  lucio
**      -v option
**
**      Revision 1.3  2004/07/22 06:09:02  lucio
**      Recent updates
**
**      Revision 1.2  2004/07/07 12:26:57  lucio
**      Added a TAB revealing feature
**
**      Revision 1.1.1.1  2003/11/11 05:13:53  lucio
**      Sundry small utilities
**
**      Revision 1.4  2001/04/12 05:08:09  lucio
**      Brought in line with new "usage" conventions.
**
**      Revision 1.3  2001/02/28 11:21:07  lucio
**      .cvsignore
**        added a few intermediate products
**      Makefile
**        facility to build HTML docs from MAN pages
**      dirseek.1
**        updated to latest version
**      dirseek.c
**        Missing 'H' in permissible options string
**        Allow '-' to represent std{in,out}
**      trim.c
**        A second stab at bringing it up to speed
**
**      Revision 1.2  2001/02/15 11:50:09  lucio
**      Preliminary update - more in line with modern practice.
**      Still some work required.
**
**      Revision 1.1  2001/02/15 11:39:54  lucio
**      New archivery.
**
** ==================================================================
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#if defined(__NetBSD__) || defined(MSDOS) || defined(_POSIX_SOURCE)
#include <unistd.h>
#if defined(_BSD_EXTENSION)
#include <bsd.h>
#endif
#else
#include <getopt.h>
#endif
#if defined(MSDOS)
extern char *optarg;
extern int optind;
#endif

#define     TAB     '\t'
#define     NL      '\n'
#define     SP      ' '
#define     SQ      '\''
#define     DQ      '"'

#define     FALSE   0
#define     TRUE    1

#define     TABLIM  12

static  char *ident = "@(#) $Id: trim.c,v 1.6 2016/02/21 04:04:12 lucio Exp $";
static  char *copyright = "Copyright (C) 1988-1991 Lucio de Re";
static  char *usage[] = {
    "usage: %s\t[-v|V]\nusage: \t\t[-h|H] [-v|V] [-t|T <arg>] [-R <ch>] [-|infile] [outfile]\n",
    "\n",
    "opts: h/H     - this message\n",
	"      R <ch>  - use <ch> as last char in tab expansions\n",
    "      t <arg> - compress all spaces to tabs\n",
    "      T <arg> - expand all tabs to spaces\n",
	"      v/V     - show release\n",
    "\n",
    "       <arg>: none - default to 8,16,24 ...\n",
    "              c - 'C' tabs (4,8,12 ...)\n",
    "              C - COBOL tabs (8,12,16 ...)\n",
    "              <n1>,<n2>,<n3> ...\n",
    "\n",
    "opts override TRIMTABS in environment:\n",
    "         TRIMTABS=[-]<arg>\n",
    "     where <arg> is as above\n",
    "     and leading '-' to compress to tabs,\n",
    "     (default is to expand to spaces)\n",
    "\n",
    "'-' may be used to force read from 'stdin'\n",
    "    when 'outfile' is specified.\n",
    NULL
};

static void
notice (void)
{
	printf ("@(#) trim: tab manipulator\n");
	printf ("@(#) $Id: trim.c,v 1.6 2016/02/21 04:04:12 lucio Exp $\n");
	printf ("@(#) Copyright (C) 1988-2016 Lucio de Re\n");
	printf ("@(#) Author (A) Lucio De Re, 2016.\n");
}

static void
use (char *argv0, char **usage) {
    fprintf (stderr, *usage, argv0);
	return;
}

static void
help (char *argv0, char **usage) {
    fprintf (stderr, *usage++, argv0);
    while (*usage) {
        fputs (*usage++, stderr);
    }
	return;
}

static void
version (char *argv0, char **v) {
    fprintf (stderr, "%s: %s\n", argv0, ident);
	return;
}

#if !defined(NetBSD) && !defined(MSDOS)
static char *
basename (char *name) {
    char *c = strrchr (name, '/');

    if (c) {
            return (c + 1);
    } else {
            return (name);
    }
}
#endif

static  int tablim = 2;
static  int tabs[TABLIM + 2];

extern char *getenv ();

static void
set (char *str) {
    switch (*str) {
        case 'c':
            tabs[1] = 4;
            tablim = 2;
            break;
        case 'C':
            tabs[1] = 7;
            tabs[2] = 11;
            tablim = 3;
            break;
        case '\0':
            tabs[1] = 8;
            tablim = 2;
            break;
        default:
            if (isdigit (*str)) {
                char ch;

                tabs[tablim = 1] = 0;
                while ((ch = *str) != '\0') {
                    if (isdigit (ch)) {
                        tabs[tablim] *= 10;
                        tabs[tablim] += ch - '0';
                        str++;
                    } else if (ch == ',') {
                        if (tabs[tablim] > tabs[tablim-1]) {
                            tabs[++tablim] = 0;
                            str++;
                        } else {
                            --tablim;
                            while (*(++str))		/* flush remainder */
                                ;
                        }
                    } else
                        while (*(++str))
                            ;
                }
                if (tabs[tablim] > tabs[tablim-1])
                    tablim++;
            }
            break;
    }
    tabs[tablim] = tabs[tablim - 1] - tabs[tablim - 2];
	return;
}

static int
next (int tabdex, int index) {
    int diff;

    if (index < tabs[tabdex])
        tabdex = 1;
    while (tabdex < tablim && index >= tabs[tabdex])
        tabdex++;
    if (tabdex >= tablim) {
        diff = tabs[tabdex - 1] - tabs[tabdex - 2];
        tabs[tabdex] = tabs[tabdex - 1] + diff;
        while (index >= tabs[tabdex])
            tabs[tabdex] += diff;
    }
    return (tabdex);
}

int
main (int argc, char *argv[]) {
    FILE *inf = stdin,
         *outf = stdout;
    char *argv0 = basename (argv[0]);
    int ch,
        sp,
        index;
    int tins = 0;
    int old_tins,
        quote = '\0',
        escseq = FALSE;
    int tabdex = 1,
        htdex = 1;
    char *opts;
	int tch = 0, exp = 0;

    tabs[0] = 0;
    tabs[1] = 8;
    if ((opts = getenv ("TRIMTABS")) != 0) {
        if (*opts == '-') {
            tins = 1;
            opts++;
        }
        set (opts);
    }

    while ((ch = getopt (argc, argv, "t:T:R:hHvV")) != -1) {
        switch (ch) {
            case 't':           /* tab stops to be inserted */
				if (tch) {
					fprintf (stderr, "%s: options conflict (t/R)\n", argv0);
					exit (1);
				}
                tins = 1;
            case 'T':   		/* tab stops to be expanded */
                set (optarg);
                break;
			case 'R':
				if (tins) {
					fprintf (stderr, "%s: options conflict (t/R)\n", argv0);
					exit (1);
				}
				tch = *optarg;
				break;
            case 'h':
            case 'H':
                help (argv0, usage);
                exit (0);
			case 'v':
                version (argv0, usage);
                exit (0);
            case 'V':
				notice ();
				exit (0);
            default:
                use (argv0, usage);
                exit (2);
        }
    }
    argc -= optind;
    argv += optind;
    if (argc) {
        if (*argv[0] != '-') {
            if ((inf = fopen (*argv, "r")) == NULL) {
                 fprintf (stderr, "%s: cannot open %s for input\n", argv0, *argv);
                 exit (1);
            }
        }
        --argc;
        ++argv;
    }
    if (argc) {
        if (*argv[0] != '-') {
            if ((outf = fopen (*argv, "w")) == NULL) {
                 fprintf (stderr, "%s: cannot open %s for output\n", argv0, *argv);
                 exit (1);
            }
        }
    }
    sp = index = 0;
    old_tins = tins;
    while ((ch = fgetc (inf)) != EOF) {
        switch (ch) {
            case NL:
                quote = '\0';
                tins = old_tins;
                fputc (ch, outf);
                sp = index = 0;
                escseq = FALSE;
                break;
            case SP:
                sp++;
                index++;
                escseq = FALSE;
                break;
            case TAB:
                if (tablim > 0) {
                    int x = tabs [tabdex = next (tabdex, index)];

                    sp += x - index;
					++exp;
                    index = x;
                    escseq = FALSE;
                }
                break;
            default:
				{
                    int x = index - sp;
                    int c;

                    while ((c = tabs[htdex = next (htdex, x)]) <= index) {
                        if (c == x + 1) {
                            fputc (SP, outf);
                        } else {
                			if (tins) {
                            	fputc (TAB, outf);
							} else {
								while (++x < c)
									fputc (SP, outf);
								if (tch && exp) {
									fputc (tch, outf);
									--exp;
								} else {
									fputc (SP, outf);
								}
							}
                        }
                  		x = c;
                    }
                    while (x++ < index)
                        fputc (SP, outf);
                    sp = 0;
                }
                fputc (ch, outf);
                index++;
                if (escseq)
                    escseq = FALSE;
                else
                    switch (ch) {
                        case '\\':
                            escseq = TRUE;
                            break;
                        case SQ:
                        case DQ:
                            if (quote == ch) {
                                quote = '\0';
                                tins = old_tins;
                            } else if (quote == '\0') {
                                quote = ch;
                                old_tins = tins;
                                tins = 0;
                            }
                        default:
                            escseq = FALSE;
                            break;
                    }
                break;
        }
    }
    fclose (inf);
    fclose (outf);
    return 0;
}
