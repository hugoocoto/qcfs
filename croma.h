#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Internal helpers for croma_unused (up to 6 arguments). */
#define _croma_unused1(what, ...) __VA_OPT__(_croma_unused2(__VA_ARGS__);)(void) (what)
#define _croma_unused2(what, ...) __VA_OPT__(_croma_unused3(__VA_ARGS__);)(void) (what)
#define _croma_unused3(what, ...) __VA_OPT__(_croma_unused4(__VA_ARGS__);)(void) (what)
#define _croma_unused4(what, ...) __VA_OPT__(_croma_unused5(__VA_ARGS__);)(void) (what)
#define _croma_unused5(what, ...) (void) (what)

#define croma_unused(what, ...) __VA_OPT__(_croma_unused1(__VA_ARGS__);)(void) (what)

/* == OPTIONS == */

/* Expose aliases without croma_ prefix for the full public API. */
#ifndef TRIMPREFIX
#define TRIMPREFIX
#endif

/* Stream used by panic/todo/unreachable output. */
#ifndef CROMA_OUTFILE
#define CROMA_OUTFILE stderr
#endif

/* panic, todo, unreachable macros: print and die */

#define _croma_panic_msg "PANIC "
#define CROMA_PANIC(fmt, ...)                                                         \
        do {                                                                          \
                fprintf(CROMA_OUTFILE, _croma_panic_msg "at %s:%d: ", __FILE__, __LINE__); \
                fprintf(CROMA_OUTFILE, fmt, ##__VA_ARGS__);                           \
                abort();                                                              \
        } while (0)

#define _croma_todo_msg "TODO "
#define CROMA_TODO(fmt, ...)                                                         \
        do {                                                                         \
                fprintf(CROMA_OUTFILE, _croma_todo_msg "at %s:%d: ", __FILE__, __LINE__); \
                fprintf(CROMA_OUTFILE, fmt, ##__VA_ARGS__);                          \
                abort();                                                             \
        } while (0)

#define _croma_unreachable_msg "UNREACHABLE "
#define CROMA_UNREACHABLE(fmt, ...)                                                        \
        do {                                                                               \
                fprintf(CROMA_OUTFILE, _croma_unreachable_msg "at %s:%d: ", __FILE__, __LINE__); \
                fprintf(CROMA_OUTFILE, fmt, ##__VA_ARGS__);                                \
                abort();                                                                   \
        } while (0)

#define croma_panic CROMA_PANIC
#define croma_todo CROMA_TODO
#define croma_unreachable CROMA_UNREACHABLE

/* Type-agnostic dynamic array */

#ifndef croma_DA_REALLOC
#define croma_DA_REALLOC(dest, size) realloc((dest), (size))
#endif

#ifndef croma_DA_MALLOC
#define croma_DA_MALLOC(size) malloc((size))
#endif

#define croma_AUTO_TYPE __auto_type

#define croma_DA(type)        \
        struct {              \
                int capacity; \
                int count;    \
                type *items;  \
        }

/* Add E to DA_PTR (pointer to croma_DA). */
#define croma_da_append(da_ptr, ...)                                                      \
        do {                                                                               \
                if ((da_ptr)->count >= (da_ptr)->capacity) {                               \
                        (da_ptr)->capacity = (da_ptr)->capacity ? (da_ptr)->capacity * 2 : 4; \
                        (da_ptr)->items = croma_DA_REALLOC(                                 \
                        (da_ptr)->items,                                                    \
                        sizeof(*((da_ptr)->items)) * (da_ptr)->capacity);                  \
                }                                                                          \
                (da_ptr)->items[(da_ptr)->count++] = (__VA_ARGS__);                        \
        } while (0)

/* Destroy croma_DA pointed by DA_PTR. */
#define croma_da_destroy(da_ptr)                                  \
        do {                                                      \
                if ((da_ptr) == 0) break;                         \
                if ((da_ptr)->items && (da_ptr)->capacity > 0) {  \
                        free((da_ptr)->items);                    \
                }                                                 \
                (da_ptr)->capacity = 0;                           \
                (da_ptr)->count = 0;                              \
                (da_ptr)->items = NULL;                           \
        } while (0)

/* Insert element E into DA_PTR at index I. */
#define croma_da_insert(da_ptr, e, i)                                              \
        do {                                                                       \
                croma_da_append((da_ptr), (__typeof__((e))) { 0 });               \
                memmove((da_ptr)->items + (i) + 1, (da_ptr)->items + (i),         \
                        ((da_ptr)->count - (i) - 1) * sizeof *((da_ptr)->items)); \
                (da_ptr)->items[(i)] = (e);                                        \
        } while (0)

#define croma_da_count(da_ptr) ((da_ptr)->count)
#define croma_da_index(da_elem_ptr, da_ptr) ((int) ((da_elem_ptr) - ((da_ptr)->items)))

/* Remove element at index I. */
#define croma_da_remove(da_ptr, i)                                           \
        do {                                                                 \
                if ((i) >= 0 && (i) < (da_ptr)->count) {                     \
                        --(da_ptr)->count;                                   \
                        memmove((da_ptr)->items + (i), (da_ptr)->items + (i) + 1, \
                                ((da_ptr)->count - (i)) * sizeof *(da_ptr)->items); \
                }                                                            \
        } while (0)

#define croma_for_da_each(_i_, da_ptr)                          \
        for (croma_AUTO_TYPE(_i_) = (da_ptr)->items;            \
             (int) ((_i_) - (da_ptr)->items) < (da_ptr)->count; \
             ++(_i_))

/* Duplicate struct and item buffer (element data copied byte-wise). */
#define croma_da_dup(da_ptr)                                                          \
        ({                                                                            \
                __auto_type cpy = *(da_ptr);                                          \
                cpy.items = croma_DA_MALLOC((da_ptr)->capacity * sizeof (da_ptr)->items[0]); \
                memcpy(cpy.items, (da_ptr)->items, (da_ptr)->capacity * sizeof (da_ptr)->items[0]); \
                cpy;                                                                  \
        })

/* Stack wrapper over croma_DA (LIFO). */
#define croma_SS croma_DA
#define croma_for_ss_each croma_for_da_each
#define croma_ss_top(s_ptr) ((s_ptr)->items[(s_ptr)->count - 1])
#define croma_ss_count(s_ptr) croma_da_count(s_ptr)
#define croma_ss_push(s_ptr, ...) croma_da_append((s_ptr), __VA_ARGS__)
#define croma_ss_pop(s_ptr)                      \
        do {                                     \
                if ((s_ptr)->count > 0) {        \
                        (s_ptr)->count -= 1;     \
                }                                \
        } while (0)

/* Queue wrapper over croma_DA (FIFO). */
#define croma_QQ croma_DA
#define croma_for_qq_each croma_for_da_each
#define croma_qq_top(q_ptr) ((q_ptr)->items[0])
#define croma_qq_count(q_ptr) croma_da_count(q_ptr)
#define croma_qq_push(q_ptr, ...) croma_da_append((q_ptr), __VA_ARGS__)
#define croma_qq_pop(q_ptr)                      \
        do {                                     \
                if ((q_ptr)->count > 0) {        \
                        croma_da_remove((q_ptr), 0); \
                }                                \
        } while (0)

#define croma_min(a, b) ({              \
        __auto_type _a = (a);           \
        __auto_type _b = (b);           \
        (_a) < (_b) ? (_a) : (_b);      \
})

#define croma_max(a, b) ({              \
        __auto_type _a = (a);           \
        __auto_type _b = (b);           \
        (_a) > (_b) ? (_a) : (_b);      \
})

