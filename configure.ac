#
# modio configure.ac
#

# initial information about the project
AC_INIT([modio],[0.8])

# notice message
AC_MSG_NOTICE([Checking depedencies for modio build])

# safety check in case user overwritten src dir
AC_CONFIG_SRCDIR([src/modio.c])

# store the auxiliary tools in build-aux dir
AC_CONFIG_AUX_DIR([build-aux])

# init automake, and specify this program use relaxed structures
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# check for C preprocessor
AC_PROG_CPP

AC_SUBST([CFLAGS], ["-g -O"])

# check C compiler
AC_PROG_CC

# use the C compiler for the following checks
AC_LANG([C])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([string.h], [],  [echo; echo "ERROR: <string.h> not found!, exiting..."; exit -1])
AC_CHECK_HEADERS([unistd.h], [],  [echo; echo "ERROR: <unistd.h> not found!, exiting..."; exit -1])
AC_CHECK_HEADERS([dirent.h], [],  [echo; echo "ERROR: <dirent.h> not found!, exiting..."; exit -1])
AC_CHECK_HEADERS([errno.h], [],  [echo; echo "ERROR: <errno.h> not found!, exiting..."; exit -1])
AC_CHECK_HEADERS([math.h], [],  [echo; echo "ERROR: <math.h> not found!, exiting..."; exit -1])

# include libmodbus include path
AC_SUBST(CPPFLAGS, "$CPPFLAGS -I/usr/local/include/modbus")
AC_CHECK_HEADERS([modbus.h], [],  [echo; echo "ERROR: <modbus.h> not found!, exiting..."; exit -1])
AC_CHECK_HEADERS([libconfig.h], [],  [echo; echo "ERROR: <libconfig.h> not found!, exiting..."; exit -1])

# push C lang on top of stack and set C++ lang
AC_LANG_PUSH([C++])
AC_CHECK_HEADERS([hashmap.h], [],  [echo; echo "ERROR: <hashmap.h> not found!, exiting..."; exit -1])

# pop C lang from top of stack
AC_LANG_POP([C++])

# check for libraries
AC_MSG_CHECKING([Checking whether the math library is present])
AC_CHECK_LIB([m], [log10], [], [echo; echo "ERROR: linker failed to link with libm (-lm), exiting..."])
AC_MSG_CHECKING([Checking whether the modbus library is present])
AC_CHECK_LIB([modbus], [modbus_connect], [], [echo; echo "ERROR: linker failed to link with libmodbus (-lmodbus), exiting..."])
AC_MSG_CHECKING([Checking whether the config library is present])
AC_CHECK_LIB([config], [config_read_file], [], [echo; echo "ERROR: linker failed to link with libconfig (-lconfig), exiting..."])
AC_MSG_CHECKING([Checking whether the hashmap library is present])
AC_CHECK_LIB([hashmap], [hashmap_hash_string], [], [echo; echo "ERROR: linker failed to link with libhashmap (-lhashmap), exiting..."])

#AC_PREFIX_DEFAULT (prefix)
AC_SUBST([modiodir], [$datadir/modio])
#AC_SUBST([CFLAGS], [""])

# create Makefile
AC_CONFIG_FILES([Makefile src/Makefile regs/Makefile])

# gerate output
AC_OUTPUT

# goodbye message
AC_MSG_NOTICE([])
AC_MSG_NOTICE([-------------------------------------------------------------------])
AC_MSG_NOTICE([Configured build system. You can now run make and sudo make install])
AC_MSG_NOTICE([-------------------------------------------------------------------])
AC_MSG_NOTICE([])
