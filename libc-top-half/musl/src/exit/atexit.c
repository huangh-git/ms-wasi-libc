#include <stdlib.h>
#include <stdint.h>
#include "libc.h"
#include "lock.h"
#include "fork_impl.h"

#define malloc __libc_malloc
#define calloc __libc_calloc
#define realloc undef
#define free undef

/* Ensure that at least 32 atexit handlers can be registered without malloc */
#define COUNT 32

static struct fl
{
	struct fl *next;
	void (*f[COUNT])(void (*)(void)); // the param is function pointer
	void (*a[COUNT])(void);
} builtin, *head;

static int slot;

#if defined(__wasilibc_unmodified_upstream) || defined(_REENTRANT)
static volatile int lock[1];
volatile int *const __atexit_lockptr = lock;
#endif

void __funcs_on_exit()
{
	void (*func)(void (*)(void));
	void (*arg)(void);
	LOCK(lock);
	for (; head; head=head->next, slot=COUNT) while(slot-->0) {
		func = head->f[slot];
		arg = head->a[slot];
		UNLOCK(lock);
		func(arg);
		LOCK(lock);
	}
}

void __cxa_finalize(void *dso)
{
}

int __cxa_atexit(void (*func)(void (*)(void)), void (*arg)(void), void *dso)
{
	LOCK(lock);

	/* Defer initialization of head so it can be in BSS */
	if (!head) head = &builtin;

	/* If the current function list is full, add a new one */
	if (slot==COUNT) {
		struct fl *new_fl = calloc(sizeof(struct fl), 1);
		if (!new_fl) {
			UNLOCK(lock);
			return -1;
		}
		new_fl->next = head;
		head = new_fl;
		slot = 0;
	}

	/* Append function to the list. */
	head->f[slot] = func;
	head->a[slot] = arg;
	slot++;

	UNLOCK(lock);
	return 0;
}

static void call(void (*p)(void))
{
	((void (*)(void))p)();
}

int atexit(void (*func)(void))
{
	return __cxa_atexit(call, func, 0);
}
