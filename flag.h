/* * Copyright (c) 2026 Hugo Coto Florez
 *
 * This work is licensed under the Creative Commons Attribution 4.0
 * International License. To view a copy of this license, visit
 * http://creativecommons.org/licenses/by/4.0/
 *
 * SPDX-License-Identifier: CC-BY-4.0
 */

/* +---------------------------------------------------+
 * | Github: https://github.com/hugoocoto/flag.h       |
 * +---------------------------------------------------+
 * | Contributors:                                     |
 * | Hugo Coto Florez                                  |
 * +---------------------------------------------------+
 * */

/* Quick start / notes:
 *
 * 1. #include "flag.h"
 *
 * 2. flag_program([args]) -- optional
 *      args: zero or more of
 *      .name="", .help="", .positionals=flag_list("",""),
 *
 * 3. flag_add(char** var, [args]) -- one per flag
 *      var: address of a char pointer that is going to store the flag value
 *      args: zero or more of
 *      .opt="",  .abbr="", .help="", .nargs=0/1, .defaults="", .required=0/1,
 *
 * 4. if (flag_parse(&argc, &argv)) { flag_show_help(STDOUT_FILENO); exit(1); }
 * 5. flag_free()
 *
 * argc and argv are modified, the flags and their values are deleted. So the
 * final argc is 1 (program name) + non-flag count.
 *
 * for the flag -f that expects a parameter, you can use either -f 1 or -f=1.
 *
 * The values are (char*)1 or (char*)0 for boolean flags; and a heap-allocated
 * copy of the argument if the flags expects an arg, or NULL if not set. The
 * pointers are set to 0/NULL by default, you don't have to care about this.
 *
 * This library api uses optional-like function params. You have to specify it
 * as a struct field (.name=value,).
 *
 * You have to increment by hand the number of flags supported, to avoid
 * reallocations. This is not optimal, i know, but it works fine.
 *
 * Check the example.
 */

/* Example:
 *
 * #include "flag.h"
 *
 * int
 * main(int argc, char **argv)
 * {
 *         char *b;
 *         char *foo;
 *
 *         // Optional. Set program info.
 *         flag_program(.help = "flag.h by Hugo Coto", .positionals = flag_list("pos1", "pos2"));
 *
 *         // Register flags. The first argument is a pointer that should be set to
 *         // the constant string with the argument. If the flag is not set, it
 *         // would be set to NULL. See flag_opts
 *         flag_add(&foo, "--foo", "-f", .defaults = "nothing", .help = "foo flag", .nargs = 1);
 *         flag_add(&b, "--b", .required = 1, .help = "required flag");
 *
 *         // Check if all the required flags are set. Check if argc is at least
 *         // the same as the number of positionals. Return 0 if succeed.
 *         if (flag_parse(&argc, &argv)) {
 *                 // Print help. The flags --help, -h and -help are set by default
 *                 flag_show_help(STDOUT_FILENO);
 *                 exit(1);
 *         }
 *
 *         printf("foo = %s\n", foo);
 *
 *         flag_free();
 *
 *         return 0;
 * }
 *
 * Note: This example is the same as in ./test.c, if for some reason I forgot to
 * update it here, you can check it there.
 *
 * Output of the previous program with no args
 *
 * ./flag
 * Flag error: Required flag --b not set!
 * Flag error: Positional argument pos1 not provided!
 * Flag error: Positional argument pos2 not provided!
 *
 * usage: ./flag [-h] [-f F] --b pos1 pos2
 *
 * flag.h by Hugo Coto
 *
 * options:
 *  --help, -h     Show this help
 *  --foo, -f F    foo flag (default: nothing)
 *  --b    required flag
 */

#ifndef FLAG_H_
#define FLAG_H_

