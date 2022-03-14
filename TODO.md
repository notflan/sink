# Argument passing format

Argument parsing can be done with short `-<chars...>` and/or long `--arg[{=, }<value>]` (with or without value, `=` optional.
Short arguments can be chained together: `-abc` is equal to `-a -b -c`. There can be multiple of these: `-ie -O` is equal to `-i -e -O`.

# **TODO** Long arguments

* **TODO** User-set sink file path (e.g `--path /path/to/sink`) which overrides the default `/dev/null`.
* `--env <file>` - Read environment variables, line-by-line, from `<file>` in the format `KEY=value` and send that as the program's `envp`. Multiple of these can exist, they are concatenated.
(example usage: `LD_PRELOAD=malicious.so sink --env <(env | grep ^LD_PRELOAD) setuid-binary`)
* `--env=[<key>=<value>]` - Add (or replace) `<key>=<value>` to/in `envp`. Multiple of these can exist. 
(example usage: `sink --env=LD_PRELOAD=replace.so binary`)

# Short Arguments
* `-i`	Replace `stdin` (**default**)
* `-o`	Replace `stdout` (**default**)
* `-e`	Replace `stderr` (*default with feature* `REPLACE_STDERR`)
* `-I`	No replace `stdin`
* `-O`	No replace `stderr`
* `-E`	No replace `stderr` (**default** *without feature* `REPLACE_STDERR`)

# **TODO** Other options
* **TODO** Specific fd passthrough (*probably long arg, could also possibly be short args for std{in,out,err} streams specifically*)
* **TOTO** CLOEXEC control (*short arg*)
* **TOTO** add specific fds / open specific files (optionally also at specific fds) to passthrough (*probably long arg*)

# Examples

Short args:
```shell
$ sink -ioe cat	# sink steam in,out,err
$ sink -iOE cat	# sink stream in. keep stream out and err
$ sink -O cat	# sink default streams, but keep out
$ sink -E cat	# sink default streams and err
```

**TODO** Long args:


## Redundant examples

Short args:
```shell
$ sink -iI cat	# sink default streams and in (I comes after the redundant i)
$ sink -Ii cat	# sink default streams except in (i comes after the redundant I)
```

**TODO** Long args:

