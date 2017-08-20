#!/bin/sh

# IMPORTANT: Gettext must be installed manually.
#
# Brew:
#   By default, Homebrew's gettext formula is keg-only, and does not symlink it into /usr/local
#   Hence we add Homebrew's keg-only path (/usr/local/opt/gettext/bin) to the front of PATH
#   so that users with Homebrew installed need only run `brew install gettext`.
#

# Checks
export PATH=/usr/local/opt/gettext/bin:$PATH:/sw/bin:/usr/local/bin:/opt/local/bin
if type -aP msgfmt; then
    echo "msgfmt exists, skipping"
    exit 0
elif type -aP brew &> /dev/null; then
	echo "error: Gettext is missing, and must be installed before continuing." >&2
	echo "warning: Please run the following command in the terminal: 'brew install gettext'." >&2
	open -b com.apple.Terminal
	exit 1
elif [ -x "/opt/local/bin/port" ]; then
	echo "error: Gettext is missing, and must be installed before continuing." >&2
	echo "warning: Please run the following command in the terminal: 'sudo port install gettext'." >&2
	open -b com.apple.Terminal
	exit 1
else
    touch build/notrans.dis
    echo "warning: Gettext support has been disabled because we could not find a binary, nor a supported method of installation. Please read macosx/README.md for details on how to install Gettext." >&2
    exit 0
fi

