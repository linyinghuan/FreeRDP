#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "hash.h"

hash_table* hash_table_init(unsigned long long size)
{
	hash_table *table = malloc(sizeof(hash_table));

	if(!table)
		return NULL;

	table->bucket = malloc(sizeof(hash_node) * size);
	if(!table->bucket)
		return NULL;
	memset((char *)table->bucket, 0, sizeof(hash_node) * size);
	table->size = size;
	table->count = 0;
	return table;
}

int hash_table_insert(hash_table *table, uint32_t key, void *value)
{
	unsigned long index = key % table->size;
	hash_node *node = &table->bucket[index];

	while(node->used) {
		if(key == node->key){
			return 0;
		}

		if(node == &table->bucket[table->size - 1])
		{
			return -1;
		}
		node++;
	}

	node->key = key;
	node->value = malloc(strlen(value) + 1);
	memset(node->value, 0, strlen(value) + 1);
	strcpy(node->value, value);
	table->count++;
	node->used = 1;
	return 0;
}

hash_node* hash_table_search(hash_table *table, uint32_t key)
{
	unsigned long index = key % table->size;
	hash_node *node = &table->bucket[index];

	if(!node->used) {
		return NULL;
	}

	while(key != node->key) {
		if(node == &table->bucket[table->size - 1])
			return NULL;
		
		node++;
		if(!node->used)
			return NULL;
	}

	return node->value;		
}

void hash_table_delete(hash_table *table, uint32_t key)
{
	unsigned long index = key % table->size;
	hash_node *node = &table->bucket[index];

	if(!node->used) {
		return NULL;
	}

	while(key != node->key) {
		if(node == &table->bucket[table->size - 1])
			return;
		
		node++;
		if(!node->used)
			return;
	}

	free(node->value);
	node->used = 0;
	table->count--;		
}