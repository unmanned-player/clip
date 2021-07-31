/* SPDX-License-Identifier: ISC */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "clip.h"

#ifndef _WIN32
#include <unistd.h>
#endif

/* Sub-command usage example. */

static int cb(
    const struct clip *clip,
    const struct cli_sub_cmd *cmd,
    const struct cli_opt *opt,
    const char *value)
{
    fprintf(clip->out, "CB: ");

    if (cmd != NULL && cmd->name != NULL) {
        fprintf(clip->out, "%s >> ", cmd->name);
    }

    if (opt != NULL) {
        if (isalnum(opt->a_short)) {
            fprintf(clip->out, "-%c", opt->a_short);
        } else if (opt->a_long != NULL) {
            fprintf(clip->out, "--%s", opt->a_long);
        }
        if (opt->tag != NULL) {
            fprintf(clip->out, " <%s>", opt->tag);
        }
    }
    if (value != NULL) {
        fprintf(clip->out, "\t -> %s", value);
    }

    fprintf(clip->out, "\n");

    return 0;
}

static struct cli_opt base_opts[] = {
    CLI_OPT_SWITCH('v', "verbose", "Give more output."),
    CLI_OPT_SWITCH(300, "version", "Show version and exit."),
    CLI_OPT_SWITCH('q', "quit", "Give less output."),
    CLI_OPT_VALUE(301, "log", "path", "Path to a verbose appending log."),
    CLI_OPT_SWITCH(302, "no-input", "Disable prompting for input."),
    CLI_OPT_END()
};
static struct cli_opt install_opts[] = {
    CLI_OPT_VALUE('e', "editable", "path/url", "Install a project in editable mode"),
    CLI_OPT_VALUE('r', "requirement", "file", "Install from the given requirements file."),
    CLI_OPT_VALUE('t', "target", "dir", "Install packages into <dir>."),
    CLI_OPT_SWITCH('U', "upgrade", "Upgrade all packages to the newest available version."),
    CLI_OPT_SWITCH(305, "no-deps", "Don't install package dependencies."),
    /* This option will not be displayed in help summary as there's no help */
    CLI_OPT_SWITCH(306, "secret", NULL),
    CLI_OPT_END()
};
static const struct cli_sub_cmd base_cmd = CLI_CMD(NULL, base_opts);

static const struct cli_sub_cmd cmd_list[] = {
    CLI_CMD("install", install_opts),
    CLI_CMD_END()
};

int main(int argc, char **argv)
{
    struct clip clip;

    memset(&clip, 0, sizeof(struct clip));

    clip.usr      = NULL;
    clip.progname = "pip";
    clip.header   = "A tool for installing and managing Python packages";
    clip.footer   = "Copyright (c) 2020 someone";
    clip.version  = "1.2.3-alpha";
    clip.base     = &base_cmd;
    clip.cmds     = cmd_list;
    clip.cb       = cb;
    clip.out      = stdout;
    clip.flags    = CLIP_FLAG_HELP | CLIP_FLAG_VERSION;
    /* Windows doesn't have ANSI.SYS anymore :( */
#ifndef _WIN32
    clip.flags   |= isatty(fileno(clip.out))? CLIP_FLAG_USE_ANSI: 0;
#endif

    /* Windows doesn't have ANSI.SYS anymore :( */
#ifndef _WIN32
    clip.flags   |= isatty(fileno(clip.out))? CLIP_FLAG_USE_ANSI: 0;
#endif

#ifdef _DEBUG
    /* Use this function only in DEBUG/dev mode to verify if all the settings
     * and definitions correct.
     */
    cli_verify(&clip);
#endif

    return cli_parse(&clip, argc, argv);
}
