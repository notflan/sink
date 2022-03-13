
#include "args.h"

const pargs_t* parsed_args = NULL;
static const pargs_t default_args[1] = { A_DEFAULT_ARGS }; //A_DEFAULT_ARGS_P;

const pargs_t* a_get_program_args() {
	return parsed_args ?: default_args; 
}

bool a_is_parsed() { return parsed_args != NULL; }

enum arg_parse_result a_parse_into(pargs_t *restrict parsed, int *restrict argc, char** *restrict p_argv, char** *restrict p_envp)
{
	//TODO
	return A_PF_UNKNOWN;
}

enum arg_parse_result a_parse(int *restrict argc, char** *restrict p_argv, char** *restrict p_envp)
{
	_Thread_local static pargs_t glob_parsed_args;
	enum arg_parse_result r = a_parse_into(&glob_parsed_args, argc, p_argv, p_envp);

	if(!r) parsed_args = &glob_parsed_args; // A_P_OK
	return r;
}