#ifdef __cplusplus
extern "C" {
#endif

#if _POSIX_C_SOURCE < 200809L
#define _POSIX_C_SOURCE 200809L
#endif

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define flag_list(x, ...) (char *[]){ x, ##__VA_ARGS__, 0 }

#define MAX_FLAG_COUNT 4

struct flag_opts {
        const char *opt;        // Flag (--help)
        const char *abbr;       // Flag abbreviation (-h)
        const char *help;       // Help message for the flag
        const char *defaults;   // Default value as string (default is a keyword)
        const char **var;       // Stores the pointer to the variable where the value should be set
        int nargs;              // Number of args to catch (max 1)
        int required;           // Set to 1 if the flag must be set
        signed char _need_free; // Asigned by flag.h
};

static struct program_opts {
        const char *name;         // program name. Used in the help message
        const char *help;         // program help. Used in the help message
        const char **positionals; // possitional arguments (check that argc -1 >= len(it))
} flag_prog = { 0 };


static struct {
        int count;
        struct flag_opts flags[MAX_FLAG_COUNT];
} flag_flags = {
        .count = 1,
        .flags = { (struct flag_opts) {
        .opt = "--help",
        .abbr = "-h",
        .help = "Show this help",
        } }
};

#define flag_add(var, ...) __flag_add(var, (struct flag_opts) { __VA_ARGS__ })
#define flag_program(...) __flag_program((struct program_opts) { __VA_ARGS__ })

static void
flag_show_help(int fileno)
{
        int i, j;

        dprintf(fileno, "\nusage: %s", flag_prog.name);
        if (flag_flags.count == 0) goto prog_help;

        for (i = 0; i < flag_flags.count; i++) {
                dprintf(fileno, flag_flags.flags[i].required ? " " : " [");
                if (flag_flags.flags[i].abbr)
                        dprintf(fileno, "%s", flag_flags.flags[i].abbr);
                else
                        dprintf(fileno, "%s", flag_flags.flags[i].opt);
                for (j = 0; j < flag_flags.flags[i].nargs; j++)
                        dprintf(fileno, " %c", toupper(flag_flags.flags[i].opt[2]));
                dprintf(fileno, flag_flags.flags[i].required ? "" : "]");
        }

        if (flag_prog.positionals == NULL) goto prog_help;
        for (i = 0; flag_prog.positionals[i]; i++) {
                dprintf(fileno, " %s", flag_prog.positionals[i]);
        }

prog_help:
        dprintf(fileno, "\n\n");
        if (flag_prog.help) dprintf(fileno, "%s\n\n", flag_prog.help);
        if (flag_flags.count == 0) return;

        dprintf(fileno, "options:\n");
        for (i = 0; i < flag_flags.count; i++) {
                dprintf(fileno, " ");
                if (flag_flags.flags[i].opt)
                        dprintf(fileno, "%s", flag_flags.flags[i].opt);
                if (flag_flags.flags[i].abbr)
                        dprintf(fileno, ", %s", flag_flags.flags[i].abbr);
                for (j = 0; j < flag_flags.flags[i].nargs; j++)
                        dprintf(fileno, " %c", toupper(flag_flags.flags[i].opt[2]));
                dprintf(fileno, " \t");
                if (flag_flags.flags[i].help)
                        dprintf(fileno, "%s", flag_flags.flags[i].help);
                if (flag_flags.flags[i].defaults)
                        dprintf(fileno, " (default: %s)", flag_flags.flags[i].defaults);
                dprintf(fileno, "\n");
        }
        dprintf(fileno, "\n");
}

static void
__flag_add(const char **var, struct flag_opts opts)
{
        if (flag_flags.count == MAX_FLAG_COUNT) {
                fprintf(stderr, "Flag error: Max flag count reached!"
                                " Change it in " __FILE__ "\n");
                exit(3);
        }
        if ((opts.var = var)) *opts.var = NULL;
        flag_flags.flags[flag_flags.count] = opts;
        ++flag_flags.count;
}

static void
__flag_program(struct program_opts opts)
{
        flag_prog = opts;
}

static void
__flag_pop_arg(int *argc, char ***argv, int *i)
{
        if (*i + 1 < *argc) {
                memmove(&(*argv)[*i], &(*argv)[*i + 1], (*argc - *i - 1) * sizeof(char *));
                --*i;
        }
        --*argc;
}

static int
flag_parse(int *argc, char ***argv)
{
        struct flag_opts *fopt;
        int i, j;
        int has_error = 0;

        if (!flag_prog.name || !*flag_prog.name) flag_prog.name = **argv;

        for (i = 0; i < *argc; i++) {
                if (strcmp((*argv)[i], "-h") == 0 ||
                    strcmp((*argv)[i], "-help") == 0 ||
                    strcmp((*argv)[i], "--help") == 0) {
                        __flag_pop_arg(argc, argv, &i);
                        flag_show_help(STDOUT_FILENO);
                        exit(0);
                }
        }

        for (i = 0; i < *argc; i++) {
                for (j = 0; j < flag_flags.count; j++) {
                        fopt = flag_flags.flags + j;
                        if (!fopt->var) continue;

                        int o = fopt->opt && *fopt->opt &&
                                !strncmp(fopt->opt, (*argv)[i], strlen(fopt->opt)) &&
                                ((*argv)[i][strlen(fopt->opt)] == 0 ||
                                 (*argv)[i][strlen(fopt->opt)] == '=');
                        int a = fopt->abbr && *fopt->abbr &&
                                !strncmp(fopt->abbr, (*argv)[i], strlen(fopt->abbr)) &&
                                ((*argv)[i][strlen(fopt->abbr)] == 0 ||
                                 (*argv)[i][strlen(fopt->abbr)] == '=');

                        /* var already set or name not match */
                        if (*fopt->var || (!o && !a)) continue;

                        if (fopt->nargs > 0) {
                                if (fopt->nargs > 1) {
                                        fprintf(stderr, "Flag error: Unsupported nargs > 1\n");
                                        return 1;
                                }
                                if ((o && (*argv)[i][strlen(fopt->opt)] == '=') ||
                                    (a && (*argv)[i][strlen(fopt->abbr)] == '=')) {
                                        *fopt->var = strdup(strchr((*argv)[i], '=') + 1);
                                        fopt->_need_free = 1;
                                } else if (*argc <= i + 1) {
                                        fprintf(stderr, "Flag error: OOB when reading value for `%s`\n", fopt->abbr ?: fopt->opt);
                                        return 1;
                                } else {
                                        ++i;
                                        *fopt->var = strdup((*argv)[i]);
                                        fopt->_need_free = 1;
                                        __flag_pop_arg(argc, argv, &i);
                                }
                        } else {
                                *fopt->var = (char *) 1;
                        }
                        __flag_pop_arg(argc, argv, &i);
                }
        }

        for (j = 0; j < flag_flags.count; j++) {
                fopt = flag_flags.flags + j;
                if (fopt->var == NULL || *fopt->var != NULL) continue;
                if (fopt->defaults) *fopt->var = fopt->defaults;
                if (fopt->required && *fopt->var == NULL) {
                        fprintf(stderr, "Flag error: Required flag %s not set!\n",
                                fopt->opt  ?:
                                fopt->abbr ?:
                                             "??");
                        has_error = 1;
                }
        }

        if (flag_prog.positionals == NULL) return 0;
        for (j = 0; flag_prog.positionals[j]; j++) {
                if (j >= *argc - 1) {
                        fprintf(stderr, "Flag error: Positional argument %s not provided!\n",
                                flag_prog.positionals[j]);
                        has_error = 1;
                }
        }

        return has_error;
}

static void
flag_free()
{
        struct flag_opts *fopt;
        for (int j = 0; j < flag_flags.count; j++) {
                fopt = flag_flags.flags + j;
                if (fopt->var == NULL) continue;
                if (!fopt->_need_free) continue;
                free((void *) *fopt->var);
        }
}

#ifdef __cplusplus
}
#endif

#endif // !FLAG_H_
