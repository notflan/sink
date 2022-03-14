#ifndef _VIEW_H
#define _VIEW_H

#include <stdint.h>
#include <stddef.h>

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

/// Copy `view` into nul-terminated `buffer`, up to `size` bytes. Return the number of bytes that would've been copied regardless of `sz` (incl. nul-terminator)
size_t sv_copy_to(size_t sz, char buffer[restrict static sz], view_t view);

#endif /* _VIEW_H */
