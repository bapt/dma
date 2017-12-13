/*-
 * Copyright (c) 2017 Baptiste Daroussin <bapt@FreeBSD.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in this position and unchanged.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR(S) BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <err.h>
#include <errno.h>
#include <ctype.h>

#include "dma.h"

static struct alias *
alias_lookup(const char *name)
{
	struct alias *al;

	LIST_FOREACH(al, &aliases, next) {
		if (strcmp(al->alias, name) == 0)
			return (al);
	}
	return (NULL);
}

static void
parse_alias_val(struct alias *al, char *val)
{
	struct stritem *it;

	/* ignore initial spaces */
	while (isspace(*val)) {
		val++;
	}

	while (isspace(val[strlen(val) - 1])) {
		val[strlen(val) -1] = '\0';
	}

	if (strncmp(":include:", val, 8) == 0) {
		/* XXX read_include_file(val + 8); */
		return;
	}

	if (strncmp("error:", val, 6) == 0) {
		warnx("Unsupported 'error:'");
		return;
	}

	if (strncmp("maildir:", val, 8) == 0) {
		warnx("Unsupported 'maildir:'");
		return;
	}

	it = calloc(1, sizeof(*it));
	if (it == NULL)
		err(1, "calloc()");

	it->str = strdup(val);
	if (it->str == NULL)
		err(1, "strdup()");

	SLIST_INSERT_HEAD(&al->dests, it, next);
}

int
read_aliases(const char *alias_file)
{
	char *walk, *token, *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;
	size_t lineno = 0;
	FILE *conf;
	struct alias *al;

	conf = fopen(alias_file, "r");
	if (conf == NULL) {
		if (errno == ENOENT)
			return (0);
		return (-1);
	}

	while ((linelen = getline(&line, &linecap, conf)) > 0) {
		lineno++;
		walk = line;
		/* eliminate comments */
		strsep(&walk, "#");
		if ((walk = strchr(line, ':')) == NULL) {
			/* empty line */
			if (line[strspn(line, " \t\n")] == '\0')
				continue;
			warnx("Ignoring malformed alias entry in %s, line %zu",
			    alias_file, lineno);
			continue;
		}
		*walk = '\0';
		walk++;

		/* remove end spaces */
		while (isspace(line[strlen(line) - 1])) {
			line[strlen(line) -1] = '\0';
		}

		if (alias_lookup(line) != NULL) {
			warnx("Ignoring duplicate entry for %s, line "
			    "%zu", line, lineno);
			continue;
		}

		/* empty alias? */
		if (walk[strspn(walk, " \t\n")] == '\0')
			continue;

		al = calloc(1, sizeof(*al));
		if (al == NULL)
			err(1, "calloc()");

		al->alias = strdup(line);
		if (al->alias == NULL)
			err(1, "calloc()");

		while ((token = strsep(&walk, ",")) != NULL) {
			parse_alias_val(al, token);
		}
		LIST_INSERT_HEAD(&aliases, al, next);
	}

	free(line);
	fclose(conf);
	return (0);
}
