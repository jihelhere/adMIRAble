#ifndef __HASHTABLE__
#define __HASHTABLE__

#include <stdint.h>

typedef union {
    double double_value;
    int int_value;
    void* pointer_value;
} value_t;

typedef struct {
    char* key;
    value_t value;
} pair_t;

typedef struct {
    int num_values;
    int size;
    pair_t* values;
} hashtable_t;

uint32_t hashtable_find_location(const pair_t* values, const int num_values, const char* key);
void hashtable_resize(hashtable_t* hashtable, int new_size);
value_t hashtable_get(const hashtable_t* hashtable, const char* key);
uint32_t hashtable_put(hashtable_t* hashtable, const char* key, const value_t value);
value_t hashtable_get_or_create(hashtable_t* hashtable, const char* key, const value_t absent);
uint32_t hashtable_get_location_or_create(hashtable_t* hashtable, const char* key, const value_t absent);
uint32_t hashtable_inc(hashtable_t* hashtable, const char* key, const value_t absent, const value_t present);
hashtable_t* hashtable_new(int size);
void hashtable_free(hashtable_t* hashtable);

#endif
