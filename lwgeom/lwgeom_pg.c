#include "postgres.h"

void *palloc_fn(size_t size)
{
	void * result;
	result = palloc(size);
	//elog(NOTICE,"  palloc(%d) = %p", size, result);
	return result;
}

void free_fn(void *ptr)
{
	//elog(NOTICE,"  pfree(%p)", ptr);
	pfree(ptr);
}
