#ifndef NATIVE_HEADERS
#define NATIVE_HEADERS

#define MAX_NAME 100

typedef struct map_entry_non_native {
    //key
    int key;

    /* Values. I have kept only element in the value tuple (name) as we can not return 
       more than one more than one value in return type of native function call between C and Java.
    */
    char name[MAX_NAME];
    struct map_entry_non_native *next;
} map_entry_non_native;


//map_entry_non_native* transform(int key, char name[MAX_NAME]);

#endif
