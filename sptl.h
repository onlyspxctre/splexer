#ifndef SPTL_H
#define SPTL_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INNER_CONCAT(a, b) a##b
#define CONCAT(a, b) INNER_CONCAT(a, b)

#if defined(__GNUC__) || defined(__clang__)
#define sp_unreachable()                                                                                               \
    do {                                                                                                               \
        fprintf(stderr, "[ERROR] <%s:%d> Unreachable segment reached!\n", __FILE__, __LINE__);                         \
        __builtin_unreachable();                                                                                       \
    } while (0)

#endif

#define KNRM "\x1B[0m"  // Normal/Reset
#define KRED "\x1B[31m" // Red Foreground
#define KGRN "\x1B[32m" // Green Foreground
#define KYEL "\x1B[33m" // Yellow Foreground
#define KBLU "\x1B[34m" // Blue Foreground
#define KMAG "\x1B[35m" // Magenta Foreground
#define KCYN "\x1B[36m" // Cyan Foreground
#define KWHT "\x1B[37m" // White Foreground

#ifdef SP_VERBOSE_LOGS
#define __fprintf_verbose(fd, format) fprintf(fd, format)
#else
#define __fprintf_verbose(fd, format) return 0
#endif

typedef enum {
    SP_INFO,
    SP_WARNING,
    SP_ERROR,
    SP_VERBOSE,
} Sp_Log_Level;

__attribute__((format(printf, 2, 3))) static inline int sp_log(Sp_Log_Level log_level, const char *format, ...) {
    va_list arg;
    FILE *fd;

    switch (log_level) {
        case SP_INFO:
            fd = stdout;
            fprintf(fd, "[" KGRN "INFO" KNRM "] ");
            break;
        case SP_VERBOSE:
            fd = stdout;
            __fprintf_verbose(fd, "[" KCYN "INFO" KNRM "] ");
            break;
        case SP_WARNING:
            fd = stderr;
            fprintf(fd, "[" KYEL "WARNING" KNRM "] ");
            break;
        case SP_ERROR:
            fd = stderr;
            fprintf(fd, "[" KRED "ERROR" KNRM "] ");
            break;
    }

    va_start(arg, format);

    int count = vfprintf(fd, format, arg);

    va_end(arg);
    putchar('\n');

    return count;
}

/*
 * Standard-issue dynamic array.
 *
 */
#define Sp_Dynamic_Array(T)                                                                                            \
    struct {                                                                                                           \
        T *data;                                                                                                       \
        size_t count;                                                                                                  \
        size_t capacity;                                                                                               \
    }

// TODO: Generalize reserve operation into a common backend (related sp_ht_reserve)
#define SP_DA_INIT_CAP 16
#define sp_da_reserve(da, __expected__)                                                                                \
    do {                                                                                                               \
        const size_t expected = (__expected__);                                                                        \
        size_t capacity = (da)->capacity;                                                                              \
        if (capacity < expected) {                                                                                     \
            if (capacity == 0) {                                                                                       \
                capacity = SP_DA_INIT_CAP;                                                                             \
            }                                                                                                          \
            while (capacity < expected) {                                                                              \
                capacity *= 2;                                                                                         \
            }                                                                                                          \
            __typeof__((da)->data) data = (__typeof__((da)->data)) malloc(capacity * sizeof(*(da)->data));             \
            memcpy(data, (da)->data, (da)->capacity * sizeof(*(da)->data));                                            \
            free((da)->data);                                                                                          \
            (da)->data = data;                                                                                         \
            (da)->capacity = capacity;                                                                                 \
        }                                                                                                              \
    } while (0)

#define sp_da_resize(da, __count__)                                                                                    \
    do {                                                                                                               \
        size_t count = (__count__);                                                                                    \
        if (count > (da)->capacity) {                                                                                  \
            sp_da_reserve((da), count);                                                                                \
        }                                                                                                              \
        if (count > (da)->count) {                                                                                     \
            memset((da)->data + (da)->count, 0, (count - (da)->count) * sizeof(*(da)->data));                          \
        }                                                                                                              \
        (da)->count = count;                                                                                           \
    } while (0)

#define sp_da_push(da, element)                                                                                        \
    do {                                                                                                               \
        sp_da_reserve((da), (da)->count + 1);                                                                          \
        (da)->data[(da)->count++] = element;                                                                           \
    } while (0)

#define sp_da_pop(da)                                                                                                  \
    do {                                                                                                               \
        if ((da)->count > 0)                                                                                           \
            --(da)->count;                                                                                             \
        (da)->data[(da)->count] = 0;                                                                                   \
    } while (0)

/*
 * Clears the dynamic array, but does NOT free it.
 */
#define sp_da_clear(da)                                                                                                \
    do {                                                                                                               \
        memset((da)->data, 0, (da)->capacity * sizeof(*(da)->data));                                                   \
        (da)->count = 0;                                                                                               \
    } while (0)

