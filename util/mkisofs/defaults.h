/*
 * Header file defaults.h - assorted default values for character strings in
 * the volume descriptor.
 *
 * 	$Id: defaults.h,v 1.8 1999/03/02 03:41:25 eric Exp $
 */

#define  PREPARER_DEFAULT 	NULL
#define  PUBLISHER_DEFAULT	NULL
#ifndef	APPID_DEFAULT
#define  APPID_DEFAULT 		PACKAGE_NAME " ISO 9660 filesystem builder"
#endif
#define  COPYRIGHT_DEFAULT 	NULL
#define  BIBLIO_DEFAULT 	NULL
#define  ABSTRACT_DEFAULT 	NULL
#define  VOLSET_ID_DEFAULT 	NULL
#define  VOLUME_ID_DEFAULT 	"CDROM"
#define  BOOT_CATALOG_DEFAULT   "boot.catalog"
#define  BOOT_IMAGE_DEFAULT     NULL
#define  SYSTEM_ID_DEFAULT 	"GNU"
