#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// http://www.azillionmonkeys.com/qed/hash.html
#include "stdint.h"
#undef get16bits
#if (defined(__GNUC__) && defined(__i386__)) || defined(__WATCOMC__) \
  || defined(_MSC_VER) || defined (__BORLANDC__) || defined (__TURBOC__)
#define get16bits(d) (*((const uint16_t *) (d)))
#endif

#if !defined (get16bits)
#define get16bits(d) ((((uint32_t)(((const uint8_t *)(d))[1])) << 8)\
                       +(uint32_t)(((const uint8_t *)(d))[0]) )
#endif
#include "hashtable.h"

uint32_t hash_function (const char * data, int len) {
    uint32_t hash = len, tmp;
    int rem;

    if (len <= 0 || data == NULL) return 0;

    rem = len & 3;
    len >>= 2;

    /* Main loop */
    for (;len > 0; len--) {
        hash  += get16bits (data);
        tmp    = (get16bits (data+2) << 11) ^ hash;
        hash   = (hash << 16) ^ tmp;
        data  += 2*sizeof (uint16_t);
        hash  += hash >> 11;
    }

    /* Handle end cases */
    switch (rem) {
        case 3: hash += get16bits (data);
                hash ^= hash << 16;
                hash ^= data[sizeof (uint16_t)] << 18;
                hash += hash >> 11;
                break;
        case 2: hash += get16bits (data);
                hash ^= hash << 11;
                hash += hash >> 17;
                break;
        case 1: hash += *data;
                hash ^= hash << 10;
                hash += hash >> 1;
    }

    hash ^= hash << 3;
    hash += hash >> 5;
    hash ^= hash << 4;
    hash += hash >> 17;
    hash ^= hash << 25;
    hash += hash >> 6;

    return hash;
}

uint32_t hashtable_find_location(const pair_t* values, const int num_values, const char* key) {
    int length = strlen(key);
    uint32_t location = hash_function(key, length) % num_values;
    while ((values[location].key != NULL) && (strcmp(values[location].key, key) != 0)) {
        location = (location + 1) % num_values;
    }
    return location;
}

void hashtable_resize(hashtable_t* hashtable, int new_size) {
    pair_t* new_values = calloc(sizeof(pair_t), new_size);
    int i;
    for(i = 0; i < hashtable->num_values; i++) {
        if(hashtable->values[i].key != NULL) {
            uint32_t location = hashtable_find_location(new_values, new_size, hashtable->values[i].key);
            new_values[location].key = hashtable->values[i].key;
            new_values[location].value = hashtable->values[i].value;
        }
    }
    free(hashtable->values);
    hashtable->values = new_values;
    hashtable->num_values = new_size;
}

value_t hashtable_get(const hashtable_t* hashtable, const char* key) {
    uint32_t location = hashtable_find_location(hashtable->values, hashtable->num_values, key);
    if(hashtable->values[location].key != NULL) return hashtable->values[location].value;
    return (value_t) -1;
}

uint32_t hashtable_put(hashtable_t* hashtable, const char* key, const value_t value) {
    if(hashtable->size >= hashtable->num_values / 2) hashtable_resize(hashtable, hashtable->num_values * 2);
    uint32_t location = hashtable_find_location(hashtable->values, hashtable->num_values, key);
    if(hashtable->values[location].key != NULL) hashtable->values[location].value = value;
    else {
        hashtable->values[location].key = strdup(key);
        hashtable->values[location].value = value;
        hashtable->size ++;
    }
    return location;
}

value_t hashtable_get_or_create(hashtable_t* hashtable, const char* key, const value_t absent) {
    if(hashtable->size >= hashtable->num_values / 2) hashtable_resize(hashtable, hashtable->num_values * 2);
    uint32_t location = hashtable_find_location(hashtable->values, hashtable->num_values, key);
    if(hashtable->values[location].key == NULL) {
        hashtable->values[location].key = strdup(key);
        hashtable->values[location].value = absent;
        hashtable->size ++;
    }
    return hashtable->values[location].value;
}

uint32_t hashtable_get_location_or_create(hashtable_t* hashtable, const char* key, const value_t absent) {
    if(hashtable->size >= hashtable->num_values / 2) hashtable_resize(hashtable, hashtable->num_values * 2);
    uint32_t location = hashtable_find_location(hashtable->values, hashtable->num_values, key);
    if(hashtable->values[location].key == NULL) {
        hashtable->values[location].key = strdup(key);
        hashtable->values[location].value = absent;
        hashtable->size ++;
    }
    return location;
}

uint32_t hashtable_inc(hashtable_t* hashtable, const char* key, const value_t absent, const value_t present) {
    if(hashtable->size >= hashtable->num_values / 2) hashtable_resize(hashtable, hashtable->num_values * 2);
    uint32_t location = hashtable_find_location(hashtable->values, hashtable->num_values, key);
    if(hashtable->values[location].key != NULL) hashtable->values[location].value.int_value += present.int_value;
    else {
        hashtable->values[location].key = strdup(key);
        hashtable->values[location].value = absent;
        hashtable->size ++;
    }
    return location;
}

hashtable_t* hashtable_new(int size) {
    hashtable_t* hashtable = calloc(sizeof(hashtable_t), 1);
    hashtable->num_values = size;
    hashtable->values = calloc(sizeof(pair_t), size);
    return hashtable;
}

void hashtable_free(hashtable_t* hashtable) {
    int i;
    for(i = 0; i < hashtable->num_values; i++) free(hashtable->values[i].key);
    free(hashtable->values);
    free(hashtable);
}
