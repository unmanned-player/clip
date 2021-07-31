/* SPDX-License-Identifier: ISC */

#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "clip.h"

#ifndef _WIN32
#include <unistd.h>
#endif

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

int main(int argc, char **argv)
{
    static struct cli_opt base_opts[] = {
        CLI_OPT_SWITCH('v', "verbose", "Give more output"),
        CLI_OPT_SWITCH('d', "no-daemon", "Do not daemonize"),
        CLI_OPT_SWITCH('q', "quit", "Quit after clock is set"),
        CLI_OPT_SWITCH('N', NULL, "Run at high priority"),
        CLI_OPT_SWITCH('w', "query-only", "Do not set time (only query peers), implies -n"),
        CLI_OPT_VALUE('s', "run", "PROG", "Run PROG after stepping time, stratum change, and every 11 min"),
        CLI_OPT_VALUE('k', NULL, "FILE", "FILE Key file (ntp.keys compatible)"),
        CLI_OPT_VALUE('p', "PEER", "[keyno:NUM:]PEER",
            "Obtain time from PEER (may be repeated). Use key NUM for "
            "authentication. If -p is not given, 'server HOST' lines from "
            "/etc/ntp.conf are used"),
        CLI_OPT_SWITCH('l', NULL, "Also run as server on port 123"),
        CLI_OPT_VALUE('I', "interface", "IFACE", "Bind server to IFACE, implies -l"),
        CLI_OPT_END()
    };
    static struct cli_sub_cmd base_cmd = CLI_CMD(NULL, base_opts);

    struct clip clip;

    memset(&clip, 0, sizeof(struct clip));

    clip.usr      = NULL;
    clip.progname = "ntpd";
    clip.header   = "NTP client/server";
    clip.footer   = "BusyBox v1.33.0 (2021-05-22 10:51:33 +08) multi-call binary.";
    clip.version  = "1.33.0";
    clip.base     = &base_cmd;
    clip.cmds     = NULL;
    clip.cb       = cb;
    clip.out      = stdout;
    clip.flags    = CLIP_FLAG_HELP | CLIP_FLAG_VERSION;
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
