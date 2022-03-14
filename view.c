#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "view.h"

#define S_PTR_MASK (0xcafebeef00000000ul) /* XXX: Should this frob the high or end parts of the pointer? We'll go with high? */

#define $mixin __attribute__((__gnu_inline__, __always_inline__, __artificial__)) inline
#define $nullchk(ptr, or) ({ __auto_type _$p = (ptr); \
			__builtin_expect(_$p == NULL, false) ? ({ or; }),NULL : _$p; })

typedef uint8_t byte_t;

__attribute__((packed))
struct string_metadata {
	bool is_owned	: 1; // was the string malloc()'d
	bool has_nul	: 1; // does this string have a nul terminator
};

struct string_data {
	struct string_metadata meta;

	uintptr_t ptr_frob;

	
	struct /*_Alignas(struct string_view)*/ {
		size_t len;
		char ptr[];
	} __attribute__((aligned(_Alignof(struct string_view)))); //__attribute__((align(struct string_view)));
};

#define _s_frob_pointer_(p, f) ((/*__typeof((p))*/void*) (((uintptr_t)(p)) ^ ((uintptr_t)(f))))
__attribute__((const))
$mixin static void* _s_frob_pointer(void* p) 
{
	return _s_frob_pointer_(p, S_PTR_MASK); 
}
#define _s_frob_pointer(p) _s_frob_pointer_((p), S_PTR_MASK)

#define $frob(v, w) _s_frob_pointer_((v), (w))

static inline struct string* _s_frob(struct string_data* restrict d) {
	return (struct string*) _s_frob_pointer(d->ptr);
}
static inline struct string_data* _s_unfrob(struct string *restrict ptr) { 
//	static const off64_t O_this = offsetof(struct string_data, ptr);

	struct string_data* restrict base = (void*)((byte_t*)_s_frob_pointer(ptr)) - offsetof(struct string_data, ptr);

	uintptr_t frob = base->ptr_frob;
	if(frob != ((uintptr_t)_s_frob_pointer(base))) return NULL; //XXX: TODO: Is this valid? Probably, since struct string_data will only ever be heap-allocated.

	return base;
}

$mixin static char* _s_str(struct string_data* restrict data) { return data->ptr; }

$mixin extern struct string_data* _s_data(char* str, bool check) {
	struct string_data* sd = (struct string_data*)( ((byte_t*)str) - offsetof(struct string_data, ptr) );

	if(__builtin_constant_p(check) && !check) return sd;
	else if(check && sd->ptr_frob != ((uintptr_t)_s_frob_pointer(sd))) return NULL; 
	else return sd;
}
static struct string_data* _s_data(char* str, bool check) {
	static const size_t O_base = offsetof(struct string_data, ptr);
	struct string_data* sd = (struct string_data*)( ((byte_t*)str) - O_base );

	if(__builtin_constant_p(check) && !check) return sd;
	else if(check && sd->ptr_frob != ((uintptr_t)_s_frob_pointer(sd))) return NULL;
	else return sd;
}

__attribute__((pure))
str_info_t s_get_info(sv_t str) 
{
	return _s_data($nullchk(str.ptr, return NULL), true);
}

__attribute__((pure))
sv_t s_get_str(str_info_t data) 
{ 
	return (sv_t) {
		.len = data->len,
		.ptr = (unsigned char*)data->ptr,
	};
}
