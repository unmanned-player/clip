/* SPDX-License-Identifier: ISC */

#include <stddef.h>

#include <assert.h>

#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "clip.h"

#define ARG_REQD                        ((unsigned)0x01)
#define ARG_ANYK                        ((unsigned)0x02)

#define ANSI_END                        "\033[0m"
#define ANSI_PROG                       "\033[1m\033[1;37m"
#define ANSI_SUBTITLE                   "\033[2m\033[1;37m"
#define ANSI_CMD                        "\033[1;32m"
#define ANSI_OPT                        "\033[1;34m"
#define ANSI_ANY                        "\033[1;33m"
#define ANSI_ERR                        "\033[0;31m"

#define IS_OPT_END(opt) \
    ( \
        opt->a_short == 0 && \
        opt->a_long == NULL && \
        opt->tag == NULL && \
        opt->mode == 0 && \
        opt->help == NULL \
    )

#define IS_CMD_END(cmd) \
    ( \
        cmd->name == NULL && \
        cmd->opts == NULL \
    )

#define FLAGS_HAS_AUTO(f) \
    ( \
        (f & CLIP_FLAG_HELP) != 0 || \
        (f & CLIP_FLAG_VERSION) != 0 \
    )

#define IS_SHORT_OPT(v) \
    ((v)[0] == '-' && isalnum((v)[1]))

#define IS_LONG_OPT(v) \
    ((v)[0] == '-' && (v)[1] == '-' && isalnum((v)[2]))

#define IS_DOUBLE_DASH(v) \
    ((v)[0] == '-' && (v)[1] == '-' && (v)[2] == 0)

static const struct cli_opt def_help_base =
    CLI_OPT_SWITCH(
        'h',
        "help",
        "Show help message."
    );

static struct cli_opt def_version =
    CLI_OPT_SWITCH(
        'v',
        "version",
        "Show version and if available, copyright information."
    );

static const struct cli_opt def_help_cmds =
    CLI_OPT_SWITCH(
        'h',
        "help",
        "Show help message. If this option is used along with a sub-command, "
        "then a help message specific to that sub-command is shown."
    );

static void cli__puts(
    FILE *out,
    const char *colour,
    const char *pfx,
    const char *sfx,
    const char *str,
    size_t n)
{
    if (colour != NULL) fprintf(out, "%s", colour);
    if (pfx != NULL) fprintf(out, "%s", pfx);

    if (n != 0) {
        fprintf(out, "%.*s", (int)n, str);
    } else {
        fprintf(out, "%s", str);
    }

    if (sfx != NULL) fprintf(out, "%s", sfx);
    if (colour != NULL) fprintf(out, ANSI_END);
}

/**
 * Find an option marked by ::CLI_OPT_NARGS() macro
 */
static const struct cli_opt *cli__find_any(const struct cli_sub_cmd *cmd)
{
    const struct cli_opt *opt;

    if (cmd == NULL) {
        return NULL;
    }

    for (opt = cmd->opts; !IS_OPT_END(opt); opt++) {
        if ((opt->mode & ARG_ANYK) != 0) {
            return opt;
        }
    }

    return NULL;
}

/**
 * Simple, unintuitive text-wrap algorithm that chops text at spaces and new
 * lines at about 78 character per line.
 */
static void cli__put_text(FILE *out, const char *text)
{
    const char *p, *last, *sp, *nl;

    last = sp = p = NULL;
    nl = text;
    for (p = text; *p != 0; p++) {
        if (*p != ' ' && *p != '\t') {
            continue;
        }
        last = sp;
        sp = p;

        if ((size_t)(sp - nl) > 78) {
            fprintf(out, "  %.*s\n", (int)(last - nl), nl);
            nl = last + 1;
        }
    }

    if (p > nl) {
        fprintf(out, "  %s\n", nl);
    }
}

/**
 * Print a single command-line option.
 */
static void cli__put_opt(FILE *out, int is_ansi, const struct cli_opt *opt)
{
    if (opt->mode == ARG_ANYK) {
        if (is_ansi) fprintf(out, ANSI_ANY);
        fprintf(out, "%s...", opt->tag);
        if (is_ansi) fprintf(out, ANSI_END);
    } else {
        if (is_ansi) fprintf(out, ANSI_OPT);
        if (isalnum(opt->a_short)) {
            fprintf(out, "-%c", opt->a_short);
            if (opt->tag != NULL) {
                fprintf(out, " %s", opt->tag);
            }
            if (opt->a_long) {
                fprintf(out, ", ");
            }
        }
        if (opt->a_long) {
            fprintf(out, "--%s", opt->a_long);
            if (opt->tag != NULL) {
                fprintf(out, "=%s", opt->tag);
            }
        }
        if (is_ansi) fprintf(out, ANSI_END);
    }

    fputc('\n', out);

    /* Now print the help function in a specific way. */
    if (opt->help == NULL) {
        return;
    }

    cli__put_text(out, opt->help);
}

