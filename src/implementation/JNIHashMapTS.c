#include <jni.h> /** This is part of JDK installation */
#include <bharati_binita_bridge_javaToC_jni_NativeHashMap.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h> 
#include <time.h>
#include "../headers/common_headers.h"
#include "../headers/native_headers.h"
#include <pthread.h>

/**
 * Warning: Untested code!!
 * Thread safe version of JNIHashMap.
 */


#define TABLE_SIZE 100000000
#define MAX_NAME 100

map_entry_non_native *hash_table[TABLE_SIZE];
//bucket level locks, same as Java's ConcurrentHashMap
pthread_mutex_t bucket_locks[TABLE_SIZE];

void init_locks() {
    for (int i = 0; i < TABLE_SIZE; i++) {
        pthread_mutex_init(&bucket_locks[i], NULL);
    }
}

void init_hash_table(){
     for (int i = 0 ; i < TABLE_SIZE; i++){
        hash_table[i] = NULL;//if you dont set it, it will be set to garbage values. And you wont really know if its a valid pointer or not. 
    }
    init_locks();
}

map_entry_non_native* transform(int key, char name[MAX_NAME]) {
    map_entry_non_native *mapEntry = malloc(sizeof(map_entry_non_native));
    
    mapEntry->key = key;

    strncpy(mapEntry->name, name, MAX_NAME - 1);
    mapEntry->name[MAX_NAME - 1] = '\0';
    
    mapEntry->next = NULL;
    return mapEntry;
}

int hash(unsigned int key){
    return key % TABLE_SIZE;
}

void insert_to_hash_table(int key,  char name[MAX_NAME]) {
    if (name == NULL) {
        return;
    }

    map_entry_non_native *mapEntry = transform(key, name);
    //printf("malloc assigned %p for key = %d\n",mapEntry,key);

    int index = hash(mapEntry->key); 
    pthread_mutex_lock(&bucket_locks[index]);
    //printf("key = %d, index = %d\n",key, index);
    if (hash_table[index] == NULL) {
         hash_table[index] = mapEntry;   
         pthread_mutex_unlock(&bucket_locks[index]);
         return;
    }
    else {
        map_entry_non_native *headPtr = hash_table[index];
        if (headPtr->key  == mapEntry->key) {
            //replace the current node with input node.
            mapEntry->next = headPtr->next;
            hash_table[index] = mapEntry;
            //printf("free ptr %p for key = %d\n",headPtr,key);
            free(headPtr);
            headPtr = NULL;
            pthread_mutex_unlock(&bucket_locks[index]);
            return;
        }
        else {
            map_entry_non_native *prevPtr = headPtr;
            map_entry_non_native *nextPtr = hash_table[index]->next;
            while (nextPtr != NULL) {
                if (nextPtr->key == mapEntry->key) {
                    //replace the current node with input node.
                    mapEntry->next = nextPtr->next;
                    prevPtr->next = mapEntry;    
                    //printf("free ptr %p for key = %d\n",nextPtr,key);
                    free(nextPtr);
                    nextPtr = NULL;
                    pthread_mutex_unlock(&bucket_locks[index]);
                    return;
            }
            prevPtr = nextPtr;
            nextPtr = nextPtr->next;
        }
        //if control is here that means, we need to add a brand new node at the end of the linked list
        mapEntry->next = NULL;
        //printf("free ptr %p for key = %d\n",prevPtr->next,key);
        //free(prevPtr->next);
        prevPtr->next = mapEntry;
        pthread_mutex_unlock(&bucket_locks[index]);
        return;
        }
    }
}


void print_hash_table(){
    for (int i = 0 ; i < TABLE_SIZE; i++){
        printf("--------\n");
        printf("Printing bucket number = %u\n",i);

        if (hash_table[i] != NULL) {
            printf("\t\t\t%d %s\n", hash_table[i]->key, hash_table[i]->name);
            map_entry_non_native *nextPtr = hash_table[i]->next;
            while (nextPtr != NULL) {
                printf("\t\t\t%d %s\n", nextPtr->key, nextPtr->name);
                nextPtr = nextPtr->next;
            }
        }
    }
}


map_entry_non_native *hash_table_look_up(unsigned int key) {
    map_entry_non_native *hashmap_value;
    unsigned int index = hash(key);
    pthread_mutex_lock(&bucket_locks[index]);
   
    hashmap_value = hash_table[index];
    if (hashmap_value != NULL) {
       if (hashmap_value->key == key) {
        pthread_mutex_unlock(&bucket_locks[index]);
        return hashmap_value;
       }
       else {
        map_entry_non_native *headPtr = hashmap_value;
        map_entry_non_native *nextPtr = hashmap_value->next;
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

bool delete_key_fromhashtable(unsigned int key) {
    unsigned int index = hash(key);
    pthread_mutex_lock(&bucket_locks[index]);
    map_entry_non_native *headPtr = hash_table[index];
    if (headPtr->key == key) {        
        hash_table[index] = headPtr->next;
        free(headPtr);
        pthread_mutex_unlock(&bucket_locks[index]);
        return true;
    }
    else {
        map_entry_non_native *prevPtr = headPtr;
        map_entry_non_native *nextPtr = prevPtr->next;
        while(nextPtr != NULL){
            if (nextPtr->key == key) {  
                prevPtr->next=nextPtr->next;      
                free(nextPtr);
                pthread_mutex_unlock(&bucket_locks[index]);
                return true;
            }
            prevPtr = nextPtr;
            nextPtr = nextPtr->next;
        }
    }
    pthread_mutex_unlock(&bucket_locks[index]);
    return false;
}

JNIEXPORT void JNICALL Java_bharati_binita_bridge_javaToC_jni_NativeHashMap_init_1hash_1table(JNIEnv *env, jobject obj) {
   init_hash_table();
}


JNIEXPORT void JNICALL Java_bharati_binita_bridge_javaToC_jni_NativeHashMap_insert_1to_1hash_1table
  (JNIEnv *env, jobject obj, jint key, jstring name) {

    if (name == NULL) {
        return;
    }
    // Convert jstring to C string
    const char *name_cstr = (*env)->GetStringUTFChars(env, name, NULL);
    if (name_cstr == NULL) {
        return; // Out of memory
    }

    // Copy safely to fixed-length buffer
    char name_buf[MAX_NAME];
    strncpy(name_buf, name_cstr, MAX_NAME - 1);
    name_buf[MAX_NAME - 1] = '\0';  // Ensure null-termination

   // Call your original C function
    insert_to_hash_table((int)key, name_buf);
    //if you dont do below, java heap memory will leak.
    (*env)->ReleaseStringUTFChars(env, name, name_cstr);
}

JNIEXPORT jstring JNICALL Java_bharati_binita_bridge_javaToC_jni_NativeHashMap_hash_1table_1look_1up
  (JNIEnv *env, jobject obj, jint key) {
    map_entry_non_native *entry = hash_table_look_up((int)key);
    if (entry == NULL) return NULL;
    return (*env)->NewStringUTF(env, entry->name);
}

JNIEXPORT jboolean JNICALL Java_bharati_binita_bridge_javaToC_jni_NativeHashMap_delete_1key_1fromhashtable
  (JNIEnv *env, jobject obj, jint key){
    return delete_key_fromhashtable((int)key) ? JNI_TRUE : JNI_FALSE;
}



