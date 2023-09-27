#define _BSD_SOURCE
#include <stdlib.h>

typedef int (*cmpfun)(const void *, const void *);

static int wrapper_cmp(const void *v1, const void *v2, int (*cmp)(const void *, const void *))
{
	return ((cmpfun)cmp)(v1, v2);
}

void qsort(void *base, size_t nel, size_t width, int (*cmp)(const void *, const void *))
{
	__qsort_r(base, nel, width, wrapper_cmp, cmp);
}
