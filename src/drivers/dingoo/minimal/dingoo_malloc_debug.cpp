#include <stdlib.h>
#include <stdio.h>

void * debug_malloc(unsigned int size, const char *func)
{
	void *ptr=(void *)0;

	ptr=malloc(size);

	if (ptr)
	{
		printf ("%s malloc %d at %d\n", func, size, ptr);
		return (ptr);
	}
	else
	{
		printf ("%s malloc %d failed\n", func, size);
		return((void *)0);
	}
}

void debug_free(void * ptr, const char *func)
{
	printf ("%s free at %d\n", func, ptr);
	if (ptr)
		free(ptr);
	else
		printf("Cannot free 0 pointer\n");
}
