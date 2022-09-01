#include <stdlib.h>
#include <string.h>

#define likely 

#include "mempool.h"

mempool_t *mempool_create(int min_nr, mempool_alloc_t *alloc_fn,
			mempool_free_t *free_fn, void *pool_data)
{
	return mempool_create_node(min_nr,alloc_fn,free_fn, pool_data,
				   0, 0);
}

mempool_t *mempool_create_node(int min_nr, mempool_alloc_t *alloc_fn,
			mempool_free_t *free_fn, void *pool_data,
			gfp_t gfp_mask, int nid)
{
    mempool_t *pool;
	pool = malloc(sizeof(*pool));

    pool->min_nr = min_nr;
	pool->pool_data = pool_data;
	pool->alloc = alloc_fn;
	pool->free = free_fn;
	
	return pool;
}

int mempool_resize(mempool_t *pool, int new_min_nr, gfp_t gfp_mask)
{
    return 0;
}

void mempool_destroy(mempool_t *pool)
{
    free(pool);
}

void *mempool_alloc(mempool_t *pool, gfp_t gfp_mask)
{
    void *element;
	element = pool->alloc(0, pool->pool_data);
	if (likely(element != NULL))
		return element;
    
    return NULL;
}

void mempool_free(void *element, mempool_t *pool)
{
    pool->free(element, pool->pool_data);
}
