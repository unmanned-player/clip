# Command Line Parser for C

CLI is a source-only library for parsing command-line arguments in C or perhaps
even C++. It parses POSIX and GNU styled switches, it's fast, terse yet doesn't
skimp on features. CLIP is written in ISO C-89/90 and should work on any
reasonably standards compliant C compiler. Copy `cli.h` and `cli.c` into your
source tree and include the header file in the place where command-line is
parsed. Usually, this is in the source file where `main()` function is. In the
`main()` function or wherever command line is parsed, invoke `cli_parse()` once
with appropriate arguments. That's all there is to it.

Use `cli_verify()` for debugging purpose only. It's meant to ensure all
structures are correct. In case, `struct clip` disables (by not specifying)
`CLAP_FLAG_HELP`, use `cli_summary()` to print help summary.

This parser adds an extra feature for systems where arguments memory is too
small. In that, the arguments can be saved to a file and passed on the
command-line as:

```
C:\DOS> myprog.exe @args.txt
```

where, `@args.txt` contains one option per line. For example,

```
no-daemon
q
peer=192.168.1.1
key=keys.txt
```

Note, there's no need to begin switch with `-` or `--` when set in file. The
`@file` argument can be interspersed with other options on command-line too.
Command-line is always parsed in the same order of their appearance.

If CLIP is used with sub-commands, displaying help summary for each sub-command
is also possible.

```
$ myprog -h

Usage: myprog [COMMAND] [OPTIONS]
One-line summary for myprog

Sub-commands:
    something

Options:
...
```

and,
```
$ myprog something -h

Usage: myprog install [OPTIONS]
One-line summary for myprog

Default Options:
-h, --help
  Show help message.
--version
  Show version and if available, copyright information.

Options:
...
```

## Features

 * ANSI/ISO C-89 compliant, works on old compilers too as long as it supports
   16-bits or above.
 * Consistent POSIX/GNU styled command-line parsing across platforms
 * Sub-commands support like `git`, `pip`, etc.
 * Automatic help and version summary with ANSI/VT-100 colours
 * Simple call-back interface
 * Parse arguments inside file
 * Depends on standard C functions only
 * No memory allocation calls are made by the parser

## Description

CLI sees command line arguments as a set of options (`struct cli_opt`) grouped
in one or more sub-commands (`struct cli_cmd`) -- all referred inside top-level
parser context (struct `cli`). There is always a default sub-command with no
name, but more can be added as long as they have unique names. For the
sub-command and for each option on the command-line a registered call-back is
invoked. CLI does not use global variables as everything is stored inside the
parser context `struct clip`.

Define a top-level parser context as:
```c
struct clip prog_cli = {
    .usr = NULL /* Store some user pointer, useful in call-backs */
    .flags = CLIP_FLAG_HELP | CLIP_FLAG_VERSION | CLIP_FLAG_USE_ANSI,
    .progname = "myprog",
    .header = "A simple demo program",
    .footer = "Copyright (c) 2020"
    .version = "1.2.3",
    .base = base_cmd,
    .cmds = sub_cmds,
    .out = stdout, /* Default file to print messages/summary to */
    .cb = mycb, /* The registered call-back */
};
```
**usr** field is a pointer back to some user specific data. CLI doesn't use it.

**flags** field tells `cli_parse_argv()` function to automatically provide help
and version summary should it encounter `-h`/`--help` or `-v`/`--version`. It
is also reasonably intelligent should `-v` mean `--verbose` and accordingly
consider `-v`. The flag `CLIP_FLAG_USE_ANSI` tell that these summaries be
printed with some colours. Note, it would be better to check if `prog_cli.out`
is actually capable of ANSI escape sequences before setting this flag. For
example,
```c
    prog_cli.flags |= isatty(fileno(prog_cli.out))? CLIP_FLAG_USE_ANSI: 0;
```
**progname** must be the proper name of program. It is highly recommended that
this be a constant string and not dependent on `argv[0]`. **header** is a short
description of the program and optionally, **footer** is any copyright
statements. **version** is mandatory only if `CLIP_FLAG_VERSION` was used in the
flags and should be just a stringified version.

**out** is the default output `FILE` pointer. Usually, this is `stdout`. Care
must be taken to ensure that this `FILE` pointer can support ANSI escape codes
if `CLIP_FLAG_USE_ANSI` is used.

**base** is a list of default options. For most trivial use-cases, this is the
only list of options that need to be defined.

**cmds** is a list of sub-commands. Sub-commands appear as the first argument
in the command-line at `argv[1]`. Note, that during parsing if a sub-command was
selected and an option was not found in the current sub-command, it will search
for the option in the base/default options list. If the option was not found
even there, only then parser will fail.

**cb** is the call-back function that is invoked first for a sub-command if it
is not `cli::base` and once for every command-line option encountered in the
arguments vector. The call-back signature is:
```c
typedef int (*clap_cb)(
    const struct clip *clip,
    const struct cli_sub_cmd *cmd,
    const struct cli_opt *arg,
    const char *value
);
```

Regardless of how the call-back is invoked, `clip` points to the top-level
definition that was created. In our example, this would point to `prog_cli`.
When the call-back is invoked for sub-command, `cmd` is always non-NULL
pointing to the actual sub-command while `arg` and `value` are NULL. If called
for an option, `cmd` either points to the sub-command or the default on at
`base` while `arg` is non-NULL and may carry `value` if the option was declared
using `CLI_OPT_VALUE()` or `CLI_OPT_NARGS()`.

Options are defined as an array of unique values. Note that, it is the
developer's responsibility to ensure options are unique in the array. For
example,
```c
static struct cli_opt base_opts[] = {
    CLI_OPT_SWITCH('v', "verbose", "Give more output"),
    CLI_OPT_VALUE('f', "output", "FILE", "Output file"),
    CLI_OPT_NARGS("FILE", "Input files list"),
    CLI_OPT_END()
};
```

If there is no short switch for the option, it must contain a non-zero number
preferably > 256 to avoid any printable characters.

The macro `CLI_OPT_SWITCH` is used to define a simple option that doesn't take
any value with it. The switch option can appear several times on the command
line and for each appearance, the registered call-back is invoked.

**Note**: An option is considered *hidden* if help message (last parameter) is
set to `NULL`.

## Usage and examples

See `exntpd.c` for simple use-case and `expip.c` for sub-command usage
examples.

## License, contributions, blames

This project is released under ISC license. The project can be nominally found
at [GitLab](https://gitlab.com/unmdplyr/clip). Please report issues or raise
feature requests or contributions there.
