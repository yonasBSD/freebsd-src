#!/bin/sh

# Copyright 2001, 2002, 2003 by the Massachusetts Institute of Technology.
# All Rights Reserved.
#
# Export of this software from the United States of America may
#   require a specific license from the United States Government.
#   It is the responsibility of any person or organization contemplating
#   export to obtain such a license before exporting.
#
# WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
# distribute this software and its documentation for any purpose and
# without fee is hereby granted, provided that the above copyright
# notice appear in all copies and that both that copyright notice and
# this permission notice appear in supporting documentation, and that
# the name of M.I.T. not be used in advertising or publicity pertaining
# to distribution of the software without specific, written prior
# permission.  Furthermore if you modify this software you must label
# your software as modified software and not distribute it in such a
# fashion that it might be confused with the original M.I.T. software.
# M.I.T. makes no representations about the suitability of
# this software for any purpose.  It is provided "as is" without express
# or implied warranty.
#
#

# Configurable parameters set by autoconf
# Disreagard the above. Edit this by hand in the bespoke FreeBSD build.
version_string="Kerberos 5 release 1.21.3"

prefix=/usr
exec_prefix=${prefix}
includedir=${prefix}/include
libdir=${exec_prefix}/lib
CC_LINK='$(CC) $(PROG_LIBPATH) $(PROG_RPATH_FLAGS) $(CFLAGS) $(LDFLAGS)'
KDB5_DB_LIB=
RPATH_FLAG=''
PROG_RPATH_FLAGS=''
PTHREAD_CFLAGS='-pthread'
DL_LIB=''
DEFCCNAME='FILE:/tmp/krb5cc_%{uid}'
DEFKTNAME='FILE:/etc/krb5.keytab'
DEFCKTNAME='FILE:/var/krb5/user/%{euid}/client.keytab'

LIBS='-lintl -L/usr/local/lib'
GEN_LIB=

# Defaults for program
library=krb5

# Some constants
vendor_string="Massachusetts Institute of Technology"

# Process arguments
# Yes, we are sloppy, library specifications can come before options
while test $# != 0; do
    case $1 in
	--all)
	    do_all=1
	    ;;
	--cflags)
	    do_cflags=1
	    ;;
	--defccname)
	    do_defccname=1
	    ;;
	--defcktname)
	    do_defcktname=1
	    ;;
	--defktname)
	    do_defktname=1
	    ;;
	--deps) # historically a no-op
	    ;;
	--exec-prefix)
	    do_exec_prefix=1
	    ;;
	--help)
	    do_help=1
	    ;;
	--libs)
	    do_libs=1
	    ;;
	--prefix)
	    do_prefix=1
	    ;;
	--vendor)
	    do_vendor=1
	    ;;
	--version)
	    do_version=1
	    ;;
	krb5)
	    library=krb5
	    ;;
	gssapi)
	    library=gssapi
	    ;;
	gssrpc)
	    library=gssrpc
	    ;;
	kadm-client)
	    library=kadm_client
	    ;;
	kadm-server)
	    library=kadm_server
	    ;;
	kdb)
	    library=kdb
	    ;;
	*)
	    echo "$0: Unknown option \`$1' -- use \`--help' for usage"
	    exit 1
    esac
    shift
done

# If required options - provide help
if test -z "$do_all" -a -z "$do_version" -a -z "$do_vendor" -a \
    -z "$do_prefix" -a -z "$do_vendor" -a -z "$do_exec_prefix" -a \
    -z "$do_defccname" -a -z "$do_defktname" -a -z "$do_defcktname" -a \
    -z "$do_cflags" -a -z "$do_libs"; then
    do_help=1
fi


