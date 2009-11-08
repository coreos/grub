/*
 * Header file defaults.h - assorted default values for character strings in
 * the volume descriptor.
 *
 * 	$Id: defaults.h,v 1.6 1998/06/02 02:40:37 eric Exp $
 */

#define  PREPARER_DEFAULT 	NULL
#define  PUBLISHER_DEFAULT	NULL
#define  APPID_DEFAULT 		NULL
#define  COPYRIGHT_DEFAULT 	NULL
#define  BIBLIO_DEFAULT 	NULL
#define  ABSTRACT_DEFAULT 	NULL
#define  VOLSET_ID_DEFAULT 	NULL
#define  VOLUME_ID_DEFAULT 	"CDROM"
#define  BOOT_CATALOG_DEFAULT   "boot.catalog"
#define  BOOT_IMAGE_DEFAULT     NULL
#ifdef __QNX__
#define  SYSTEM_ID_DEFAULT 	"QNX"
#endif

#ifdef __osf__
#define  SYSTEM_ID_DEFAULT 	"OSF"
#endif

#ifdef __sun
#ifdef __svr4__
#define  SYSTEM_ID_DEFAULT    "Solaris"
#else
#define  SYSTEM_ID_DEFAULT    "SunOS"
#endif
#endif

#ifdef __hpux
#define  SYSTEM_ID_DEFAULT 	"HP-UX"
#endif

#ifdef __sgi
#define  SYSTEM_ID_DEFAULT 	"SGI"
#endif

#ifdef _AIX
#define  SYSTEM_ID_DEFAULT 	"AIX"
#endif

#ifdef _WIN
#define	SYSTEM_ID_DEFAULT       "Win32"
#endif /* _WIN */

#ifndef SYSTEM_ID_DEFAULT
#define  SYSTEM_ID_DEFAULT 	"LINUX"
#endif
