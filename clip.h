/* SPDX-License-Identifier: ISC */
#ifndef CLIP_H
#define CLIP_H

#include <stddef.h>

#include <stdio.h>

#ifndef CLIP_BUFFER_SIZE
#define CLIP_BUFFER_SIZE                1024
#endif

/**
 * This is not an error as such, but a return code showing that the parser
 * encountered -h/--help or -v/--version on the command line.
 */
#define CLIP_ERR_HELP                    1

/**
 * Parser completed successfully.
 */
#define CLIP_ERR_OK                      0

/**
 * An invalid option was encountered during parsing.
 */
#define CLIP_ERR_INVALID                -1

/**
 * Call-back returned non-zero value.
 */
#define CLIP_ERR_CB_FAIL                -2

/**
 * Parser encountered a bad-sub-command.
 */
#define CLIP_ERR_BAD_SUBCMD             -3

/**
 * Arguments file could not be opened.
 */
#define CLIP_ERR_BAD_ARG                -4

/**
 * Provide -h/--help support automatically.
 */
#define CLIP_FLAG_HELP                  ((unsigned)0x01)

/**
 * Provide -v/--version support automatically. In case the default sub-command
 * already has a `-v` this flag only enables --version.
 */
#define CLIP_FLAG_VERSION               ((unsigned)0x02)

/**
 * Use colours to display help summary.
 */
#define CLIP_FLAG_USE_ANSI              ((unsigned)0x04)

/**
 * \brief Define a generic command-line option
 * \hideinitializer
 */
#define CLI_OPT_GENERIC(_short, _long, _tag, _mode, _help) \
    { _short, _long, _tag, _mode, _help }

/**
 * \brief Define a switch option
 *
 * \hideinitializer
 *
 * This is useful for representing boolean-like command-line option that doesn't
 * take any values. For e.g., `-v` or `--verbose`. The same switch can appear
 * multiple times on the CLI and for each time it appears, the call-back is
 * invoked.
 *
 * \param _short
 *      Short, sing character option
 * \param _long
 *      Long string like option
 * \param _help
 *      A brief help message describing the option
 */
#define CLI_OPT_SWITCH(_short, _long, _help) \
    { _short, _long, NULL, 0, _help }

/**
 * \brief Define an option that also takes a value
 * \hideinitializer
 *
 * \param _short
 *      Short, sing character option
 * \param _long
 *      Long string like option
 * \param _tag
 *      Single word tag naming the value
 * \param _help
 *      A brief help message describing the option
 */
#define CLI_OPT_VALUE(_short, _long, _tag, _help) \
    { _short, _long, _tag, ((unsigned)0x01), _help }

/**
 * \brief Final list of arguments usually to capture a list of files
 * \hideinitializer
 *
 * \details
 *  When there are a list of files to be captures on CLI, this is the kind of
 *  option to use. For example,
 * 
 *      myprog -c file
 *
 * \note There is no short/long option for this, just a tag.
 *
 * \param _tag
 *      Single word tag naming the value
 * \param _help
 *      A brief help message describing the option
 */
#define CLI_OPT_NARGS(_tag, _help) \
    { 0, NULL, _tag, ((unsigned)0x02), _help }

/**
 * \brief Mark the end of options list
 * \hideinitializer
 */
#define CLI_OPT_END() \
    { 0, NULL, NULL, 0, NULL }

/**
 * \brief Add a sub-command to the list
 * \hideinitializer
 *
 * \param _name
 *      Name of the sub-command. Set this to NULL for default sub-command.
 * \param _opts
 *      Pointer to struct cli_opt that belongs to this sub-command.
 */
#define CLI_CMD(_name, _opts) \
    { _name, _opts }

/**
 * \brief Mark the end of sub-commands-list
 * \hideinitializer
 */
#define CLI_CMD_END()                   CLI_CMD(NULL, NULL)

