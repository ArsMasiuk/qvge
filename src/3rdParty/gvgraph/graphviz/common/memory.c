/* $Id$ $Revision$ */
/* vim:set shiftwidth=4 ts=8: */

/*************************************************************************
 * Copyright (c) 2011 AT&T Intellectual Property 
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * which accompanies this distribution, and is available at
 * http://www.eclipse.org/legal/epl-v10.html
 *
 * Contributors: See CVS logs. Details at http://www.graphviz.org/
 *************************************************************************/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

void *zmalloc(size_t nbytes)
{
    if (nbytes == 0)
	return 0;
    return gcalloc(1, nbytes);
}

void *zrealloc(void *ptr, size_t size, size_t elt, size_t osize)
{
    void *p = realloc(ptr, size * elt);
    if (p == NULL && size) {
	fprintf(stderr, "out of memory\n");
	exit(EXIT_FAILURE);
    }
    if (osize < size)
	memset((char *) p + (osize * elt), '\0', (size - osize) * elt);
    return p;
}

void *gcalloc(size_t nmemb, size_t size)
{
    char *rv = calloc(nmemb, size);
    if (rv == NULL) {
	fprintf(stderr, "out of memory\n");
	exit(EXIT_FAILURE);
    }
    return rv;
}

void *gmalloc(size_t nbytes)
{
    char *rv;
    if (nbytes == 0)
	return NULL;
    rv = malloc(nbytes);
    if (rv == NULL) {
	fprintf(stderr, "out of memory\n");
	exit(EXIT_FAILURE);
    }
    return rv;
}

void *grealloc(void *ptr, size_t size)
{
    void *p = realloc(ptr, size);
    if (p == NULL && size) {
	fprintf(stderr, "out of memory\n");
	exit(EXIT_FAILURE);
    }
    return p;
}
