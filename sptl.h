#pragma once

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INNER_CONCAT(a, b) a##b
#define CONCAT(a, b) INNER_CONCAT(a, b)

/*
 * Standard-issue dynamic array.
 *
 * TODO: support other data types, currently only supported data type is char
 */
#define Sp_Dynamic_Array(T)                                                                                            \
    typedef struct {                                                                                                   \
        T* data;                                                                                                       \
        size_t count;                                                                                                  \
        size_t capacity;                                                                                               \
    }

#define SP_DA_INIT_CAP 16
#define sp_da_reserve(da, expected)                                                                                    \
    do {                                                                                                               \
        if ((da)->capacity < ((size_t) (expected))) {                                                                  \
            if ((da)->capacity == 0) {                                                                                 \
                (da)->capacity = SP_DA_INIT_CAP;                                                                       \
            }                                                                                                          \
            while ((da)->capacity < ((size_t) (expected))) {                                                           \
                (da)->capacity *= 2;                                                                                   \
            }                                                                                                          \
            (da)->data = realloc((da)->data, (da)->capacity);                                                          \
        }                                                                                                              \
    } while (0)

#define sp_da_push(da, element)                                                                                        \
    do {                                                                                                               \
        sp_da_reserve((da), (da)->count + 1);                                                                          \
        (da)->data[(da)->count] = element;                                                                             \
        ++(da)->count;                                                                                                 \
    } while (0)

#define sp_da_clear(da)                                                                                                \
    do {                                                                                                               \
        memset((da)->data, 0, (da)->count);                                                                            \
        (da)->count = 0;                                                                                               \
    } while (0)

#define sp_da_free(da)                                                                                                 \
    do {                                                                                                               \
        free((da)->data);                                                                                              \
        (da)->count = 0;                                                                                               \
        (da)->capacity = 0;                                                                                            \
    } while (0)

Sp_Dynamic_Array(char) Sp_String_Builder;

/*
Appends formatted `format` to `sb`, extending the dynamic array if necessary.

Increments `sb->count` by the length of parsed `format` excluding the null terminator, but `sb->data` itself is
safe-to-use.
*/
__attribute__((format(printf, 2, 3))) int sp_sb_appendf(Sp_String_Builder* sb, const char* format, ...) {
    va_list arg;

    va_start(arg, format);
    int count = vsnprintf(NULL, 0, format, arg);
    va_end(arg);

    sp_da_reserve(sb, sb->count + (size_t) count + 1); // allocate enough room for null terminator to

    char* dest = sb->data + sb->count;
    va_start(arg, format);
    vsnprintf(dest, (size_t) count + 1, format, arg);
    va_end(arg);

    sb->count += (size_t) count; // increased allocated count but not include null terminator

    return count;
}

#define Sp_Linked_List(T)                                                                                              \
    struct CONCAT(Sp_Internal_Node, __LINE__) {                                                                        \
        T data;                                                                                                        \
        struct CONCAT(Sp_Internal_Node, __LINE__) * prev;                                                              \
        struct CONCAT(Sp_Internal_Node, __LINE__) * next;                                                              \
    };                                                                                                                 \
    typedef struct {                                                                                                   \
        struct CONCAT(Sp_Internal_Node, __LINE__) * head;                                                              \
        struct CONCAT(Sp_Internal_Node, __LINE__) * tail;                                                              \
    }

#define sp_ll_push_back(ll, element)                                                                                   \
    do {                                                                                                               \
        if ((ll)->head == NULL && (ll)->tail == NULL) { /* uninitialized state */                                      \
            (ll)->head = malloc(sizeof(*(ll)->head));                                                                  \
            (ll)->head->data = element;                                                                                \
            (ll)->tail = (ll)->head;                                                                                   \
        } else {                                                                                                       \
            (ll)->tail->next = malloc(sizeof(*(ll)->tail));                                                            \
            (ll)->tail->next->prev = (ll)->tail;                                                                       \
            (ll)->tail = (ll)->tail->next;                                                                             \
            (ll)->tail->data = element;                                                                                \
        }                                                                                                              \
    } while (0)

