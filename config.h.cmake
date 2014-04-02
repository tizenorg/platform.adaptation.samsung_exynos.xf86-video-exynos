#include "xorg-server.h"

/* Define to 1 if you have the <dlfcn.h> header file. */
#cmakedefine HAVE_DLFCN_H 1

/* Define to 1 if you have the <inttypes.h> header file. */
#cmakedefine HAVE_INTTYPES_H 1

/* Define to 1 if you have the <memory.h> header file. */
#cmakedefine HAVE_MEMORY_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#cmakedefine HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#cmakedefine HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#cmakedefine HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#cmakedefine HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#cmakedefine HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#cmakedefine HAVE_SYS_TYPES_H 1

/* Enable udev-based monitor hotplug detection */
#cmakedefine HAVE_UDEV 1

/* Define to 1 if you have the <unistd.h> header file. */
#cmakedefine HAVE_UNISTD_H 1

/* Define if you have the xdbg_log_module header file. */
#cmakedefine USE_XDBG_EXTERNAL


/* Name of package */
#define PACKAGE "${PACKAGE}"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "${PACKAGE_BUGREPORT}"

/* Define to the full name of this package. */
#define PACKAGE_NAME "${PACKAGE_NAME}"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "${PACKAGE_STRING}"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "${PACKAGE_TARNAME}"

/* Define to the home page for this package. */
#define PACKAGE_URL "${PACKAGE_URL}"

/* Define to the version of this package. */
#define PACKAGE_VERSION "${PACKAGE_VERSION}"

/* Major version of this package */
#define PACKAGE_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR}

/* Minor version of this package */
#define PACKAGE_VERSION_MINOR ${PACKAGE_VERSION_MINOR}

/* Patch version of this package */
#define PACKAGE_VERSION_PATCHLEVEL ${PACKAGE_VERSION_PATCHLEVEL}

/* Use libpciaccess */
#cmakedefine PCIACCESS

/* Define to 1 if you have the ANSI C header files. */
#cmakedefine STDC_HEADERS 1

/* Version number of package */
#define VERSION "${VERSION}"
