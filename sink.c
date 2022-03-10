#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#define r_stdin 0
#define r_stdout 1
#define r_stderr 2

#define LIKELY(e) __builtin_expect((e) != 0, 1)
#define UNLIKELY(e) __builtin_expect((e) != 0, 0)

static inline int dupall(int from)
{
	if(UNLIKELY(dup2(from, r_stdin) < 0)) {
		perror("failed to dup2() stdin to sink");
		return 1;
	}
	if(UNLIKELY(dup2(from, r_stdout) < 0)) {
		perror("failed to dup2() stdout to sink");
		return 2;
	}
#ifdef REPLACE_STDERR
	if(UNLIKELY(dup2(from, r_stderr) < 0)) {
		perror("failed to dup2() stderr to sink");
/*#else
	if(UNLIKELY(close(r_stderr) != 0)) {
		perror("failed to close stderr");
		*/
		return 3;
	}
#endif
	return 0;
}

#ifndef NO_SEARCH_PATH
inline static int err_not_found(int er) {
	return er == ENOENT;
}

static inline int path_exists(const char fname[restrict static 1])
{
	return access(fname, F_OK | X_OK) != -1;
}

static int path_lookup(size_t sz; const char name[restrict static 1], char fullpath[restrict sz], size_t sz, const char envpath[restrict static 1])
{
	char* paths = strdup(envpath);
	char* _tmpfree = paths;
	int found = 0;
	const char* item;

	while((item = strsep(&paths, ":")) != NULL) {
		if(UNLIKELY( ((size_t)snprintf(fullpath, sz, "%s/%s", item, name)) >= sz )) {
#if defined(DEBUG) && !defined(REPLACE_STDERR)
			fprintf(stderr, "Warning: normalised path item '%s' truncated (from %s/%s). Full path was longer than %lu bytes\n", fullpath, item, name, sz);
#endif
			errno = ENAMETOOLONG;
			continue;
		}
		if(path_exists(fullpath)) {
			found = 1;
			break;
		}
	}

	free(_tmpfree);
	return found;
}
#endif // !NO_SEARCH_PATH

int main(int argc, char** argv, char** envp)
{
	(void)argc;
#ifdef NO_ENV
	(void)envp;
#define GET_ENV ((char*[]){NULL})
#else
#define GET_ENV envp
#endif

#define $execve(path) execve((path), argv+1, GET_ENV)

	int null = open("/dev/null", O_RDWR);
	if(UNLIKELY(null<0)) {
		perror("failed to open /dev/null");
		return 1;
	}
	register int rc = dupall(null);
	if(LIKELY(!rc)) close(null);
	else return rc;

#ifdef REPLACE_STDERR
#define perror(v) ((void)(v)) 
#pragma GCC diagnostic ignored "-Wunused-value"
#define eprintf(...) ((void)(__VA_ARGS__))
#else
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#endif


	if(argv[1]) {
#ifdef NO_SEARCH_PATH
		if(UNLIKELY($execve(argv[1]) < 0)) {
			perror("execve() failed");
			return errno;
		}
#else
#define $try_execve(path) do { if(UNLIKELY($execve((path)) < 0))  { perror("execve() failed"); return errno; } else __builtin_unreachable(); } while(0)
		const char* path = NULL;
		int err = 0;
		if(LIKELY(($execve(argv[1]) < 0)
			&& (err_not_found(err = errno))
			&& (path = getenv("PATH")))) {
			// Failed to exec raw pathname (not found), lookup in $PATH
			char fullpath[PATH_MAX+1];
			errno = 0;
			if(path_lookup(argv[1], fullpath, PATH_MAX, path)) $try_execve(fullpath);
			else {
				eprintf("Error: failed to find %s in PATH\n", argv[1]);
				return errno ?: -1;
			}
		} else {
			if(UNLIKELY(!err)) err = errno;

			if(err_not_found(err)) {
				perror(path 
					? "execve() failed. Program not found in PATH"
					: "execve() failed. No PATH set to look through");
			} else	perror("execve() failed");
			return err ?: -1;
		}
#endif
	}
	return 0;
}

