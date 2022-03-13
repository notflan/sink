#ifndef _ARGS_H
#define _ARGS_H
#define _a_inline_proto __attribute__((__gnu_inline__, __always_inline__)) extern inline

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

enum arg_parse_result {
	A_PF_UNKNOWN = -1,
	A_P_OK = 0,

	// TODO: specific errors... 
};

typedef struct {
	struct {
		bool in,out,err;
	} replace; // { 1, 1, 0 }

	const char* target; // NULL -> default "/dev/null"

	struct {
		/*struct _Alignas(uint64_t) {
			int64_t n : 63;
			bool all  : 1;
		} __attribute__((packed, aligned(uint64_t)));*/
		size_t n;

		int fds[];
	}* restrict fd_pass; // A_FD_PASS_NONE

} pargs_t;

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

enum arg_parse_result a_parse_into(pargs_t *restrict parsed, int *restrict argc, char** *restrict p_argv, char** *restrict p_envp);

_a_inline_proto // TODO: XXX: Is this worth it?
enum arg_parse_result a_parse(int *restrict argc, char** *restrict p_argv, char** *restrict p_envp)
{
	static pargs_t glob_parsed_args; //XXX: Wth is this an error?
	enum arg_parse_result r = a_parse_into(&glob_parsed_args, argc, p_argv, p_envp);

	if(!r) parsed_args = &glob_parsed_args; // A_P_OK
	return r;
}
#undef _a_inline_proto
#endif /* _ARGS_H */
