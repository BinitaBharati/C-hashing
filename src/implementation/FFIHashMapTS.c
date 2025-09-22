#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> 
#include <time.h>
#include "../headers/common_headers.h"
#include <pthread.h>

#define MAX_NAME 50
#define TABLE_SIZE 100000000

/**
 * Warning: Untested code!!
 * Hashmap implementation meant to be invoked from Java FFI. However, FFI library
 * is so generic that this file can be used normally within C code (how you would
 * do without FFI involved). This implementation is THREAD SAFE!!
 */

typedef struct map_entry {
    //key
    int key;

    char name[MAX_NAME];
    struct map_entry *next;
} map_entry;

map_entry *hash_table[TABLE_SIZE];
//bucket level locks, same as Java's ConcurrentHashMap
pthread_mutex_t bucket_locks[TABLE_SIZE];

void init_locks() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        pthread_mutex_init(&bucket_locks[i], NULL);
    }
}

void init_hash_table(){
     for (int i = 0 ; i < TABLE_SIZE; i++){
        hash_table[i] = NULL;//if you dont set it, it will be set to garbage values. And you wont really know if its a valkey pointer or not. 
    }
    init_locks();
}

void print_hash_table(){
    for (int i = 0 ; i < TABLE_SIZE; i++){
        printf("--------\n");
        printf("Printing bucket number = %u\n",i);

        if (hash_table[i] != NULL) {
            printf("\t\t\t%d %s\n", hash_table[i]->key, hash_table[i]->name);
            map_entry *nextPtr = hash_table[i]->next;
            while (nextPtr != NULL) {
                printf("\t\t\t%d %s\n", nextPtr->key, nextPtr->name);
                nextPtr = nextPtr->next;
            }
        }
    }
}

unsigned int hash(unsigned int key){
    return key % TABLE_SIZE;
}

map_entry* transform(int key, char* name){
    map_entry *mapEntry = malloc(sizeof(map_entry));
    //printf("malloc assigned %p for key = %d\n",mapEntry,key);

    mapEntry->key = key;
    strncpy(mapEntry->name, name, MAX_NAME);
    return mapEntry;
}

void insert_to_hash_table(int key, char *name) {
    if (name == NULL) {
        return;
    }

    map_entry* mapEntry = transform(key, name);
    //printf("got new malloc pointer ptr %p for key = %d\n",mapEntry,mapEntry->key);

    int index = hash(key); 
    pthread_mutex_lock(&bucket_locks[index]);
    if (hash_table[index] == NULL) {
         hash_table[index] = mapEntry;   
         pthread_mutex_unlock(&bucket_locks[index]);
         return;
    }

    map_entry *headPtr = hash_table[index];
    if (headPtr->key  == mapEntry->key) {
        //replace the current node with input node.
        mapEntry->next = headPtr->next;
        hash_table[index] = mapEntry;
        printf("free ptr %p for key = %d\n",headPtr,mapEntry->key);
        free(headPtr);
        pthread_mutex_unlock(&bucket_locks[index]);
        return;
    }
    else {
        map_entry *prevPtr = headPtr;
        map_entry *nextPtr = headPtr->next;
        while (nextPtr != NULL) {
            if (nextPtr->key == mapEntry->key) {
                //replace the current node with input node.
                mapEntry->next = nextPtr->next;
                prevPtr->next = mapEntry;  
                printf("free ptr %p for key = %d\n",nextPtr,mapEntry->key);      
                free(nextPtr);
                pthread_mutex_unlock(&bucket_locks[index]);
                return;
        }
        prevPtr = nextPtr;
        nextPtr = nextPtr->next;
    }
    //if control is here that means, we need to add a brand new node at the end of the linked list
    mapEntry->next = NULL;
    //printf("free ptr %p for key = %d\n",prevPtr->next,mapEntry->key);      
    //free(prevPtr->next);
    prevPtr->next = mapEntry;
    pthread_mutex_unlock(&bucket_locks[index]);
    return;
    }
    
}

map_entry *hash_table_look_up(unsigned int key) {
    map_entry *hashmap_value;
    unsigned int index = hash(key);
    pthread_mutex_lock(&bucket_locks[index]);
    
    hashmap_value = hash_table[index];
    if (hashmap_value != NULL) {
       if (hashmap_value->key == key) {
        pthread_mutex_unlock(&bucket_locks[index]);
        return hashmap_value;
       }
       else {
        map_entry *headPtr = hashmap_value;
        map_entry *nextPtr = hashmap_value->next;
        while (nextPtr != NULL) {
            if (nextPtr->key == key) {
                pthread_mutex_unlock(&bucket_locks[index]);
                return nextPtr;
            }
            nextPtr = nextPtr->next;
        }
       }
    }   
    pthread_mutex_unlock(&bucket_locks[index]);
    return NULL;
}

bool delete_key_from_hashtable(unsigned int key) {
    unsigned int index = hash(key);
    map_entry *headPtr = hash_table[index];
    if (headPtr->key == key) {        
        hash_table[index] = headPtr->next;
        free(headPtr);
        return true;
    }
    else {
        map_entry *prevPtr = headPtr;
        map_entry *nextPtr = prevPtr->next;
        while(nextPtr != NULL){
            if (nextPtr->key == key) {  
                prevPtr->next=nextPtr->next;      
                free(nextPtr);
                return true;
            }
            prevPtr = nextPtr;
            nextPtr = nextPtr->next;
        }
    }
    return false;
}

const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

void generate_random_string(char *str, unsigned int length) {
    unsigned int charset_size = sizeof(charset);  // exclude null terminator

    for (unsigned int i = 0; i < length-1; i++) {
        int key = rand() % charset_size;
        str[i] = charset[key];
    }

    str[length-1] = '\0';  // null-terminate the string
}

void lookup_random_keys(){
    int startKey = 2;
    for (int i = 0 ; i < 10 ; i++){
        printf("Looking up key = %d\n",startKey);
        map_entry *mapEntry = hash_table_look_up(startKey);
        if (mapEntry != NULL) {
            printf("look up key = %d, value = \n",startKey);
            printf("\t\t\t%s\n",mapEntry->name);
        }
        else {
            printf("\t\t\tNULL\n");
        }
       
        startKey = startKey + 2;

    }
}

int main(int argc, char *argv[]) {
    //printf("Argument count: %d\n", argc);
    if (argc < 2) {
        printf("Missing mandatory input argument\n");
        printf("\t\t\t 1. Total map entries\n");
        return 1;
    }
    
    init_hash_table();
    int TOTAL_ENTRIES = atoi(argv[1]);
    while (true) {
        long long start_millis = current_time_millis();
        for (int key = 1 ; key <= TOTAL_ENTRIES; key++) {
            char name[MAX_NAME];
            generate_random_string(name,MAX_NAME);

            insert_to_hash_table(key, name);
        }

        long long end_millis = current_time_millis();
         //try looking up some entries
        //lookup_random_keys();

        printf("Done with adding all entries in map. Time taken in ms = %lld\n",(end_millis-start_millis)); 
        printf("Going to sleep\n");
        sleep(1*60);
        printf("Woke up from sleep.\n");   
    }
    return 0;
}
