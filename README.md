# `sink` - sinks all input into `/dev/null`

Small and fast util that re-routes `stdin`, `stdout` (and optionally, `stderr`) of a program to `/dev/null` without needing to spawn a (sub-)shell. 
Can be useful in some situations (*see example below*.)

## Usage
Command usage is in the form of: `./sink [<program name> [<arguments...>]]`.

Example:
```shell
$ sink cat sink.c # Identical to `>>/dev/null 0>&1 cat sink.c`, but will not cause cat to error on `stdin` being a redirect to an output stream.
```

When invoked with `<program name>`: `execve()`s into the program (if it exists or is found in `$PATH`) with `<arguments....>` and a passed-through `envp` (see below in *Compiler flags* on changing this behaviour.)

## Building

Run `make && sudo make install` to build with full optimisations and install `sink` to `/usr/local/bin/sink`. (run `sudo make uninstall` to remove.)

To build for debugging, run `make debug`. 

### Compiler flags
* `-DREPLACE_STDERR` - Add to `CFLAGS` when building a target to re-route the child program's `stderr` stream to the sink as well. By default, it is passed through as default.
* `-DNO_SEARCH_PATH` - Add to `CFLAGS` when building to prevent the program looking up its argument in the `PATH` environment variable.
* `-DNO_ENV` - Add to `CFLAGS` when building to prevent `envp` passthrough to the `execve()`'d program.
* `-DDEBUG_IGNORE_SPLASH` - Do not print project and version info in debug builds on startup.

### Installation
The program must have been built before installation, and installation and uninstallation must be done as root.
By default, `make install` will install `sink` to `$(DESTDIR)$(PREFIX)/bin`. You can override this by setting `PREFIX=/path/to/usr make install`

To install a different target, for example, `debug`, run `make debug && sudo make install OUTPUT=sink-debug` (`OUTPUT` is the binary name.)
*NOTE*: You must specify the same `OUTPUT` value for `make uninstall` if done this way.

### Target CPU
If building on a VPS, you may want to run `TARGET_ARCH= make` instead of `make`, to build for generic CPU instead of the default native target for optimised (`release`) builds.

### Static linkage
By default, the program is entirely statically linked. To override this, run `SHARED=yes make`, for any building target.

### Stripping
To build optimised without stripping the symbol table, run `make release` or `make STRIP=:`.
For installing *without* stripping the symbol table, run `sudo make install STRIP=true`; the effect is the same as `STRIP=:` which does not work for the `install` target.

### Other targets
* `sink` (**default**): stripped `release` target. output: `sink`
* `release`: optimised build. output: `sink-release`
* `debug`: unoptimised build with debuginfo. output: `sink-debug`
* `clean`: remove all previous object and binary output files

### Environment variables
* `PREFIX` - The installation path prefix. Defaults to `/usr/local` if not set.
* `SHARED` - set to "yes" to prevent passing `--static` to cc
* `TARGET_ARCH` - target arch (`-march=`), or set `TARGET_ARCH=` to force set to generic target. (default [release only]: `native`)
* `CFLAGS`, `COMMON_FLAGS` - passthrough to cc
* `LDFLAGS` - passthrough to ld
* `DEBUG_CFLAGS`, `DEBUG_LDFLAGS` - passthrough to cc / ld on target `debug`
* `RELEASE_CFLAGS`, `RELEASE_LDFLAGS` - passthrough to cc / ld on target `release`

#### Make overridable only
* `STRIP` - Strip command for default output (stripped release) target (`make sink`). set `make STRIP=:` to prevent stripping entirely. (NOTE: When using `make install`, `STRIP=:` will not work, instead, paradoxically, set `STRIP=true`, they have the same effect for all targets)
* `OUTPUT` - The name of the output binary from the default target, and the binary that `install`/`uninstall` targets look for.

# License
GPL'd with <3