#if defined(_DEBUG) && !defined(NDEBUG)

#define _TEST(test, msg)                assert(((void)(msg), !(test)))

static void cmd_verify(const struct cli_sub_cmd *cmd)
{
    const struct cli_opt *opt;
    size_t anys;

    anys = 0;
    for (opt = cmd->opts; !IS_OPT_END(opt); opt++) {
        if ((opt->mode & ARG_REQD) != 0) {
            _TEST(
                opt->tag == NULL,
                "Option that needs value, doesn't specify a tag-name for it"
            );
        } else if ((opt->mode & ARG_ANYK) != 0) {
            _TEST(
               opt->a_short != 0 || opt->a_long != 0,
               "NARGS option must not have an option name, only tag"
            );
            _TEST(
                opt->tag == NULL || strlen(opt->tag) == 0,
                "NARGS option doesn't name a tag name"
            );
            anys++;
        } else {
            _TEST(
                opt->a_short == 0 && opt->a_long == NULL,
                "Switch option doesn't have a name"
            );
        }
    }
    _TEST(
        anys > 1,
        "Too many NARGS option defined"
    );
}
#endif

void cli_verify(struct clip *clip)
{
#if defined(_DEBUG) && !defined(NDEBUG)
    _TEST(clip == NULL, "`clip` is NULL");
    _TEST(clip->cb == NULL, "call-back is NULL");
    _TEST(
        clip->base == NULL && clip->cmds == NULL,
        "Must define some options or sub-commands"
    );
    _TEST(
        (clip->flags && CLIP_FLAG_VERSION) != 0 && clip->version == NULL,
        "Automatic version requested, but no version specified"
    );
    _TEST(
        clip->base != NULL && clip->base->name != NULL,
        "Base options cannot be a named sub-command"
    );
    if (clip->base != NULL) {
        cmd_verify(clip->base);
    }

    if (clip->cmds != NULL) {
        const struct cli_sub_cmd *cmd;

        for (cmd = clip->cmds; !IS_CMD_END(cmd); cmd++) {
            _TEST(
                cmd->name == NULL,
                "Sub-command doesn't have a name"
            );
            cmd_verify(cmd);
        }
    }
#else
    (void)clip;

    return;
#endif
}

static const struct cli_sub_cmd *cli__find_cmd(
    struct clip *clip,
    const char *name)
{
    size_t c_len, n_len;
    const struct cli_sub_cmd *cmd;

    if (clip->cmds == NULL) {
        return NULL;
    }

    n_len = strlen(name);
    for (cmd = clip->cmds; !IS_CMD_END(cmd); cmd++) {
        c_len = strlen(cmd->name);
        if (n_len == c_len && memcmp(cmd->name, name, n_len) == 0) {
            return cmd;
        }
    }

    return NULL;
}

static const struct cli_opt *cli__find_opt_0(
    const struct cli_sub_cmd *cmd,
    const char *str)
{
    const struct cli_opt *opt;
    size_t s_len, o_len;

    if (cmd == NULL || cmd->opts == NULL) {
        return NULL;
    }

    s_len = strlen(str);

    for (opt = cmd->opts; !IS_OPT_END(opt); opt++) {
        if ((opt->mode & ARG_ANYK) != 0) {
            continue;
        }

        if (s_len == 1 && str[0] == opt->a_short) {
            return opt;
        } else if (s_len > 1 && opt->a_long != NULL) {
            o_len = strlen(opt->a_long);
            if (s_len == o_len && memcmp(str, opt->a_long, s_len) == 0) {
                return opt;
            }
        }
    }

    return NULL;
}

