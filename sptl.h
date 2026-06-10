#ifndef SPTL_H
#define SPTL_H

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INNER_CONCAT(a, b) a##b
#define CONCAT(a, b) INNER_CONCAT(a, b)

#define macro_var(id) CONCAT(id, __LINE__)

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

#define sp_da_type(da) __typeof__((da)->data)
#define sp_da_get(da, i) (da)->data[i]

// TODO: Generalize reserve operation into a common backend (related sp_ht_reserve)
#define SP_DA_INIT_CAP 16
#define sp_da_reserve(da, __expected__)                                                                                \
    do {                                                                                                               \
        const size_t expected = (__expected__);                                                                        \
        size_t capacity = (da)->capacity;                                                                              \
        if (capacity < expected) {                                                                                     \
            if (capacity == 0) {                                                                                       \
                capacity = expected;                                                                                   \
            }                                                                                                          \
            while (capacity < expected) {                                                                              \
                capacity *= 2;                                                                                         \
            }                                                                                                          \
            __typeof__((da)->data) data = (__typeof__((da)->data)) malloc(capacity * sizeof(*(da)->data));             \
            memcpy(data, (da)->data, (da)->count * sizeof(*(da)->data));                                               \
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
        memset((da), 0, sizeof(*(da)));                                                                                \
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

static inline Sp_String_Builder sp_cstr_to_sb(const char *cstr) {
    Sp_String_Builder sb = {0};
    sp_sb_appendf(&sb, "%s", cstr);
    return sb;
}

static inline const char *sp_sb_cstr(Sp_String_Builder *sb) { return sb->data; }

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
        memset((queue), 0, sizeof(*(queue)));                                                                          \
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
#define sp_ll_node_unwrap(ll, node) ((sp_ll_type(ll) *) (node)->data)

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

static inline uint32_t sp_ht_streq(const char *s1, const char *s2) { return (uint32_t) !strcmp(s1, s2); }

/*
 * Hash table implementation with closed addressing collision resolution, where buckets are backed by Sp_Dynamic_Array.
 *
 * Re-hashes entire table at 2x capacity on insertion when load factor reaches the ratio as defined by
 * `SP_HT_LOAD_CAPACITY` (default = 0.9).
 */
// TODO: strlen is undefined for non string types, meaning only const char* keys are supported as of right now
#define Sp_Hash_Table(K, T)                                                                                            \
    struct {                                                                                                           \
        Sp_Dynamic_Array(Sp_Dynamic_Array(struct {                                                                     \
            K key;                                                                                                     \
            T value;                                                                                                   \
        })) table;                                                                                                     \
        size_t count;                                                                                                  \
        uint32_t (*hash)(K, const size_t);                                                                             \
        uint32_t (*equal)(K, K);                                                                                       \
    }

#define sp_ht_node_t(ht) __typeof__(*(ht)->table.data->data)
#define sp_ht_key_type(ht) __typeof__((ht)->table.data->data->key)
#define sp_ht_value_type(ht) __typeof__((ht)->table.data->data->value)

#define SP_HT_LOAD_CAPACITY 0.9
#define SP_HT_INIT_CAP 16

#define sp_ht_reserve(ht, __expected__)                                                                                \
    do {                                                                                                               \
        const size_t expected = (__expected__);                                                                        \
        if (!(ht)->hash) {                                                                                             \
            (ht)->hash = _Generic((ht)->table.data->data->key, const char *: &hash_fnv, default: NULL);                \
        }                                                                                                              \
        if (!(ht)->equal) {                                                                                            \
            (ht)->equal = _Generic((ht)->table.data->data->key, const char *: &sp_ht_streq, default: NULL);            \
        }                                                                                                              \
        if ((ht)->table.capacity == 0) {                                                                               \
            sp_da_resize(&(ht)->table, expected < SP_HT_INIT_CAP ? expected : SP_HT_INIT_CAP);                         \
        } else if ((ht)->count > 0) {                                                                                  \
            __typeof__((ht)->table) old_table = (ht)->table;                                                           \
            (ht)->table = (__typeof__((ht)->table)) {0};                                                               \
            sp_da_resize(&(ht)->table, expected);                                                                      \
            for (size_t macro_var(i) = 0; macro_var(i) < old_table.count; ++macro_var(i)) {                            \
                for (size_t macro_var(j) = 0; macro_var(j) < sp_da_get(&old_table, macro_var(i)).count;                \
                     ++macro_var(j)) {                                                                                 \
                    sp_da_push(                                                                                        \
                        &(ht)->table.data[(ht)->hash(old_table.data[macro_var(i)].data[macro_var(j)].key,              \
                                                     strlen(old_table.data[macro_var(i)].data[macro_var(j)].key)) %    \
                                          (ht)->table.capacity],                                                       \
                        old_table.data[macro_var(i)].data[macro_var(j)]);                                              \
                }                                                                                                      \
                sp_da_free(&(sp_da_get(&old_table, macro_var(i))));                                                    \
            }                                                                                                          \
            sp_da_free(&old_table);                                                                                    \
        }                                                                                                              \
    } while (0)

/* Points `sp_ht_node_t_ptr` to the `sp_ht_node_t` instance containing the key, or NULL if not found.
 * This pointer can be invalidated by any subsequent instructions to the `Sp_Hash_Table` object. */
#define sp_ht_get(ht, __key__, sp_ht_node_t_ptr)                                                                       \
    do {                                                                                                               \
        if ((sp_ht_node_t_ptr)) {                                                                                      \
            size_t macro_var(idx) = (ht)->hash((__key__), strlen(__key__)) % (ht)->table.capacity;                     \
            for (size_t macro_var(i) = 0; macro_var(i) < sp_da_get(&(ht)->table, macro_var(idx)).count;                \
                 ++macro_var(i)) {                                                                                     \
                if (!(ht)->equal((__key__),                                                                            \
                                 sp_da_get(&(sp_da_get(&(ht)->table, macro_var(idx))), macro_var(i)).key)) {           \
                    continue;                                                                                          \
                } else {                                                                                               \
                    *((sp_ht_node_t_ptr)) = &(sp_da_get(&sp_da_get(&(ht)->table, macro_var(idx)), macro_var(i)));      \
                }                                                                                                      \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

#define sp_ht_insert(ht, __key__, __value__)                                                                           \
    do {                                                                                                               \
        if ((ht)->table.capacity == 0) {                                                                               \
            sp_ht_reserve((ht), SP_HT_INIT_CAP);                                                                       \
        } else if ((double) (ht)->count > (SP_HT_LOAD_CAPACITY * (double) (ht)->table.capacity)) {                     \
            sp_ht_reserve((ht), (ht)->table.capacity * 2);                                                             \
        }                                                                                                              \
        size_t macro_var(idx) = (ht)->hash(__key__, strlen(__key__)) % (ht)->table.capacity;                           \
        for (size_t macro_var(i) = 0; macro_var(i) < (ht)->table.data[macro_var(idx)].count; ++macro_var(i)) {         \
            if ((ht)->equal(__key__, (ht)->table.data[macro_var(idx)].data[macro_var(i)].key)) {                       \
                (ht)->table.data[macro_var(idx)].data[macro_var(i)].value = __value__;                                 \
                goto macro_var(sp_ht_insert_end);                                                                      \
            }                                                                                                          \
        }                                                                                                              \
        sp_da_push(&(ht)->table.data[macro_var(idx)], ((sp_ht_node_t(ht)) {.key = __key__, .value = __value__}));      \
        ++(ht)->count;                                                                                                 \
        macro_var(sp_ht_insert_end) : break;                                                                           \
    } while (0)

#define sp_ht_free(ht)                                                                                                 \
    do {                                                                                                               \
        for (size_t macro_var(i) = 0; macro_var(i) < (ht)->table.count; ++macro_var(i)) {                              \
            sp_da_free(&(ht)->table.data[macro_var(i)]);                                                               \
        }                                                                                                              \
        sp_da_free(&(ht)->table);                                                                                      \
        memset((ht), 0, sizeof(*(ht)));                                                                                \
    } while (0)

#endif
