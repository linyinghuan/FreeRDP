#ifndef _HASH_H_
#define _HASH_H_

#include <stdint.h>

typedef struct _hash_node {
	uint32_t key;
	uint32_t used;
	void *value;
} hash_node;

typedef struct _hash_table {
	long long size;
	long long count;
	hash_node *bucket;
}hash_table;

hash_table* hash_table_init(unsigned long long size);
int hash_table_insert(hash_table *table, uint32_t key, void *value);
char* hash_table_search(hash_table *table, uint32_t key);
void hash_table_delete(hash_table *table, uint32_t key);
#endif