#define sp_da_free(da)                                                                                                 \
    do {                                                                                                               \
        free((da)->data);                                                                                              \
        (da)->data = NULL;                                                                                             \
        (da)->count = 0;                                                                                               \
        (da)->capacity = 0;                                                                                            \
    } while (0)

typedef Sp_Dynamic_Array(char) Sp_String_Builder;

/*
 * Appends formatted `format` to `sb`, extending the dynamic array if necessary.
 *
 * Increments `sb->count` by the length of parsed `format` excluding the null terminator, but `sb->data`
 * itself is safe-to-use.
 */
__attribute__((format(printf, 2, 3))) static inline int sp_sb_appendf(Sp_String_Builder *sb, const char *format, ...) {
    va_list arg;

    va_start(arg, format);
    int count = vsnprintf(NULL, 0, format, arg);
    va_end(arg);

    sp_da_reserve(sb, sb->count + (size_t) count + 1); // allocate enough room for null terminator to

    char *dest = sb->data + sb->count;
    va_start(arg, format);
    vsnprintf(dest, (size_t) count + 1, format, arg);
    va_end(arg);

    sb->count += (size_t) count; // increased allocated count but not include null terminator

    return count;
}

static inline Sp_String_Builder sp_cstr_to_sb(const char* cstr) {
    Sp_String_Builder sb = {0};
    sp_sb_appendf(&sb, "%s", cstr);
    return sb;
}

static inline const char *sp_sb_cstr(Sp_String_Builder* sb) {
    return sb->data;
}

#define Sp_Queue(T)                                                                                                    \
    struct {                                                                                                           \
        T *data;                                                                                                       \
        size_t count;                                                                                                  \
        size_t head;                                                                                                   \
        size_t tail;                                                                                                   \
        size_t capacity;                                                                                               \
    }

#define SP_QUEUE_INIT_CAP SP_DA_INIT_CAP
#define sp_queue_reserve(queue, __expected__)                                                                          \
    do {                                                                                                               \
        const size_t expected = (__expected__);                                                                        \
        size_t capacity = (queue)->capacity;                                                                           \
        if (capacity < expected) {                                                                                     \
            if (capacity == 0) {                                                                                       \
                capacity = SP_QUEUE_INIT_CAP;                                                                          \
            }                                                                                                          \
            while (capacity < expected) {                                                                              \
                capacity *= 2;                                                                                         \
            }                                                                                                          \
            __typeof__((queue)->data) data = (__typeof__((queue)->data)) calloc(capacity, sizeof(*(queue)->data));     \
            for (size_t i = 0; i < (queue)->capacity; ++i) {                                                           \
                data[i] = (queue)->data[((queue)->head + i) % (queue)->capacity];                                      \
            }                                                                                                          \
            free((queue)->data);                                                                                       \
            (queue)->data = data;                                                                                      \
            (queue)->head = 0;                                                                                         \
            (queue)->tail = (queue)->count;                                                                            \
            (queue)->capacity = capacity;                                                                              \
        }                                                                                                              \
    } while (0)

#define sp_queue_push(queue, element)                                                                                  \
    do {                                                                                                               \
        sp_queue_reserve((queue), (queue)->count + 1);                                                                 \
        (queue)->data[(queue)->tail++ % (queue)->capacity] = (element);                                                \
        ++(queue)->count;                                                                                              \
    } while (0)

#define sp_queue_pop(queue)                                                                                            \
    do {                                                                                                               \
        ++(queue)->head;                                                                                               \
        if ((queue)->count > 0)                                                                                        \
            --(queue)->count;                                                                                          \
    } while (0)

#define sp_queue_peek(queue) ((queue)->count == 0 ? 0 : (queue)->data[(queue)->head % (queue)->capacity])

#define sp_queue_free(queue)                                                                                           \
    do {                                                                                                               \
        free((queue)->data);                                                                                           \
        (queue)->data = NULL;                                                                                          \
        (queue)->count = 0;                                                                                            \
        (queue)->head = 0;                                                                                             \
        (queue)->tail = 0;                                                                                             \
        (queue)->capacity = 0;                                                                                         \
    } while (0)

typedef struct sp_ll_node {
    struct sp_ll_node *prev;
    struct sp_ll_node *next;
    char data[];
} sp_ll_node;

#define Sp_Linked_List(T)                                                                                              \
    struct {                                                                                                           \
        T type;                                                                                                        \
        sp_ll_node *head;                                                                                              \
        sp_ll_node *tail;                                                                                              \
    }

/* Returns the type of the underlying data stored within the Sp_Linked_List. */
#define sp_ll_type(ll) __typeof__((ll)->type)
/* Returns a pointer of `sp_ll_type(ll)` to the underlying data stored at `sp_ll_node* node`. */
#define sp_ll_node_unwrap(ll, node) ((sp_ll_type(ll)*) (node)->data)

