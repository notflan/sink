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

static int r_parent_stderr = r_stderr;
#define b_stderr r_parent_stderr

#define LIKELY(e) __builtin_expect((e) != 0, 1)
#define UNLIKELY(e) __builtin_expect((e) != 0, 0)

/// Private dup (F_DUPFD_CLOEXEC)
static inline int pdup(int fd)
{
	return fcntl(fd, F_DUPFD_CLOEXEC, fd);
}

static inline int pfd(int fd)
{
	return fcntl(fd, FD_CLOEXEC);
}

/// Save r_stderr -> b_stderr. and replace the `stderr` stream fd with this CLOEXEC'd dup of 
static int save_stderr()
{
	/*if(UNLIKELY( (b_stderr = dup(r_stderr)) < 0 )) {
		perror("failed to dup() parent stderr");
		return 0;
	}*/
	fflush(stderr);
	if(UNLIKELY( (b_stderr = pdup(r_stderr)) < 0 )) {
		perror("failed to pdup(stderr)");
		return 0;
	}
	int sfd = fileno(stderr);
	if(UNLIKELY( dup2(b_stderr, sfd = fileno(stderr)) < 0 )) {
		perror("failed to dup2() to stderr");
		return 0;
	} else if(UNLIKELY( pfd(sfd) < 0 )) return (perror("failed to (redundantly?) FD_CLOEXEC b_stderrr"),0); // TODO: XXX: Does dup2() copy the CLOEXEC flag? Find out, and it it does, remove this line.

	return 1;
}

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

	if(UNLIKELY(!save_stderr())) {
		return 3;
	}
#if defined(REPLACE_STDERR) && !defined(DEBUG) /* XXX: We may be removing this section soon, for now just leave it though */
	close(b_stderr);
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
#if defined(DEBUG) || !defined(REPLACE_STDERR)
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

#ifdef DEBUG
static inline size_t count_list(const void*const* p)
{
	size_t n=0;
	while(*p++) n+=1;
	return n;
}
static void print_debug_info(int argc, char* const* argv, char* const* envp)
{
	fprintf(stderr, "[DEBUG BUILD]\n");
#ifndef DEBUG_IGNORE_SPLASH
	fprintf(stderr,  _PROJECT " v" _VERSION ": " _DESCRIPTION "\n");
	fprintf(stderr, " :: written by " _AUTHOR " with <3 (License " _LICENSE ")\n---\n");
#endif
	fprintf(stderr, "> program: %s (path lookup: "
#ifdef NO_SEARCH_PATH
							"false"
#else
							"true"
#endif
			")\n", argv ? (argv[1] ?: "(unbound)") : "<invalid>" );
	fprintf(stderr, "> argc (expected): %d\n", argc-1);
	fprintf(stderr, "> argv: %p (# %lu)\n", (const void*)(argv+1), (argv+1) ? count_list((const void*const*)(argv+1)) : 0);
	fprintf(stderr, "> environ: %p (# %lu)\n", (const void*)envp, LIKELY(envp) ? count_list((const void*const*)envp) : 0);
	fprintf(stderr, "---\n");

	fflush(stderr);
}
#else
#define print_debug_info(a,b,c) ((void)((void)(a), (void)(b), (void)(c)))
#endif

int main(int argc, char** argv, char** envp)
{
#ifdef NO_ENV
	(void)envp;
#define GET_ENV ((char*[]){NULL})
#else
#define GET_ENV envp
#endif

	print_debug_info(argc, argv, GET_ENV);

#define $execve(path) execve((path), argv+1, GET_ENV)

	int null = open("/dev/null", O_RDWR);
	if(UNLIKELY(null<0)) {
		perror("failed to open /dev/null");
		return 1;
	}
	register int rc = dupall(null);
	if(LIKELY(!rc)) close(null);
	else return rc;

#if defined(REPLACE_STDERR) && !defined(DEBUG)
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

