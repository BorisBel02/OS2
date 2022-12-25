//
// Created by boris on 19/12/22.
//

#include <string.h>
#include <time.h>
#include <stdio.h>
#include "Cache.h"

unsigned long hashFunc(const char* key){
    unsigned long h = 1;
    unsigned const char* str;

    str = (unsigned const char*) key;

    while(*str != '\0'){
        h *= 37 + *str;
        ++str;
    }
    return h;
}
struct HashTable* cacheInit(size_t cache_size){
    struct HashTable* hashTable = (struct HashTable*) malloc(sizeof (struct HashTable));
    hashTable->hashtable_capacity = cache_size;
    hashTable->hashtable_length = 0;

    hashTable->tableArray = (struct HashTableCell*)malloc(sizeof (struct HashTableCell) * hashTable->hashtable_capacity);
    for (int i = 0; i < hashTable->hashtable_capacity; ++i) {
        hashTable->tableArray[i].key = NULL;
        hashTable->tableArray[i].next = NULL;
    }

    pthread_rwlock_init(&hashTable->rwlock, NULL);
    return hashTable;
}
struct HashTableCell* getCachedResponse(char* key, struct HashTable* hashTable){
    //printf("searching in cache\non key: %s", key);
    pthread_rwlock_rdlock(&hashTable->rwlock);
    unsigned long hash = hashFunc(key) % hashTable->hashtable_capacity;
    struct HashTableCell* entry = &hashTable->tableArray[hash];

    while(entry){
        if(entry->key == NULL){
            pthread_rwlock_unlock(&hashTable->rwlock);
            return NULL;
        }
        if(strcmp(key, entry->key) == 0){
            printf("key matches\n");
            entry->last_used = time(NULL);
            pthread_rwlock_unlock(&hashTable->rwlock);
            return entry;
        }
        else{
            entry = entry->next;
            printf("next entry\n");
        }
    }
    pthread_rwlock_unlock(&hashTable->rwlock);
    return NULL;
}
int addToCache(char* key, char* value, size_t size, struct HashTable* hashTable){
    if(key == NULL){
        return 1;
    }
    unsigned long hash = hashFunc(key) % hashTable->hashtable_capacity;
    printf("hash for key %s = %ld", value, hash);
    pthread_rwlock_wrlock(&hashTable->rwlock);
    if(hashTable->tableArray[hash].key == NULL){
        hashTable->tableArray[hash].key = (char*) malloc((strlen(key) + 1) * sizeof (char));
        strcpy(hashTable->tableArray[hash].key, key);
        hashTable->tableArray[hash].key[strlen(key) + 1] = '\0';

        hashTable->tableArray[hash].response_string = (char*) malloc(size * sizeof(char));
        memcpy(hashTable->tableArray[hash].response_string, value, size * sizeof(char));
        hashTable->tableArray[hash].response_string[size] = '\0';

        hashTable->tableArray[hash].response_size = size;
    }
    else{
        struct HashTableCell* newEntry = malloc(sizeof (struct HashTableCell));

        newEntry->key = (char*) malloc((strlen(key) + 1) * sizeof (char));
        strcpy(newEntry->key, key);
        newEntry->key[strlen(key) + 1] = '\0';

        newEntry->response_string = (char*) malloc((strlen(value) + 1) * sizeof(char));
        strcpy(newEntry->response_string, value);
        hashTable->tableArray[hash].response_string[strlen(value) + 1] = '\0';

        struct HashTableCell* cell = &hashTable->tableArray[hash];
        while(cell->next != NULL){
            cell = cell->next;
        }
        cell->next = newEntry;
        newEntry->next = NULL;
    }
    ++hashTable->hashtable_length;
    pthread_rwlock_unlock(&hashTable->rwlock);
    return 0;
}