//
// Created by boris on 25/12/22.
//


#ifndef LAB33_PROXY_CACHE_H
#define LAB33_PROXY_CACHE_H


#include <bits/types/time_t.h>
#include <stdlib.h>
#include <pthread.h>

struct HashTableCell{
    char* key;

    char* response_string;
    size_t response_size;

    time_t last_used;
    struct HashTableCell* next;
};
struct HashTable{
    struct HashTableCell* tableArray;

    size_t hashtable_capacity;
    size_t hashtable_length;

    pthread_rwlock_t rwlock;
};

struct HashTable* cacheInit(size_t cache_size);

unsigned long hashFunc(const char* key);

struct HashTableCell* getCachedResponse(char* key, struct HashTable* hashTable);
int addToCache(char* key, char* value, size_t size, struct HashTable* hashTable); // key and value are copied

#endif //LAB33_PROXY_CACHE_H