#define sp_ll_push_front(ll, element)                                                                                  \
    do {                                                                                                               \
        if ((ll)->head == NULL && (ll)->tail == NULL) { /* uninitialized state */                                      \
            (ll)->head = malloc(sizeof(*(ll)->head));                                                                  \
            (ll)->head->data = element;                                                                                \
            (ll)->tail = (ll)->head;                                                                                   \
        } else {                                                                                                       \
            (ll)->head->prev = malloc(sizeof(*(ll)->tail));                                                            \
            (ll)->head->prev->next = (ll)->head;                                                                       \
            (ll)->head = (ll)->head->prev;                                                                             \
            (ll)->head->data = element;                                                                                \
        }                                                                                                              \
    } while (0)

// TODO: Make sp_ll_pop use a common backend for common functions
//
#define sp_ll_pop_back(ll)                                                                                             \
    do {                                                                                                               \
        if ((ll)->head == NULL && (ll)->tail == NULL) { /* uninitialized state */                                      \
            break;                                                                                                     \
        } else if ((ll)->head == (ll)->tail) { /* count == 1 */                                                        \
            free((ll)->head);                                                                                          \
            (ll)->head = NULL;                                                                                         \
            (ll)->tail = NULL;                                                                                         \
        } else {                                                                                                       \
            (ll)->tail = (ll)->tail->prev;                                                                             \
            free((ll)->tail->next);                                                                                    \
            (ll)->tail->next = NULL;                                                                                   \
        }                                                                                                              \
    } while (0)

#define sp_ll_pop_front(ll)                                                                                            \
    do {                                                                                                               \
        if ((ll)->head == NULL && (ll)->tail == NULL) { /* uninitialized state */                                      \
            break;                                                                                                     \
        } else if ((ll)->head == (ll)->tail) { /* count == 1 */                                                        \
            free((ll)->head);                                                                                          \
            (ll)->head = NULL;                                                                                         \
            (ll)->tail = NULL;                                                                                         \
        } else {                                                                                                       \
            (ll)->head = (ll)->head->next;                                                                             \
            free((ll)->head->prev);                                                                                    \
            (ll)->head->prev = NULL;                                                                                   \
        }                                                                                                              \
    } while (0)

#define sp_ll_free(ll)                                                                                                 \
    do {                                                                                                               \
        void* next;                                                                                                    \
        while ((ll)->head) {                                                                                           \
            next = (ll)->head->next;                                                                                   \
            free((ll)->head);                                                                                          \
            (ll)->head = next;                                                                                         \
        }                                                                                                              \
        (ll)->head = NULL;                                                                                             \
        (ll)->tail = NULL;                                                                                             \
    } while (0)

#define sp_ll_node_ptr(ll) __typeof__((ll)->head)

#define FNV_PRIME_32 16777619
#define FNV_OFFSET_BASIS_32 2166136261

static inline uint32_t hash_fnv(const char* data, const size_t bytes) {
    uint32_t hash = FNV_OFFSET_BASIS_32;

    for (size_t i = 0; i < bytes; ++i) {
        hash = (hash ^ (uint32_t) data[i]) * FNV_PRIME_32; // EXPERIMENTAL: casting to uint32_t may break hash function
    }

    return hash;
}

#define Sp_Hash_Table(T)                                                                                               \
    struct {                                                                                                           \
        T* data;                                                                                                       \
        size_t count;                                                                                                  \
        size_t capacity;                                                                                               \
    }

#define SP_HT_INIT_CAP 64
#define sp_ht_reserve(ht, expected)                                                                                    \
    do {                                                                                                               \
        if ((ht)->capacity < ((size_t) (expected))) {                                                                  \
            if ((ht)->capacity == 0) {                                                                                 \
                (ht)->capacity = SP_HT_INIT_CAP;                                                                       \
            }                                                                                                          \
            while ((ht)->capacity < ((size_t) (expected))) {                                                           \
                (ht)->capacity *= 2;                                                                                   \
            }                                                                                                          \
            (ht)->data = realloc((ht)->data, (ht)->capacity);                                                          \
        }                                                                                                              \
    } while (0)

//TESTE
#define sp_ht_insert(ht)\
