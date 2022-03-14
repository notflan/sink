#ifndef _VIEW_H
#define _VIEW_H

#include <stdint.h>
#include <stddef.h>

typedef struct string_view {
	size_t len;
	unsigned char* ptr;
} sv_t;

struct string_data;
typedef struct string_data* str_info_t;

struct string;

// Functions //

sv_t s_get_str(str_info_t data) __attribute__((pure));
str_info_t s_get_info(sv_t str) __attribute__((pure));

#endif /* _VIEW_H */