static const struct cli_opt *cli__find_opt(
    const struct cli_sub_cmd **whence,
    struct clip *clip,
    const struct cli_sub_cmd *cmd,
    const char *str)
{
    const struct cli_opt *opt;

    *whence = cmd;
    /* Find first in live sub command */
    if ((opt = cli__find_opt_0(cmd, str)) == NULL && cmd != clip->base) {
        /* If not, find it in global/base */
        opt = cli__find_opt_0(clip->base, str);
        *whence = clip->base;
    }

    if (opt == NULL) {
        *whence = NULL;
    }
    return opt;
}

static void cli_bad_arg(
    FILE *out,
    unsigned flags,
    int tag,
    const char *pfx,
    const char *key)
{
    fprintf(out, "%s ", pfx);
    if ((flags & CLIP_FLAG_USE_ANSI) != 0) {
        fprintf(out, ANSI_ERR);
    }
    switch (tag) {
        case 3:
            fputc('@', out);
            break;
        case 2:
            fputc('-', out);
            /* fall-through */
        case 1:
            fputc('-', out);
            break;
    }
    fprintf(out, "%s", key);
    if ((flags & CLIP_FLAG_USE_ANSI) != 0) {
        fprintf(out, ANSI_END);
    }
    fputc('\n', out);
}

static int cli_do_file(struct clip *clip, const char *file, size_t n)
{
    FILE *f, *out;
    char buffer[CLIP_BUFFER_SIZE];
    int r;
    const struct cli_opt *opt;
    const struct cli_sub_cmd *cmd;

    out = (clip->out != NULL)? clip->out: stderr;
    sprintf(buffer, "%.*s", (int)n, file);

    if ((f = fopen(buffer, "r")) == NULL) {
        fprintf(out, "Arguments file '%s' could not be opened.", buffer);
        return CLIP_ERR_BAD_ARG;
    }

    r = 0;
    while(fgets(buffer, CLIP_BUFFER_SIZE, f) != NULL) {
        char *eq;
        char *key, *val;
        char *nl;
        size_t len;

#if defined(_WIN32) || \
    defined(__WIN32__) || \
    defined(__WINDOWS__) || \
    defined(MSDOS) || \
    defined(__DOS__) || \
    defined(__MSDOS__) || \
    defined(_MSDOS)
        nl = strchr(buffer, '\r');
        if (nl == NULL) {
            nl = strchr(buffer, '\n');
        }
#else
        nl = strchr(buffer, '\n');
#endif
        if (nl != NULL) {
            *nl = 0;
        }

        key = buffer;
        eq = strchr(buffer, '=');
        if (eq == NULL) {
            eq = strchr(buffer, ' ');
        }
        val = NULL;
        if (eq != NULL) {
            *eq = 0;
            val = eq + 1;
        }

        opt = cli__find_opt(&cmd, clip, clip->live, key);
        if (opt == NULL) {

            len = strlen(key);
            cli_bad_arg(
                out,
                clip->flags,
                len == 1? 1: 2,
                "Invalid option:",
                key
            );
            r = CLIP_ERR_BAD_ARG;
            goto done;
        }

        if ((r = clip->cb(clip, cmd, opt, val)) != 0) {
            r = CLIP_ERR_CB_FAIL;
            goto done;
        }
    }

done:
    fclose(f);
    return r;
}