if test -n "$do_help"; then
    echo "Usage: $0 [OPTIONS] [LIBRARIES]"
    echo "Options:"
    echo "        [--help]          Help"
    echo "        [--all]           Display version, vendor, and various values"
    echo "        [--version]       Version information"
    echo "        [--vendor]        Vendor information"
    echo "        [--prefix]        Kerberos installed prefix"
    echo "        [--exec-prefix]   Kerberos installed exec_prefix"
    echo "        [--defccname]     Show built-in default ccache name"
    echo "        [--defktname]     Show built-in default keytab name"
    echo "        [--defcktname]    Show built-in default client keytab name"
    echo "        [--cflags]        Compile time CFLAGS"
    echo "        [--libs]          List libraries required to link [LIBRARIES]"
    echo "Libraries:"
    echo "        krb5              Kerberos 5 application"
    echo "        gssapi            GSSAPI application with Kerberos 5 bindings"
    echo "        gssrpc            GSSAPI RPC application"
    echo "        kadm-client       Kadmin client"
    echo "        kadm-server       Kadmin server"
    echo "        kdb               Application that accesses the kerberos database"
    exit 0
fi

if test -n "$do_all"; then
    all_exit=
    do_version=1
    do_prefix=1
    do_exec_prefix=1
    do_vendor=1
    title_version="Version:     "
    title_prefix="Prefix:      "
    title_exec_prefix="Exec_prefix: "
    title_vendor="Vendor:      "
else
    all_exit="exit 0"
fi

if test -n "$do_version"; then
    echo "$title_version$version_string"
    $all_exit
fi

if test -n "$do_vendor"; then
    echo "$title_vendor$vendor_string"
    $all_exit
fi

if test -n "$do_prefix"; then
    echo "$title_prefix$prefix"
    $all_exit
fi

if test -n "$do_exec_prefix"; then
    echo "$title_exec_prefix$exec_prefix"
    $all_exit
fi

if test -n "$do_defccname"; then
    echo "$DEFCCNAME"
    $all_exit
fi

if test -n "$do_defktname"; then
    echo "$DEFKTNAME"
    $all_exit
fi

if test -n "$do_defcktname"; then
    echo "$DEFCKTNAME"
    $all_exit
fi

if test -n "$do_cflags"; then
    if test x"$includedir" != x"/usr/include" ; then
        echo "-I${includedir}"
    else
        echo ''
    fi
fi


if test -n "$do_libs"; then
    # Assumes /usr/lib is the standard library directory everywhere...
    if test "$libdir" = /usr/lib; then
	libdirarg=
    else
	libdirarg="-L$libdir"
    fi
    # Ugly gross hack for our build tree
    lib_flags=`echo $CC_LINK | sed -e 's/\$(CC)//' \
	    -e 's/\$(PURE)//' \
	    -e 's#\$(PROG_RPATH_FLAGS)#'"$PROG_RPATH_FLAGS"'#' \
	    -e 's#\$(PROG_RPATH)#'$libdir'#' \
	    -e 's#\$(PROG_LIBPATH)#'$libdirarg'#' \
	    -e 's#\$(RPATH_FLAG)#'"$RPATH_FLAG"'#' \
	    -e 's#\$(LDFLAGS)##' \
	    -e 's#\$(PTHREAD_CFLAGS)#'"$PTHREAD_CFLAGS"'#' \
	    -e 's#\$(CFLAGS)##'`

    if test $library = 'kdb'; then
	lib_flags="$lib_flags -lkdb5 $KDB5_DB_LIB"
	library=krb5
    fi

    if test $library = 'kadm_server'; then
	lib_flags="$lib_flags -lkadm5srv_mit -lkdb5 $KDB5_DB_LIB"
	library=gssrpc
    fi

    if test $library = 'kadm_client'; then
	lib_flags="$lib_flags -lkadm5clnt_mit"
	library=gssrpc
    fi

    if test $library = 'gssrpc'; then
	lib_flags="$lib_flags -lgssrpc"
	library=gssapi
    fi

    if test $library = 'gssapi'; then
	lib_flags="$lib_flags -lgssapi_krb5"
	library=krb5
    fi

    if test $library = 'krb5'; then
	lib_flags="$lib_flags -lkrb5 -lk5crypto -lcom_err"
    fi

    # If we ever support a flag to generate output suitable for static
    # linking, we would output "-lkrb5support $GEN_LIB $LIBS $DL_LIB"
    # here.

    echo $lib_flags
fi

exit 0
