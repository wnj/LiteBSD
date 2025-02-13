/*
 * Copyright (c) 1980, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Make all the .h files for the optional entries
 */
#include <ctype.h>
#include "config.h"
#include "y.tab.h"

char *toheader(char *dev);
char *tomacro(register char *dev);
void do_count(register char *dev, char *hname, int search);

void do_header(char *dev, char *hname, int count)
{
	char *file, *name, *inw;
	struct file_list *fl, *fl_head, *tflp;
	FILE *inf, *outf;
	int inc, oldcount;

	file = toheader(hname);
	name = tomacro(dev);
	inf = fopen(file, "r");
	oldcount = -1;
	if (inf == 0) {
		outf = fopen(file, "w");
		if (outf == 0) {
			perror(file);
			exit(1);
		}
		fprintf(outf, "#define %s %d\n", name, count);
		(void) fclose(outf);
		return;
	}
	fl_head = NULL;
	for (;;) {
		char *cp;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		if ((inw = get_word(inf)) == 0 || inw == (char *)EOF)
			break;
		inw = ns(inw);
		cp = get_word(inf);
		if (cp == 0 || cp == (char *)EOF)
			break;
		inc = atoi(cp);
		if (eq(inw, name)) {
			oldcount = inc;
			inc = count;
		}
		cp = get_word(inf);
		if (cp == (char *)EOF)
			break;
		fl = (struct file_list *) malloc(sizeof *fl);
		bzero(fl, sizeof(*fl));
		fl->f_fn = inw;
		fl->f_type = inc;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	(void) fclose(inf);
	if (count == oldcount) {
		for (fl = fl_head; fl != NULL; fl = tflp) {
			tflp = fl->f_next;
			free(fl);
		}
		return;
	}
	if (oldcount == -1) {
		fl = (struct file_list *) malloc(sizeof *fl);
		bzero(fl, sizeof(*fl));
		fl->f_fn = name;
		fl->f_type = count;
		fl->f_next = fl_head;
		fl_head = fl;
	}
	outf = fopen(file, "w");
	if (outf == 0) {
		perror(file);
		exit(1);
	}
	for (fl = fl_head; fl != NULL; fl = tflp) {
		fprintf(outf,
		    "#define %s %u\n", fl->f_fn, count ? fl->f_type : 0);
		tflp = fl->f_next;
		free(fl);
	}
	(void) fclose(outf);
}

/*
 * count all the devices of a certain type and recurse to count
 * whatever the device is connected to
 */
void do_count(register char *dev, char *hname, int search)
{
	register struct device *dp, *mp;
	register int count, hicount;

	/*
	 * After this loop, "count" will be the actual number of units,
	 * and "hicount" will be the highest unit declared.  do_header()
	 * must use this higher of these values.
	 */
	for (hicount = count = 0, dp = dtab; dp != 0; dp = dp->d_next)
		if (dp->d_unit != -1 && eq(dp->d_name, dev)) {
			if (dp->d_type == PSEUDO_DEVICE) {
				count =
				    dp->d_slave != UNKNOWN ? dp->d_slave : 1;
				break;
			}
			count++;
			/*
			 * Allow holes in unit numbering,
			 * assumption is unit numbering starts
			 * at zero.
			 */
			if (dp->d_unit + 1 > hicount)
				hicount = dp->d_unit + 1;
			if (search) {
				mp = dp->d_conn;
				if (mp != 0 && mp != TO_NEXUS &&
				    mp->d_conn != 0 && mp->d_conn != TO_NEXUS) {
					do_count(mp->d_name, hname, 0);
					search = 0;
				}
			}
		}
	do_header(dev, hname, count > hicount ? count : hicount);
}

void headers(void)
{
	register struct file_list *fl;

	for (fl = ftab; fl != 0; fl = fl->f_next)
		if (fl->f_needs != 0)
			do_count(fl->f_needs, fl->f_needs, 1);
}

/*
 * convert a dev name to a .h file name
 */
char *
toheader(char *dev)
{
	static char hbuf[80];

	(void) strcpy(hbuf, path(dev));
	(void) strcat(hbuf, ".h");
	return (hbuf);
}

/*
 * convert a dev name to a macro name
 */
char *tomacro(register char *dev)
{
	static char mbuf[20];
	register char *cp;

	cp = mbuf;
	*cp++ = 'N';
	while (*dev)
		*cp++ = islower(*dev) ? toupper(*dev++) : *dev++;
	*cp++ = 0;
	return (mbuf);
}
