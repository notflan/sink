#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "view.h"

#include "args.h"

#define _PASTE_(x, y) x ## y
#define _PASTE(x, y) _PASTE_(x, y)

#define _box_name(nm) _PASTE(_box_, _PASTE(nm, __LINE__))
#define $box(expr) ({  __typeof((expr))* _box_name(p) = aligned_alloc(_Alignof(__typeof((expr))), sizeof(__typeof((expr)))); \
			if(__builtin_expect(_box_name(p) != NULL, true)) *_box_name(p) = (expr); \
			_box_name(p); })

#define $box_t(T, expr) ({ T* _box_name(pT) = aligned_alloc(_Alignof(T), sizeof(T)); \
				if(__builtin_expect(_box_name(pT) != NULL, true)) *_box_name(pT) = (expr); \
			_box_name(pT); })

typedef __typeof( *((pargs_t*)0)->fd_pass ) fd_pass_t;

const pargs_t* parsed_args = NULL;
static const pargs_t default_args[1] = { A_DEFAULT_ARGS }; //A_DEFAULT_ARGS_P;

__attribute__((__gnu_inline__, __always_inline__))
static inline size_t _a_ptr_list_len(void* const* restrict arr)
{
	register size_t n=0;
	while(*arr++) n+=1;
	return n;
}

inline static void* _a_clone__string(const void* i) { return __builtin_expect(i!=NULL, true) ? strdup(i) : NULL; }
inline static void** _a_clone_ptr_list(void* const arr[static 1], void* (*clone_elem)(const void*), size_t* restrict _n)
{
	size_t len;
	if(__builtin_constant_p(_n) && _n) *_n = len = _a_ptr_list_len(arr);
	else {
		len = _a_ptr_list_len(arr);
		if(_n) *_n = len;
	}

	void* *array = aligned_alloc(_Alignof(void*), sizeof(void*)*(len+1));
	if(__builtin_expect(array!=NULL, true)) {
		if(__builtin_constant_p(clone_elem) && !clone_elem)
			memcpy(array, arr, sizeof(void*) * len);
		else if(clone_elem) {
			//void* *restrict out = array;
			while(len --> 0) {
			//	 *out++ = clone_elem(*arr++);
				array[len] = clone_elem(arr[len]);
			}
		} else memcpy(array, arr, sizeof(void*) * len);
		array[len] = NULL; // NULL-terminate ptr list
	}
	return array;
}
static char** _a_clone_string_list(char* const arr[static 1], size_t* restrict n)
{
	return (char**)_a_clone_ptr_list((void* const*)arr, _a_clone__string, n);
}

inline static void _a_free_ptr_list(void* arr[static restrict 1], void (*free_elem)(void*), size_t* restrict _n)
{
	if(__builtin_constant_p(_n) && __builtin_constant_p(free_elem) && !_n && !free_elem) {
		free(arr);
		return;
	} else {
		size_t n =0;
		void* restrict _to_free = arr;

		void* cur;
		if(__builtin_constant_p(free_elem) && free_elem) {
			if( __builtin_constant_p(_n) && !_n)
				while( (cur = *arr++) ) free_elem(cur);
			else
				while( (cur = *arr++) )
				{
					free_elem(cur); n+=1;
				}
		} else if(!free_elem)
			while( (cur = *arr++) ) n+=1;
		else	while( (cur = *arr++) )
			{
				free_elem(cur); n+=1;
			}
	
		free(_to_free);

		if(_n) *_n = n;
	}
}
static void _a_free_string_list(char* *restrict arr/*[static restrict 1]*/, size_t* restrict n)
{
	_a_free_ptr_list((void**restrict)arr, free, n);
}

static fd_pass_t* _a_alloc_fd_pass(size_t n)
{
	fd_pass_t* pass = aligned_alloc(_Alignof(fd_pass_t), sizeof(fd_pass_t) + (sizeof(int)*n));
	if(__builtin_expect(pass!=NULL,true)) {
		pass->n = n;
		//memset(pass->fds, 0, sizeof(int) * n);
	}
	return pass;
}

void* a_pass_fds(size_t n, ...)
{
	va_list va;
	va_start(va, n);
	fd_pass_t* pass;
	if(!n) {
		pass = A_FD_PASS_NONE;
		goto _end;
	}
	pass = _a_alloc_fd_pass(n);
	if(__builtin_expect(pass!=NULL, true)) {
		int* fds = pass->fds;
		while ( n --> 0 ) {
			int fd = va_arg(va, int);
			*fds++ = fd;
		}
	}
_end:
	va_end(va);
	return pass;
}