int cli_summary(struct clip *clip, const struct cli_sub_cmd *cmd)
{
    FILE *out;
    const struct cli_opt *any;

    if (clip == NULL) {
        return CLIP_ERR_INVALID;
    }

    if (cmd == NULL) {
        cmd = clip->base;
    }

    out = (clip->out)? clip->out: stdout;
    any = cli__find_any(cmd);

    fprintf(out, "Usage: ");
    cli__puts(
        out,
        (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_PROG: NULL,
        NULL,
        NULL,
        clip->progname,
        0
    );

    if (cmd == clip->base && clip->cmds != NULL) {
        cli__puts(
            out,
            (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_CMD: NULL,
            NULL,
            NULL,
            " [COMMAND]",
            0
        );
    }
    if (cmd != NULL) {
        char cmd_name[CLIP_BUFFER_SIZE] = {0};

        cmd_name[0] = ' ';
        strcpy(&cmd_name[1], cmd->name);
        cli__puts(
            out,
            (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_OPT: NULL,
            cmd_name,
            NULL,
            " [OPTIONS]",
            0);
    }
    if (any != NULL) {
        cli__puts(
            out,
            (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_ANY: NULL,
            " ",
            "...",
            any->tag,
            0);
    }
    fprintf(out, "\n");

    if (clip->header != NULL) {
        fprintf(out, "%s\n", clip->header);
    }

    /* If there are sub-commands and this is base/default options, show
     * sub-commands too
     */
    if (cmd == clip->base && clip->cmds != NULL) {
        const struct cli_sub_cmd *sub;

        fprintf(out, "\nSub-commands:\n");
        for (sub = clip->cmds; sub->opts != NULL; sub++) {
            cli__puts(
                out,
                (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_CMD: NULL,
                "\t",
                "\n",
                sub->name,
                0
            );
        }
    }

    if (FLAGS_HAS_AUTO(clip->flags)) {
        cli__puts(
            out,
            (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_SUBTITLE: NULL,
            "\n",
            "\n",
            "Default Options:",
            0
        );
        if ((clip->flags & CLIP_FLAG_HELP) != 0) {
            if (cmd == clip->base && clip->cmds != NULL) {
                cli__put_opt(
                    out,
                    clip->flags & CLIP_FLAG_USE_ANSI,
                    &def_help_cmds
                );
            } else {
                cli__put_opt(
                    out,
                    clip->flags & CLIP_FLAG_USE_ANSI,
                    &def_help_base
                );
            }
        }

        if ((clip->flags & CLIP_FLAG_VERSION) != 0) {
            const struct cli_opt *ver;

            ver = cli__find_opt_0(clip->base, "v");
            if (ver != NULL) {
                def_version.a_short = 0;
            }

            cli__put_opt(
                out,
                clip->flags & CLIP_FLAG_USE_ANSI,
                &def_version
            );
        }
    }

    if (cmd != NULL) {
        const struct cli_opt *opt;

        cli__puts(
            out,
            (clip->flags & CLIP_FLAG_USE_ANSI) != 0? ANSI_SUBTITLE: NULL,
            "\n",
            "\n",
            (cmd == clip->base)? "Common options:": "Options:",
            0
        );
        for (opt = cmd->opts; !IS_OPT_END(opt); opt++) {
            if (opt->help == NULL) {
                continue;
            }
            cli__put_opt(out, clip->flags & CLIP_FLAG_USE_ANSI, opt);
        }
    }

    if (clip->footer != NULL) {
        fprintf(out, "\n%s\n", clip->footer);
    }

    return 0;
}

int cli_parse(struct clip *clip, int argc, char **argv)
{
    FILE *out;
    size_t len;
    const struct cli_sub_cmd *cmd;
    int i;
    int r;
    char *arg;
    const struct cli_opt *opt;

    if (clip == NULL) {
        return CLIP_ERR_INVALID;
    }
    if (clip->index != 0) {
        return CLIP_ERR_INVALID;
    }

    out = (clip->out != NULL)? clip->out: stderr;

    /* Primary index */
    clip->index = 0;

    if (argc > 1) {
        clip->index++;
    } else {
        /* No more arguments to parse, exit */
        return CLIP_ERR_OK;
    }

    /* We start at the base defaults */
    clip->live = clip->base;
    /* We are at sub-commands part */
    if (isalnum(argv[clip->index][0])) {
        /* 2 things can happen here:
         *  ->    This is a sub-command, so match for it.
         *  ->(*) Otherwise, if the base has NARGS, then feed this to it.
         *
         * We do the first one here, but let it roll into the massive loop
         * later for NARGS matching.
         */
        if (clip->cmds != NULL) {
            cmd = cli__find_cmd(clip, argv[clip->index]);
            if (cmd) {
                clip->live = cmd;
                clip->index++;
            }
        }
    }

    for (i = clip->index; i < argc; i++) {
        int show;

        arg = argv[i];

        /* Check for -h or --help */
        show  =
            arg[0] == '-' &&
            arg[1] == 'h' &&
            cli__find_opt_0(clip->base, "h") == NULL;
        if (!show) {
            len  = strlen(arg);
            show =
                len > 6 &&
                memcmp(arg, "--help", 6) == 0 &&
                cli__find_opt_0(clip->base, "help") == NULL;
        }
        /* Only if automatic help was requested, else pass it to call-back */
        show &= (clip->flags & CLIP_FLAG_HELP) != 0;
        if (show) {
            cli_summary(clip, clip->live);
            return CLIP_ERR_HELP;
        }

        /* Check for -v or --version */
        show =
            arg[0] == '-' &&
            arg[1] == 'v' &&
            cli__find_opt_0(clip->base, "v") == NULL;
        if (!show) {
            len = strlen(arg);
            show =
                len >= 9 &&
                memcmp(arg, "--version", 9) == 0 &&
                cli__find_opt_0(clip->base, "version") == NULL;
        }
        show &=
            (clip->flags & CLIP_FLAG_VERSION) != 0 &&
            clip->version != NULL;
        if (show) {
            if ((clip->flags & CLIP_FLAG_USE_ANSI) != 0) {
                fprintf(
                    out,
                    ANSI_PROG "%s" ANSI_END " %s\n",
                    clip->progname,
                    clip->version
                );
            } else {
                fprintf(out, "%s %s\n", clip->progname, clip->version);
            }
            return CLIP_ERR_HELP;
        }
    }

    r = 0;

    while (clip->index < argc) {
        arg = argv[clip->index++];

        if (IS_SHORT_OPT(arg)) {
            int c;

            i = 1;
            do {
                char chr[2];

                chr[0] = c = arg[i++];
                chr[1] = 0;

                opt = cli__find_opt(&cmd, clip, clip->live, chr);
                if (opt == NULL) {
                    cli_bad_arg(
                        out,
                        clip->flags,
                        1,
                        "Invalid option:",
                        chr
                    );
                    r = CLIP_ERR_BAD_ARG;
                    goto done;
                }

                if (opt->mode == 0) {
                    if ((r = clip->cb(clip, cmd, opt, NULL)) != 0) {
                        r = CLIP_ERR_CB_FAIL;
                        goto done;
                    }
                } else if ((opt->mode & ARG_REQD) != 0) {
                    char *val = NULL;

                    if (arg[i] == 0) {
                        if (clip->index < argc) {
                            i = 0;
                            val = argv[clip->index++];
                            arg = val;
                        }
                    } else {
                        val = &arg[i];
                    }

                    if (val == NULL) {
                        cli_bad_arg(
                            out,
                            clip->flags,
                            1,
                            "Missing required value for",
                            chr
                        );
                        r = CLIP_ERR_BAD_ARG;
                        goto done;
                    }
                    len = strlen(val);
                    i += len;
                    if ((r = clip->cb(clip, cmd, opt, val)) != 0) {
                        r = CLIP_ERR_CB_FAIL;
                        goto done;
                    }
                }
            } while (arg[i] != 0);
        } else if (IS_LONG_OPT(arg)) {
            char *key = &arg[2];
            char *eq;

            if ((eq = strchr(key, '=')) != NULL) {
                *eq = 0;
            }

            opt = cli__find_opt(&cmd, clip, clip->live, key);
            if (opt == NULL) {
                cli_bad_arg(
                    out,
                    clip->flags,
                    2,
                    "Invalid option:",
                    key
                );
                r = CLIP_ERR_BAD_ARG;
                goto done;
            }

            if (opt->mode == 0) {
                if ((r = clip->cb(clip, cmd, opt, NULL)) != 0) {
                    r = CLIP_ERR_CB_FAIL;
                    goto done;
                }
            } else if ((opt->mode & ARG_REQD) != 0) {
                const char *val;

                val = NULL;
                if (eq != NULL) {
                    val = eq + 1;
                } else if (clip->index < argc) {
                    val = argv[clip->index++];
                }

                if (val == NULL) {
                    cli_bad_arg(
                        out,
                        clip->flags,
                        2,
                        "Missing required value for",
                        key
                    );
                    r = CLIP_ERR_BAD_ARG;
                    goto done;
                }

                if ((r = clip->cb(clip, cmd, opt, val)) != 0) {
                    r = CLIP_ERR_CB_FAIL;
                    return r;
                }
            }
            if (eq) {
                *eq = '='; /* Don't forget to restore it. */
            }

        } else if (arg[0] == '@') {
            /* Read arguments from file */
            const char *name = &arg[1];
            r = cli_do_file(clip, name, strlen(name));
            if (r != 0) {
                goto done;
            }
        } else if (IS_DOUBLE_DASH(arg)) {
            goto done;
        } else {
            /* Any field? */
            if ((opt = cli__find_any(clip->live)) == NULL) {
                cli_bad_arg(
                    out,
                    clip->flags,
                    0,
                    "Unrecognised option:",
                    arg
                );
                r = CLIP_ERR_BAD_ARG;
                goto done;
            }

            if ((r = clip->cb(clip, clip->live, opt, arg)) != 0) {
                r = CLIP_ERR_CB_FAIL;
                goto done;
            }
        }
    }

done:
    return r;
}