#define sp_ll_push_back(ll, element)                                                                                   \
    do {                                                                                                               \
        if ((ll)->head == NULL && (ll)->tail == NULL) { /* uninitialized state */                                      \
            (ll)->head = malloc(sizeof(*(ll)->head) + sizeof((ll)->type));                                             \
            *sp_ll_node_unwrap(ll, (ll)->head) = (element);                                                            \
            (ll)->tail = (ll)->head;                                                                                   \
        } else {                                                                                                       \
            (ll)->tail->next = malloc(sizeof(*(ll)->tail) + sizeof((ll)->type));                                       \
            (ll)->tail->next->prev = (ll)->tail;                                                                       \
            (ll)->tail = (ll)->tail->next;                                                                             \
            *sp_ll_node_unwrap(ll, (ll)->tail) = (element);                                                            \
        }                                                                                                              \
    } while (0)

#define sp_ll_push_front(ll, element)                                                                                  \
    do {                                                                                                               \
        if ((ll)->head == NULL && (ll)->tail == NULL) { /* uninitialized state */                                      \
            (ll)->head = malloc(sizeof(*(ll)->head) + sizeof((ll)->type);                                              \
            *sp_ll_node_unwrap(ll, (ll)->head) = (element);                                                            \
            (ll)->tail = (ll)->head;                                                                                   \
        } else {                                                                                                       \
            (ll)->head->prev = malloc(sizeof(*(ll)->tail) + sizeof((ll)->type);                                        \
            (ll)->head->prev->next = (ll)->head;                                                                       \
            (ll)->head = (ll)->head->prev;                                                                             \
            *sp_ll_node_unwrap(ll, (ll)->head) = (element);                                                            \
        }                                                                                                              \
    } while (0)

// TODO: Make sp_ll_pop use a common backend for common functions

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
        void *next;                                                                                                    \
        while ((ll)->head) {                                                                                           \
            next = (ll)->head->next;                                                                                   \
            free((ll)->head);                                                                                          \
            (ll)->head = next;                                                                                         \
        }                                                                                                              \
        (ll)->head = NULL;                                                                                             \
        (ll)->tail = NULL;                                                                                             \
    } while (0)

#define FNV_PRIME_32 16777619
#define FNV_OFFSET_BASIS_32 2166136261

static inline uint32_t hash_fnv(const char *data, const size_t bytes) {
    uint32_t hash = FNV_OFFSET_BASIS_32;

    for (size_t i = 0; i < bytes; ++i) {
        hash = (hash ^ (uint32_t) data[i]) * FNV_PRIME_32; // EXPERIMENTAL: casting to uint32_t may break hash function
    }

    return hash;
}

// size_t hash_to_idx(const char* key, size_t capacity) {}

/*
 * Hash table implementation with open addressing collision resolution.
 *
 * Re-hashes entire table at 2x capacity on insertion when load factor reaches the ratio as defined by
 * `SP_HT_LOAD_CAPACITY` (default = 0.75).
 */
#define Sp_Hash_Table(T)                                                                                               \
    struct {                                                                                                           \
        struct {                                                                                                       \
            char *key;                                                                                                 \
            T value;                                                                                                   \
        } *nodes;                                                                                                      \
        size_t count;                                                                                                  \
        size_t capacity;                                                                                               \
    }

#define sp_ht_node_ptr(ht) __typeof__((ht)->nodes)

#define SP_HT_INIT_CAP 16
#define SP_HT_LOAD_CAPACITY 0.75
#define sp_ht_reserve(ht, __expected__)                                                                                \
    do {                                                                                                               \
        const size_t expected = (__expected__);                                                                        \
        if ((ht)->capacity < expected) {                                                                               \
            if ((ht)->capacity == 0) {                                                                                 \
                (ht)->capacity = SP_HT_INIT_CAP;                                                                       \
            }                                                                                                          \
            size_t old_capacity = (ht)->capacity;                                                                      \
            while ((ht)->capacity < (size_t) (SP_HT_LOAD_CAPACITY * (double) expected)) {                              \
                (ht)->capacity *= 2;                                                                                   \
            }                                                                                                          \
            if (!(ht)->nodes) {                                                                                        \
                (ht)->nodes = calloc((ht)->capacity, sizeof(*(ht)->nodes));                                            \
            } else {                                                                                                   \
                sp_ht_rehash((ht), old_capacity);                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define sp_ht_rehash(ht, __old_capacity__)                                                                             \
    do {                                                                                                               \
        size_t CONCAT(old_capacity, __LINE__) = __old_capacity__;                                                      \
        sp_ht_node_ptr((ht)) old_nodes = (ht)->nodes;                                                                  \
        (ht)->nodes = calloc((ht)->capacity, sizeof(*(ht)->nodes));                                                    \
        for (size_t i = 0; i < CONCAT(old_capacity, __LINE__); ++i) {                                                  \
            if (!old_nodes[i].key) {                                                                                   \
                continue;                                                                                              \
            }                                                                                                          \
            size_t index = sp_ht_hash((ht), old_nodes[i].key);                                                         \
            sp_ht_node_insert((ht), index, old_nodes[i].key, old_nodes[i].value);                                      \
            free(old_nodes[i].key);                                                                                    \
        }                                                                                                              \
        free(old_nodes);                                                                                               \
    } while (0)

/*
 * WARNING: Uses non-standard statement expressions, which may not be supported by all C compilers.
 *
 * If the table contains the key, an index to the node with the key and its associated value will be returned.
 * If the table does not contain the key, an index to an empty node will be returned.
 *
 * ERRORS
 * If open addressing collision resolution cannot find an empty node, the capacity of the hash table will be
 * returned. In this case, you would most likely need to regrow the hash table.
 */
#define sp_ht_hash(ht, expected_key)                                                                                   \
    ({                                                                                                                 \
        size_t index = hash_fnv((expected_key), strlen((expected_key))) % (ht)->capacity;                              \
        while (index < (ht)->capacity) {                                                                               \
            if (!(ht)->nodes[index].key) {                                                                             \
                break;                                                                                                 \
            } else if (strcmp((ht)->nodes[index].key, (expected_key)) == 0) {                                          \
                break;                                                                                                 \
            }                                                                                                          \
            ++index;                                                                                                   \
        }                                                                                                              \
        index;                                                                                                         \
    })

#define sp_ht_node_insert(ht, index, expected_key, element)                                                            \
    do {                                                                                                               \
        if (!(ht)->nodes[index].key) {                                                                                 \
            (ht)->nodes[index].key = malloc(strlen((expected_key)) + 1);                                               \
            strncpy((ht)->nodes[index].key, (expected_key), strlen((expected_key)));                                   \
            (ht)->nodes[index].key[strlen((expected_key))] = '\0';                                                     \
            ++(ht)->count;                                                                                             \
        }                                                                                                              \
        (ht)->nodes[index].value = element;                                                                            \
    } while (0)

#define sp_ht_insert(ht, expected_key, e)                                                                              \
    do {                                                                                                               \
        sp_ht_reserve((ht), (ht)->count + 1);                                                                          \
        while (sp_ht_hash((ht), (expected_key)) == (ht)->capacity) {                                                   \
            sp_ht_reserve((ht), (ht)->capacity * 2);                                                                   \
        }                                                                                                              \
        __typeof__(e) element = e;                                                                                     \
        size_t index = sp_ht_hash((ht), (expected_key));                                                               \
        sp_ht_node_insert((ht), index, (expected_key), element);                                                       \
    } while (0)

/*
 * WARNING: Uses non-standard statement expressions, which may not be supported by all C compilers.
 *
 * Returns an `sp_ht_node_ptr(ht)`, or a pointer to the node of the value of `expected_key`, or NULL if not
 * found.
 */
#define sp_ht_get(ht, expected_key)                                                                                    \
    (sp_ht_hash((ht), (expected_key)) != (ht)->capacity                                                                \
         ? ((ht)->nodes[sp_ht_hash((ht), (expected_key))].key ? &((ht)->nodes[sp_ht_hash((ht), (expected_key))])       \
                                                              : NULL)                                                  \
         : NULL)

/*
 * TODO: FOR DEBUGGING PURPOSES ONLY, REMOVE LATER MOST LIKELY
 */
#define sp_ht_print(ht)                                                                                                \
    do {                                                                                                               \
        for (size_t i = 0; i < (ht)->capacity; ++i) {                                                                  \
            printf("%ld: ", i);                                                                                        \
            if (!(ht)->nodes[i].key) {                                                                                 \
                putchar('\n');                                                                                         \
                continue;                                                                                              \
            } else {                                                                                                   \
                printf("\"%s\" -> %d\n", (ht)->nodes[i].key, (ht)->nodes[i].value);                                    \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define sp_ht_free(ht)                                                                                                 \
    do {                                                                                                               \
        for (size_t i = 0; i < (ht)->capacity; ++i) {                                                                  \
            if (!(ht)->nodes[i].key) {                                                                                 \
                continue;                                                                                              \
            }                                                                                                          \
            free((ht)->nodes[i].key);                                                                                  \
            (ht)->nodes[i].key = NULL;                                                                                 \
        }                                                                                                              \
        free((ht)->nodes);                                                                                             \
        (ht)->nodes = NULL;                                                                                            \
        (ht)->count = 0;                                                                                               \
        (ht)->capacity = 0;                                                                                            \
    } while (0)

#endif
