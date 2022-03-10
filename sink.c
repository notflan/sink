#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

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
#else
	if(UNLIKELY(close(r_stderr) != 0)) {
		perror("failed to close stderr");
		
#endif
		return 3;
	}
	return 0;
}

int main(void)
{
	int null = open("/dev/null", O_RDWR);
	if(UNLIKELY(null<0)) {
		perror("failed to open /dev/null");
		return 1;
	}
	register int rc = dupall(null);
	if(LIKELY(rc)) close(null);
	return rc;
}

