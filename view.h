#ifndef _VIEW_H
#define _VIEW_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/// Printf format string for `view_t`
#define SV_PRINT_FMT "%.*s"
/// Printf format arguments for `view_t` to match the position of `SV_PRINT_FMT`
#define SV_PRINT_ARG(view) (int)(view).len, (view).ptr

typedef struct string_view {
	size_t len;
	char* ptr;
} view_t;

struct string_data;
typedef struct string_data* str_info_t;

struct string;

// Functions //

view_t s_get_str(str_info_t data) __attribute__((pure));
str_info_t s_get_info(view_t str) __attribute__((pure));

view_t sv_slice(view_t v, size_t s, size_t n);
size_t sv_split(view_t p, char on, view_t* restrict first, view_t* restrict second);

view_t sv_from_cstr(const char* p);
view_t sv_slice_cstr(const char* p, size_t n);

__attribute__((__gnu_inline__))
extern inline size_t sv_split_cstr(const char* p, char on, view_t* restrict first, view_t* restrict second)
{
	return sv_split(sv_from_cstr(p), on, first, second);
}

/// Copy `view` into nul-terminated `buffer`, up to `size` bytes. Return the number of bytes that would've been copied regardless of `sz` (incl. nul-terminator)
size_t sv_copy_to(size_t sz, char buffer[restrict static sz], view_t view);
char* sv_to_cstr(view_t view, char* restrict buffer, size_t n);

// sprintf-like
size_t sv_svnprintf(view_t* restrict out, size_t sz, view_t fmt, ...);
size_t sv_svprintf(view_t* restrict out, view_t fmt, ...);
size_t sv_sprintf(view_t* restrict out, const char* fmt, ...);
size_t sv_snprintf(view_t* restrict out, size_t sz, const char* fmt, ...);

__attribute__((__gnu_inline__, malloc))
extern inline char* sv_dupto_cstr(view_t view) 
{ return sv_to_cstr(view, (char*)0, 0); }

bool sv_eq_cstr(view_t a, const char* b);
__attribute__((__gnu_inline__))
extern inline bool sv_eq(view_t a, view_t b)
{
	return a.len == b.len && (__builtin_expect(a.ptr == b.ptr, false) || (&sv_eq)(a, b));
}

#endif /* _VIEW_H */
