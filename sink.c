#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <limits.h>
#include <errno.h>

#include "comp_features.h"

static const enum compiled_features compiled_features = FEATURE_FLAGS;

#define r_stdin 0
#define r_stdout 1
#define r_stderr 2

static int r_parent_stderr = r_stderr;
#define b_stderr r_parent_stderr

#define LIKELY(e) __builtin_expect((e) != 0, 1)
#define UNLIKELY(e) __builtin_expect((e) != 0, 0)

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

#ifdef DEBUG
#define dprintf(...) eprintf(__VA_ARGS__)
#else
//#pragma GCC diagnostic ignored "-Wunused
#define dprintf(...) ((void)0)
#endif

#ifdef REPLACE_STDERR
#define _replace_stderr
#else
#define _replace_stderr __attribute__((unused))
#endif

/// Private dup (F_DUPFD_CLOEXEC)
_replace_stderr
static inline int pdup(int fd)
{
	return fcntl(fd, F_DUPFD_CLOEXEC, fd);
}

__attribute__((unused)) // No longer needed
static inline int pfd(int fd)
{
	return fcntl(fd, FD_CLOEXEC);
}

__attribute__((pure))
_replace_stderr
static int fdreopen(FILE** stream, int with, const char mode[static 1])
{
	if(UNLIKELY( fclose(*stream) != 0 )) return (perror("failed to close stream to fdreopen"), 0);
	return (*stream = fdopen(with, mode)) != NULL;
}

/// Save r_stderr -> b_stderr. and replace the `stderr` stream with a new wrapper around the CLOEXEC'd `dup()` of `r_stderr`.
_replace_stderr
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
#if 0
	int sfd = fileno(stderr);
	if(UNLIKELY( dup2(b_stderr, sfd) < 0 )) {
		perror("failed to dup2() to stderr");
		return 0;
	} //else if(UNLIKELY( pfd(sfd) < 0 )) return (perror("failed to (redundantly?) FD_CLOEXEC b_stderrr"),0); // TODO: XXX: Does dup2() copy the CLOEXEC flag? Find out, and it it does, remove this line.
#else
	if(UNLIKELY( !fdreopen(&stderr, b_stderr, "wb") )) {
		perror("failed to reopen parent stderr with pdup()'d stream");
		return 0;
	}
#endif
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

	
#ifdef REPLACE_STDERR
	// Save parent's stderr to a private one
	if(UNLIKELY(!save_stderr())) {
		return 3;
	}

	// Replace the child's stderr with null
	if(UNLIKELY(dup2(from, r_stderr) < 0)) {
		perror("failed to dup2() r_stderr to sink");
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
			dprintf("Warning: normalised path item '%s' truncated (from %s/%s). Full path was longer than %lu bytes\n", fullpath, item, name, sz);
			errno = ENAMETOOLONG;
			continue;
		}
		dprintf(">> PATH search in `%s'... ", item);
		if(path_exists(fullpath)) {
			dprintf("FOUND\n");
			found = 1;
			break;
		} else dprintf("failed\n");
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
static void print_compiled_features()
{
#define _X "\t%s\n"
#define X(name) fprintf(stderr, _X, FEATURE_STRING(FEATURE_FLAGS, name))
	X(REPLACE_STDERR);
	X(NO_SEARCH_PATH);
	X(NO_ENV);
	X(DEBUG_IGNORE_SPLASH);
	fprintf(stderr, "Build mode: \n\t"
#if defined(DEBUG) && defined(RELEASE)
			"release (+debug paths)"
#elif defined(DEBUG)
			"debug"
#elif degined(RELEASE)
			"release"
#else
			"unknown"
#endif
			"\n");

#undef X
#undef _X
}
static void print_debug_info(int argc, char* const* argv, char* const* envp)
{
	fprintf(stderr, "[DEBUG BUILD]\n");
#if ! FEATURE_HAS_FLAG(DEBUG_IGNORE_SPLASH)
	fprintf(stderr,  _PROJECT " v" _VERSION ": " _DESCRIPTION "\n");
	fprintf(stderr, " :: written by " _AUTHOR " with <3 (License " _LICENSE ")\n---\n");
#endif
	fprintf(stderr, "> features:\n");
	print_compiled_features();

	fprintf(stderr, "> program: %s (path lookup: "
#if FEATURE_HAS_FLAG(NO_SEARCH_PATH)
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

		dprintf("Attempting to execve() direct path %s...\n", argv[1]);

		if(LIKELY(($execve(argv[1]) < 0)
			&& (err_not_found(err = errno))
			&& (path = getenv("PATH")))) {

			dprintf("Failed to exec raw pathname %s, lookup in PATH (%s)\n", argv[1], path);

			char fullpath[PATH_MAX+1];
			errno = 0;
			if(path_lookup(argv[1], fullpath, PATH_MAX, path)) {
				dprintf("Attempting to execve() to found path %s...\n", fullpath);
				$try_execve(fullpath);
			} else {
				eprintf("Error: failed to find %s in PATH\n", argv[1]);
				return errno ?: -1;
			}
		} else {
			if(UNLIKELY(!err)) err = errno;
			dprintf("execve() failed in unexpected way with code %d (ENOENT? %s, PATH? %s)\n", err, err_not_found(err) ? "yes" : "no", path ?: "<null>");

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

