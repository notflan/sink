#ifndef _ARGS_H
#define _ARGS_H
#define _a_inline_proto __attribute__((__gnu_inline__, __always_inline__)) extern inline

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

enum arg_parse_error_kind {
	A_PF_UNKNOWN = -1,
	A_P_OK = 0,

	// TODO: specific errors... 
};

typedef struct arg_parsed {
	struct {
		bool in,out,err;
	} replace; // { 1, 1, 0 }

	char* target; // NULL -> default "/dev/null"

	struct {
		/*struct _Alignas(uint64_t) {
			int64_t n : 63;
			bool all  : 1;
		} __attribute__((packed, aligned(uint64_t)));*/
		size_t n;

		int fds[];
	}* restrict fd_pass; // A_FD_PASS_NONE

} pargs_t;

struct arg_parse_error {
	enum arg_parse_error_kind kind;
	union {
		struct{} none;
	} _data;
};
_Static_assert(sizeof(  ((struct arg_parse_error*)(0))->_data ) == 0, "Bad union ZST");

union arg_parse_result {
	struct arg_parsed ok;
	struct arg_parse_error err;
};

#define A_FD_PASS_ALL  ((void*)(~((uintptr_t)0ul)))
#define A_FD_PASS_NONE NULL

#define A_DEFAULT_ARGS ((pargs_t){ { true, true, false}, NULL, A_FD_PASS_NONE })
#define A_DEFAULT_ARGS_P ((pargs_t[1]) { A_DEFAULT_ARGS } )

extern const pargs_t* parsed_args;

/// Return parsed args or default args if no parsed args have been set.
_a_inline_proto const pargs_t* a_get_program_args()
{ 
	__attribute__((section(".text#"))) // Does this give any actual benefit?
	static const pargs_t default_args = A_DEFAULT_ARGS; 

	return parsed_args ?: &default_args;//A_DEFAULT_ARGS_P; 
}

/// Have the program's arguments been parsed?
_a_inline_proto bool a_is_parsed() 
{ return parsed_args != NULL; }

/// Create an `pargs_t.fd_pass` value for `n` fds.
void* a_pass_fds(size_t n, ...);
/// Create an `pargs_t.fd_pass` value for `n` fds from the array `ar`.
void* a_pass_fds_a(size_t n, int ar[const restrict n]);

bool a_parse_into(union arg_parse_result *restrict parsed, int *restrict argc, char** *restrict p_argv, char** *restrict p_envp);
const struct arg_parse_error* a_parse(int *restrict argc, char** *restrict p_argv, char** *restrict p_envp);

/// Clone a `pargs_t` struct. The returned struct pointer must be freed with `a_free()`.
pargs_t* a_clone_args(const pargs_t* args);

/// Clone a `struct arg_parse_error`. The returned struct pointer must be freed with `a_free_args()`, then with stdlib `free()` itself.
struct arg_parse_error* a_clone_error(const struct arg_parse_error* error);

/// Free the internals of a `pargs_t` struct.
void a_free_args(const pargs_t* args);

/// Free the internals of an `arg_parse_error` struct.
void a_free_error(const struct arg_parse_error* error);

/// Free a `malloc()` or `a_clone_args()`'d `pargs_t` struct (NOTE: do *not* pass the result of `a_get_program_args()` or `parsed_args` to this, use `a_free_args()` for those instead.
void a_free(pargs_t* restrict args);

#undef _a_inline_proto
#endif /* _ARGS_H */