inline void* a_pass_fds_a(size_t n, int ar[const restrict n])
{
	if(!n) return A_FD_PASS_NONE;
	fd_pass_t* pass = _a_alloc_fd_pass(n);
	if(__builtin_expect(pass!=NULL, true))
		memcpy(pass->fds, ar, n * sizeof(int));
	return pass;
}

const pargs_t* a_get_program_args() {
	return parsed_args ?: default_args; 
}

bool a_is_parsed() { return parsed_args != NULL; }

bool a_parse_into(union arg_parse_result *restrict parsed, int *restrict argc, char** *restrict p_argv, char** *restrict p_envp)
{
	//TODO
	parsed->err = (struct arg_parse_error) { .kind = A_PF_UNKNOWN, ._data = { .none = {} } };
	return false;
}

const struct arg_parse_error* a_parse(int *restrict argc, char** *restrict p_argv, char** *restrict p_envp)
{
	/*XXX: Probably not needed... _Thread_local*/ 
	static union arg_parse_result glob_parsed_args_r;

	if(a_parse_into(&glob_parsed_args_r, argc, p_argv, p_envp))
		parsed_args = &glob_parsed_args_r.ok; // A_P_OK
	else
		return &glob_parsed_args_r.err;
	return NULL;
}

pargs_t* a_clone_args(const pargs_t* args)
{
	if(__builtin_expect(args!=NULL, true)) {
		pargs_t clone;

		/* replace */ clone.replace = args->replace;

		/* target */ {
			const char* target_p = args->target;
			clone.target = target_p ? strdup(target_p) : NULL;
		};

		/* fd_pass */ {
			__auto_type fd_pass_p = args->fd_pass;
			switch((uintptr_t)fd_pass_p) {
				case ((uintptr_t)A_FD_PASS_ALL): 
				case ((uintptr_t)A_FD_PASS_NONE):
					clone.fd_pass = fd_pass_p;
					if(0)
				default: {
					// Clone fd_pass_p.
					clone.fd_pass = a_pass_fds_a(fd_pass_p->n, fd_pass_p->fds);
				} break;
			}
		};

		/* env_pass */ {
			char** env_pass_p = args->env_pass;
			switch((uintptr_t)env_pass_p) {
				case (uintptr_t)A_ENV_PASS_ALL:
				case (uintptr_t)A_ENV_PASS_NONE:
					clone.env_pass = env_pass_p;
					if(0)
				default: {
					// Clone envp deeply.
					clone.env_pass = _a_clone_string_list(env_pass_p, NULL);
					
				} break;
			}
		};

		return $box(clone);
	} else return NULL;
}

inline void a_free_args(const pargs_t* args)
{
	if(__builtin_expect(args==NULL, false)) return;

	/* target */ {
		char* target_p = args->target;
		if(target_p) free(target_p);
	};

	/* fd_pass */ {
		__auto_type fd_pass_p = args->fd_pass;
		if(fd_pass_p != A_FD_PASS_NONE
			&& fd_pass_p != A_FD_PASS_ALL) free(fd_pass_p);
	};

	/* env_pass */ {
		char** env_pass_p = args->env_pass;
		switch((uintptr_t)env_pass_p) {
			case (uintptr_t)A_ENV_PASS_ALL:
			case (uintptr_t)A_ENV_PASS_NONE:
				if(0)
			default: {
				// Free each string in `env_pass_p`, then free `env_pass_p` itself
				_a_free_string_list(env_pass_p, NULL);
			} break;
		}
	};
}

void a_free(pargs_t* restrict args)
{
	if(__builtin_expect(args!=NULL, true)) {
		a_free_args(args);
		free(args);
	}
}

struct arg_parse_error* a_clone_error(const struct arg_parse_error* error)
{
	/* no-op, for now */
	if(__builtin_expect(error != NULL, true)) {
		return $box_t(struct arg_parse_error, *error);
	}
	else return NULL;
}

void a_free_error(const struct arg_parse_error* error)
{
	(void)error; // no-op, for now;
}

__attribute__((destructor))
static void _a__free_globals()
{
	a_free_args(parsed_args);
}
