
DEVPKG
======

A C implementation based on an exercice from the book 'Learn C the hard way' by
Zed A. Shaw. This is a package manager for your operating system capable of
fetching and unpacking tarballs, cloning git repos, running configure scripts,
make, make install with params. It keeps a local database of all installed
packages.

**DISCLAIMER:** This is intended as an exercice, not meant for distribution. Use
at own risk.

USAGE
-----

devpkg <Command> <Options>

Commands:
-I <url>                    # Install package from <url>
-L                          # List all installed packages
-S                          # Initiate the database
-F <url>                    # Fetch a package from <url>
-B <url>                    # Build a package from <url>

Options:
-c <config options> # Will be added as arguments to the `./configuration` script
-m <make options> # Will be added as arguments to `make`.
-i <install options> # Will be added as arguments to `make install`

About Devpkg (README version from Zed A. Shaw)
----------------------------------------------

This is a small little project I'm tinkering on for my book
[Learn C The Hard Way](http://c.learncodethehardway.org) and I hope to
accomplish a few things with it:

1. Teach how to use the bstring and APR libraries.
2. Create a simple install tool for people reading the book.
3. Change how packages are installed so that it's developer controlled.

Goals #1 and #2 are coming along and they should keep this project
simple and easy to understand.  Right now the code is just a quick
thing I whipped up to see if it'd work for the book.  It's definitely
not the ideal version students will see.

Goal #3 however is a more amibitious idea I have to create an alternative
install system that is controlled by the creators of software, not
the maintainers of operating systems.  To accomplish that goal
I'm going with a few design ideas:

1. It relies entirely on the proliferation of standardized build systems
   like autotools, make, gems, and eggs.
2. It relies on the proliferation of easy public hosting of source code
   repositories and .tar.gz or .tar.bz2 files.  Basically you point it
   at a git repo or source tar and it'll figure it out.
3. There is no central repository of packages outside the author's control.
   Everything about how to build the packge resides in the project official
   repository.
4. Dependencies are tracked by URLs so no chance of name collisions and
   weird naming schemes.
5. There is no facility to add "off grid" patches to work around build
   or portability issues, instead, fork the author's repo and give them
   the patch.  If they don't accept it, then have people install the
   fork.

The idea behind this is that there's been a shift in how software is
distributed such that you don't need to have some middleman package
manager "bless" packages for you.  Most programmers end up installing
from source using git, hg, or tar anyway.  With devpkg, you're just
automating and standardizing what programmers already do anyway.
