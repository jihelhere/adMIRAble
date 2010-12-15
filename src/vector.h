#ifndef __CRF_VECTOR__
#define __CRF_VECTOR__

typedef struct {
    int element;
    int size;
    int length;
} __vector_t;

#define __vector_actual(vector) (((__vector_t*)vector)[-1])
#define __vector_element(vector) (__vector_actual(vector).element)
#define __vector_size(vector) (__vector_actual(vector).size)
#define vector_length(vector) (__vector_actual(vector).length)
#define vector_last(vector) (vector[vector_length(vector) - 1])

#define vector_new(type, name, size) { \
        name = malloc(sizeof(type) * size + sizeof(__vector_t)) + sizeof(__vector_t); \
        vector_length(name) = 0; \
        __vector_size(name) = size; \
        __vector_element(name) = sizeof(type); \
    }
#define vector_free(name) free(&__vector_actual(name));
#define vector_push(name, value) { \
        if(vector_length(name) >= __vector_size(name)) { \
            __vector_size(name) *= 2; \
            name = realloc(&__vector_actual(name), __vector_element(name) * __vector_size(name) + sizeof(__vector_t)) + sizeof(__vector_t); \
        } \
        name[vector_length(name)] = value; \
        vector_length(name) ++; \
    }

#endif