#ifdef __cplusplus
extern "C" {
#endif

struct cli_opt;
struct cli_sub_cmd;
struct clip;

/**
 * \brief Call back function that will be invoked for every option
 *
 * \param clap
 *      The Command Line Parser context
 * \param cmd
 *      The sub-command object or the default/global command object in which the
 *      option appears
 * \param arg
 *      The command line option. NULL if this is a call-back for sub-command
 * \param value
 *      The option's value if any or NULL
 */
typedef int (*clap_cb)(
    const struct clip *clap,
    const struct cli_sub_cmd *cmd,
    const struct cli_opt *arg,
    const char *value
);

/**
 * \brief A single command-line option definition
 *
 * \note Use `CLI_OPT_*` macros to define options
 */
struct cli_opt {
    int a_short;
    const char *a_long;
    const char *tag;
    unsigned mode;
    const char *help;
};

/**
 * \brief A single sub-command
 *
 * \details Represents both the default options grouping or groupings for a
 * specific sub-command.
 *
 * \note Use `CLI_CMD*` macros to define a sub-command.
 */
struct cli_sub_cmd {
    const char *name;
    const struct cli_opt *opts;
};

/**
 * \brief The command-line parser context
 *
 * \details
 *      This is the top-level definition for command-line parser. Create an
 *      instance of this structure, have it cleared using `::memset()` or other
 *      means and fill only the public fields. For, example:
 *
 *  ```c
 *      struct clip clap;
 *
 *      memset(&clap, 0, sizeof(struct clip));
 *
 *      clap.usr      = NULL;
 *      clap.progname = "my-prog";
 *      clap.header   = "My program can do many things";
 *      clap.footer   = "Copyright (c) 2021 Some Programmer";
 *      clap.version  = NULL;
 *      clap.base     = &cmd; // Any common options
 *      clap.cmds     = NULL; // A list of sub-commands
 *      clap.cb       = cb; // Call back
 *      clap.out      = stdout;
 *      clap.flags    =
 *          CLIP_FLAG_HELP |
 *          CLIP_FLAG_VERSION |
 *          (isatty(fileno(clap.out))? CLIP_FLAG_USE_ANSI: 0);
 *  ```
 */
struct clip {
    /**
     * User defined pointer, it can point to anything
     */
    void *usr;

    /**
     * Use ANSI codes to display
     *
     * Instead of blindly setting this, one possible way to set this would be
     * as:
     *
     *    clap.use_ansi = isatty(fileno(clap.f_out));
     */
    unsigned flags;

    /**
     * Name of the program
     *
     * \note GNU prefers that you explicitly state the program name as argv[0]
     * is usually unreliable.
     */
    const char *progname;

    /**
     * Small, one line description of the program
     */
    const char *header;

    /**
     * Copyrights and licenses if any
     */
    const char *footer;

    /**
     * Optional version of the program
     */
    const char *version;

    /**
     * Global/default options
     *
     * In small programs where there aren't any sub-commands, this is the only
     * set of options defined.
     */
    const struct cli_sub_cmd *base;

    /**
     * A ::CLI_CMD_END terminated list of sub-commands
     */
    const struct cli_sub_cmd *cmds;

    /**
     * Callback function to be invoked for each option
     *
     * \note this function is invoked for the sub-command as well as for each
     * option
     */
    clap_cb cb;

    /**
     * The place where help, error messages, etc. will be printed.
     */
    FILE *out;

    /* PRIVATE or RETURN FIELDS */

    int index;
    const struct cli_sub_cmd *live;
};

/**
 * \brief Display a summary of options on a specific sub-command.
 *
 * \details
 *  The summary is printed to `
 *  If the `cmd` is passed as NULL, then it picks `clap->base` as default and
 *  attempts to print a summary of it. If `clap->flags` specifies
 *  `::CLIP_FLAG_USE_ANSI`, then some ANSI escape sequences will be used to
 *  print colourful text.
 *
 * \param clap
 *      The command-line parser context
 * \param cmd
 *      Optional sub-command or NULL if global/common options need to be
 *      displayed.
 *
 * \returns 0
 *      On success
 * \returns CLIP_ERR_INVALID
 *      If `clap` is NULL
 */
int cli_summary(struct clip *clap, const struct cli_sub_cmd *cmd);

/**
 * \def cli_verify
 * \brief Check if the given `clap` instance is correct
 *
 * \details
 *  Assert that the given instance of the `clap` is valid and suitable for
 *  parsing. It calls actual `::assert()` function from `<assert.h>`. This is
 *  intended to be used in debug builds and is meant for development use only.
 *  In production builds, this function will disappear, same way as `::assert()`
 *  would disappear.
 */
void cli_verify(struct clip *clap);

/**
 * \brief Parse command-line arguments vector
 *
 * \details
 *  In C, argc/argv is the most common way to receive arguments from
 *  command-line. On success or failure, clap->index is set to the last argument
 *  that was processed.
 *
 * \returns CLIP_ERR_OK
 *      On success
 * \returns CLIP_ERR_HELP
 *      Help/Version was requested on command-line. No arguments are parsed.
 * \returns CLIP_ERR_INVALID
 *      Any arguments was unexpected, NULL
 * \returns CLIP_ERR_CB_FAIL
 *      Call-back did not return 0
 * \returns CLIP_ERR_BAD_SUBCMD
 *      No such sub-command at argv[1]
 * \returns CLIP_ERR_BAD_ARG
 *      Option on command-line did not match any in the registered options
 */
int cli_parse(struct clip *clap, int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif /* CLIP_H */
