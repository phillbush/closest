# closest

**closest** focus the closest window in a given direction.

## What is the difference between this closest and [wmutils closest](https://github.com/wmutils/contrib/blob/master/closest.sh)?

* This **closest** implementation gives preference to windows aligned with
  the focused window, this is useful for focusing a tiled window.
* This **closest** focus only windows visible in the current monitor.
* This **closest** uses ewmh hints and client messages by default.
* This **closest** is faster: everything is self-contained in a single .c file.


## Files

The files are:

* `./README`:     This file.
* `./Makefile`:   The makefile.
* `./config.mk`:  The setup for the makefile.
* `./closest.1`:  The manual file (man page) for **closest**.
* `./closest.c`:  The source code of **closest**.
* `./closest.sh`: An implementation of **closest** using shell script and awk.


## Installation

First, edit `./config.mk` to match your local setup.

In order to build **closest** you need the `Xlib` and `Xinerame` header files.
Enter the following command to build **closest**.  This command
creates the binary file `./closest`.

	make

By default, **closest** is installed into the `/usr/local` prefix.  Enter the
following command to install **closest** (if necessary as root).  This command
installs the binary file `./closest` into the `${PREFIX}/bin/` directory, and
the manual file `./closest.1` into `${MANPREFIX}/man1/` directory.

	make install


## Running **closest**

To run **closest**, call it with the argument `left`, `right`, `up` or
`down`, in order to focus the closest window in the respective direction.

For example, the following command focus the closest window to the left:

	$ closest left


## License

This software is in the public domain and is provided AS IS, with NO WARRANTY.
