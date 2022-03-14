#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <limits.h>

#include "view.h"

#define S_PTR_MASK (0xcafebeef00000000ul) /* XXX: Should this frob the high or end parts of the pointer? We'll go with high? */

#define $mixin __attribute__((__gnu_inline__, __always_inline__, __artificial__)) inline
#define $nullchk(ptr, or) ({ __auto_type _$p = (ptr); \
			__builtin_expect(_$p == NULL, false) ? ({ or; }),NULL : _$p; })

#define min(x, y) ({	__auto_type _$min_x = (x); \
			__auto_type _$min_y = (y); \
			_$min_x < _$min_y ? _$min_x : _$min_y; })

#define ptrdiff(x, y) ({ \
			__auto_type _$ptrdiff_x = (x); \
			__auto_type _$ptrdiff_y = (y); \
			(ptrdiff_t)((_$ptrdiff_x < _$ptrdiff_y) \
					? ((uintptr_t)_$ptrdiff_y) - ((uintptr_t)_$ptrdiff_x) \
					: ((uintptr_t)_$ptrdiff_x) - ((uintptr_t)_$ptrdiff_y)); \
			})



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
str_info_t s_get_info(view_t str) 
{
	return _s_data($nullchk(str.ptr, return NULL), true);
}

__attribute__((pure))
view_t s_get_str(str_info_t data) 
{ 
	return (view_t) {
		.len = data->len,
		.ptr = data->ptr,
	};
}

__attribute__((cold))
static size_t _sv_copy_manual(size_t sz, char buffer[restrict static sz], view_t view)
{
	size_t i=0;
	for(;i< (sz-1) && i<view.len;i++)
	{
		buffer[i] = view.ptr[i];
	}
	buffer[i] = 0;
	return view.len+1;
}

size_t sv_copy_to(size_t sz, char buffer[restrict static sz], view_t view)
{
	if(__builtin_expect(view.len <= INT_MAX, true))
		return (size_t)snprintf(buffer, sz, "%.*s", (int)view.len, view.ptr);
	else {
		// Manual implementation, for views longer than INT_MAX
		//XXX: TODO: Ensure return value follows the same construct.
		return _sv_copy_manual(sz, buffer, view);
	}
}

// String views //

inline view_t sv_from_cstr(const char* p)
{
	return (view_t) {
		.len = strlen(p),
		.ptr = (char*)p,
	};
}

inline view_t sv_slice_cstr(const char* p, size_t n)
{
	if(__builtin_constant_p(n) && !n ) return (view_t) { .len = 0, .ptr = (char*)p };
	else return (view_t) { .len = min(strlen(p), n), .ptr = (char*)p };
}

inline view_t sv_slice(view_t v, size_t s, size_t n)
{
	if(__builtin_expect((s = min(v.len, s)) == v.len,false)) return (view_t) { .len = 0, .ptr = v.ptr+v.len };
	else {
		return (view_t) {
			.len = n 
				? min(v.len - s, n)//min(v.len,n) - s 
				: v.len - s, //XXX: Test this, I think it's correct, but dunno...
			.ptr = v.ptr + s,
		};
	}
}

inline size_t sv_split(view_t p, char on, view_t* restrict first, view_t* restrict second)
{
	if(__builtin_constant_p(first) && __builtin_constant_p(second) && !first && !second) 
		return (size_t)(((uintptr_t) $nullchk(memchr(p.ptr, (int)on, p.len), return 0)) - (uintptr_t)p.ptr);
	else {
		char* start = memchr(p.ptr, (int)on, p.len); // `on` chr here
		if(__builtin_expect(start==NULL, false)) return 0;

		size_t diff = (size_t)ptrdiff(p.ptr, start);
		if(first) *first = (view_t) {
			.len = diff,
			.ptr = p.ptr,
		};
		if(second) *second = (view_t) {
			.len = (p.len - diff) + 1, //XXX: WARNING: this might be an off-by-1 error...
			.ptr = start + 1,
		};
		
		return diff;
	}
}

size_t sv_split_cstr(const char* p, char on, view_t* restrict first, view_t* restrict second)
{
	return sv_split(sv_from_cstr(p), on, first, second);
}

inline char* sv_to_cstr(view_t view, char* restrict buffer, size_t n)
{
	if(__builtin_constant_p(buffer) && !buffer) {
		if(__builtin_constant_p(n) && !n) return strndup(view.ptr, view.len);
		else return strndup(view.ptr, n ? min(n, view.len) : view.len);
	} else {
		if(__builtin_constant_p(n) && !n) {
			return buffer;
		} else {
			sv_copy_to(n, buffer, view);
			return buffer;
		}
	}
}

__attribute__((malloc))
char* sv_dupto_cstr(view_t view)
{
	return sv_to_cstr(view, NULL, 0);
}

bool sv_eq_cstr(view_t a, const char* b)
{
	return strncmp(a.ptr, b, a.len) == 0 && strlen(b) == a.len;
}

bool sv_eq(view_t a, view_t b)
{
	return a.len == b.len && (__builtin_expect(a.ptr == b.ptr, false) || memcmp(a.ptr, b.ptr, a.len));
}