#define croma_zero(x) memset(&(x), 0, sizeof((x)))

#define croma_kb(x) ((size_t) (x) << 10)
#define croma_mb(x) ((size_t) (x) << 20)
#define croma_gb(x) ((size_t) (x) << 30)
#define croma_tb(x) ((size_t) (x) << 40)

#define croma_bf(n) ((n) = 0u)
#define croma_bf_set(n, f) ((n) |= (f))
#define croma_bf_clr(n, f) ((n) &= ~(f))
#define croma_bf_has(n, f) (((n) & (f)) == (f))

#define croma_tostring(x) #x

#ifdef TRIMPREFIX
#define UNUSED croma_unused

#define panic croma_panic
#define todo croma_todo
#define unreachable croma_unreachable

#define AUTO_TYPE croma_AUTO_TYPE
#define DA croma_DA
#define da_append croma_da_append
#define da_destroy croma_da_destroy
#define da_insert croma_da_insert
#define da_count croma_da_count
#define da_index croma_da_index
#define da_remove croma_da_remove
#define for_da_each croma_for_da_each
#define da_dup croma_da_dup

#define SS croma_SS
#define for_ss_each croma_for_ss_each
#define ss_top croma_ss_top
#define ss_count croma_ss_count
#define ss_push croma_ss_push
#define ss_pop croma_ss_pop

#define QQ croma_QQ
#define for_qq_each croma_for_qq_each
#define qq_top croma_qq_top
#define qq_count croma_qq_count
#define qq_push croma_qq_push
#define qq_pop croma_qq_pop

#define MIN croma_min
#define MAX croma_max
#define ZERO croma_zero
#define KB croma_kb
#define MB croma_mb
#define GB croma_gb
#define TB croma_tb
#define BF croma_bf
#define BF_SET croma_bf_set
#define BF_CLR croma_bf_clr
#define BF_HAS croma_bf_has
#define TOSTRING croma_tostring
#endif
