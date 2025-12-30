#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

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

#define sp_da_reserve(da, expected)                                                                                    \
    do {                                                                                                               \
        if ((da)->capacity < ((size_t) (expected))) {                                                                  \
            if ((da)->capacity == 0) {                                                                                 \
                (da)->capacity = 16;                                                                                   \
            }                                                                                                          \
            while ((da)->capacity < ((size_t) (expected))) {                                                           \
                (da)->capacity *= 2;                                                                                   \
            }                                                                                                          \
            (da)->data = realloc((da)->data, (da)->capacity);                                                          \
        }                                                                                                              \
    } while (0)

#define sp_da_append(da, element)                                                                                      \
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

    do {
        if ((sb)->capacity < ((size_t) (sb->count + (size_t) count + 1))) {
            if ((sb)->capacity == 0) {
                (sb)->capacity = 16;
            }
            while ((sb)->capacity < ((size_t) (sb->count + (size_t) count + 1))) {
                (sb)->capacity *= 2;
            }
            (sb)->data = realloc((sb)->data, (sb)->capacity);
        }
    } while (0); // allocates enough room for a null terminator

    char* dest = sb->data + sb->count;
    va_start(arg, format);
    vsnprintf(dest, (size_t) count + 1, format, arg);
    va_end(arg);

    sb->count += (size_t) count; // increased allocated count but not include null terminator

    return count;
}
