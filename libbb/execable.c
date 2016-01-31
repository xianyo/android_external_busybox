/* vi: set sw=4 ts=4: */
/*
 * Utility routines.
 *
 * Copyright (C) 2006 Gabriel Somlo <somlo at cmu.edu>
 *
 * Licensed under GPLv2 or later, see file LICENSE in this tarball for details.
 */

#include "libbb.h"

#include <alloca.h>
#include <stdarg.h>

/* check if path points to an executable file;
 * return 1 if found;
 * return 0 otherwise;
 */
int FAST_FUNC execable_file(const char *name)
{
	struct stat s;
	return (!access(name, X_OK) && !stat(name, &s) && S_ISREG(s.st_mode));
}

/* search (*PATHp) for an executable file;
 * return allocated string containing full path if found;
 *  PATHp points to the component after the one where it was found
 *  (or NULL),
 *  you may call find_execable again with this PATHp to continue
 *  (if it's not NULL).
 * return NULL otherwise; (PATHp is undefined)
 * in all cases (*PATHp) contents will be trashed (s/:/NUL/).
 */
char* FAST_FUNC find_execable(const char *filename, char **PATHp)
{
	char *p, *n;

	p = *PATHp;
	while (p) {
		n = strchr(p, ':');
		if (n)
			*n++ = '\0';
		if (*p != '\0') { /* it's not a PATH="foo::bar" situation */
			p = concat_path_file(p, filename);
			if (execable_file(p)) {
				*PATHp = n;
				return p;
			}
			free(p);
		}
		p = n;
	} /* on loop exit p == NULL */
	return p;
}

/* search $PATH for an executable file;
 * return 1 if found;
 * return 0 otherwise;
 */
int FAST_FUNC exists_execable(const char *filename)
{
	char *path = xstrdup(getenv("PATH"));
	char *tmp = path;
	char *ret = find_execable(filename, &tmp);
	free(path);
	if (ret) {
		free(ret);
		return 1;
	}
	return 0;
}

#if ENABLE_FEATURE_PREFER_APPLETS
int FAST_FUNC bb_execv_applet(const char *name, char *const argv[], char *const envp[])
{
	const char **path = bb_busybox_exec_paths;

	errno = ENOENT;

	if (find_applet_by_name(name) < 0)
		return -1;

	for (; *path; ++path)
		execve(*path, argv, envp);

	return -1;
}

/* just like the real execvp, but try to launch an applet named 'file' first
 */
int FAST_FUNC bb_execvp(const char *file, char *const argv[])
{
	int ret = bb_execv_applet(file, argv, environ);
	if (errno != ENOENT)
		return ret;

	return execvp(file, argv);
}

int FAST_FUNC bb_execlp(const char *file, const char *arg, ...)
{
#define INITIAL_ARGV_MAX 16
	size_t argv_max = INITIAL_ARGV_MAX;
	const char **argv = malloc(argv_max * sizeof (const char *));
	va_list args;
	unsigned int i = 0;
	int ret;

	va_start (args, arg);
	while (argv[i++] != NULL) {
		if (i == argv_max) {
			const char **nptr;
			argv_max *= 2;
			nptr = realloc (argv, argv_max * sizeof (const char *));
			if (nptr == NULL)
				return -1;
			argv = nptr;
		}

		argv[i] = va_arg (args, const char *);
	}
	va_end (args);

	ret = bb_execvp(file, (char *const *)argv);
	free(argv);

	return ret;
}
#endif
