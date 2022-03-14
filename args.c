#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

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
					break;
				default: {
					// Clone fd_pass_p.
					clone.fd_pass = a_pass_fds_a(fd_pass_p->n, fd_pass_p->fds);
				} break;
			}
		};

		return $box(clone);
	} else return NULL;
}

inline void a_free_args(const pargs_t* args)
{
	if(__builtin_expect(args==NULL, false)) return;

	/* fd_pass */ {
		__auto_type fd_pass_p = args->fd_pass;
		if(fd_pass_p != A_FD_PASS_NONE
			&& fd_pass_p != A_FD_PASS_ALL) free(fd_pass_p);
	};

	/* target */ {
		char* target_p = args->target;
		if(target_p) free(target_p);
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
