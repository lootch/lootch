//	@(#) pdate.c - exploit strftime() to print various date formats
//	@(#) $Id:$

#include <u.h>
#include <libc.h>

size_t strftime(char *, size_t maxsize, const char *, const struct tm *);

static int nonl = 0, gmt = 0, secs = 0;
static char *fmt = nil, o[128];
static struct tm *tm;
static long now;

void
main (int argc, char **argv) {
	ARGBEGIN {
	case 'n':
		nonl = 1;
		break;
	case 's':
		secs = 1;
		break;
	case 'u':
		gmt = 1;
		break;
	default:
		fprint(2, "usage: %s [-uns] [+format] [seconds]\n", argv0);
		exits("usage");
	} ARGEND
	if (*argv && argv[0][0] == '+') {
		fmt = argv[0] + 1;
		--argc;
	}
	now = time(0);
	if (argc > 0)
		now = strtoul(argv[argc], 0, 0);
	if (gmt)
		tm = (struct tm*)gmtime(now);
	else
		tm = (struct tm*)localtime(now);
	if (fmt == nil)
		if (secs) {
			print("%ld", now);
			if (nonl == 0)
				print("\n");
		} else
			print("%s", asctime((Tm*) tm));
	else {
		if (strftime(o, sizeof o, fmt, tm) <= 0)
			exits("error");
		print("%s", o);
		if (nonl == 0)
			print("\n");
	}
	exits(nil);
}
