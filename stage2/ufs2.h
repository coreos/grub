/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
 *
 * Copyright (c) 1982, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)dinode.h	8.3 (Berkeley) 1/21/94
 * $FreeBSD: src/sys/ufs/ufs/dinode.h,v 1.11 2002/07/16 22:36:00 mckusick Exp $
 */

#ifndef _GRUB_UFS2_H_
#define _GRUB_UFS2_H_

typedef signed char            int8_t;
typedef signed short           int16_t;
typedef signed int             int32_t;
typedef signed long long int   int64_t;
typedef unsigned char          uint8_t;
typedef unsigned short         uint16_t;
typedef unsigned int           uint32_t;
typedef unsigned long long int uint64_t;

typedef uint8_t                u_char;
typedef uint32_t               u_int;

typedef uint8_t                u_int8_t;
typedef uint16_t               u_int16_t;
typedef uint32_t               u_int32_t;
typedef uint64_t               u_int64_t;

/*
 * __uint* constants already defined in
 * FreeBSD's /usr/include/machine/_types.h
 */
#ifndef _MACHINE__TYPES_H_
typedef uint8_t                __uint8_t;
typedef uint16_t               __uint16_t;
typedef uint32_t               __uint32_t;
typedef uint64_t               __uint64_t;
#endif /* _MACHINE__TYPES_H_ */

#define i_size di_size


#define DEV_BSIZE 512

/*
 * The root inode is the root of the filesystem.  Inode 0 can't be used for
 * normal purposes and historically bad blocks were linked to inode 1, thus
 * the root inode is 2.  (Inode 1 is no longer used for this purpose, however
 * numerous dump tapes make this assumption, so we are stuck with it).
 */
#define	ROOTINO	((ino_t)2)

/*
 * The size of physical and logical block numbers and time fields in UFS.
 */
typedef int32_t ufs1_daddr_t;
typedef	int64_t	ufs2_daddr_t;
typedef int64_t ufs_lbn_t;
typedef int64_t ufs_time_t;

/* inode number */
typedef __uint32_t      ino_t;

/* File permissions. */
#define	IEXEC		0000100		/* Executable. */
#define	IWRITE		0000200		/* Writeable. */
#define	IREAD		0000400		/* Readable. */
#define	ISVTX		0001000		/* Sticky bit. */
#define	ISGID		0002000		/* Set-gid. */
#define	ISUID		0004000		/* Set-uid. */

/* File types. */
#define	IFMT		0170000		/* Mask of file type. */
#define	IFIFO		0010000		/* Named pipe (fifo). */
#define	IFCHR		0020000		/* Character device. */
#define	IFDIR		0040000		/* Directory file. */
#define	IFBLK		0060000		/* Block device. */
#define	IFREG		0100000		/* Regular file. */
#define	IFLNK		0120000		/* Symbolic link. */
#define	IFSOCK		0140000		/* UNIX domain socket. */
#define	IFWHT		0160000		/* Whiteout. */

/*
 * A dinode contains all the meta-data associated with a UFS2 file.
 * This structure defines the on-disk format of a dinode. Since
 * this structure describes an on-disk structure, all its fields
 * are defined by types with precise widths.
 */

#define	NXADDR	2			/* External addresses in inode. */
#define	NDADDR	12			/* Direct addresses in inode. */
#define	NIADDR	3			/* Indirect addresses in inode. */

struct ufs1_dinode {
	u_int16_t       di_mode;        /*   0: IFMT, permissions; see below. */
	int16_t         di_nlink;       /*   2: File link count. */
	union {
		u_int16_t oldids[2];    /*   4: Ffs: old user and group ids. */
	} di_u;
	u_int64_t       di_size;        /*   8: File byte count. */
	int32_t         di_atime;       /*  16: Last access time. */
	int32_t         di_atimensec;   /*  20: Last access time. */
	int32_t         di_mtime;       /*  24: Last modified time. */
	int32_t         di_mtimensec;   /*  28: Last modified time. */
	int32_t         di_ctime;       /*  32: Last inode change time. */
	int32_t         di_ctimensec;   /*  36: Last inode change time. */
	ufs1_daddr_t    di_db[NDADDR];  /*  40: Direct disk blocks. */
	ufs1_daddr_t    di_ib[NIADDR];  /*  88: Indirect disk blocks. */
	u_int32_t       di_flags;       /* 100: Status flags (chflags). */
	int32_t         di_blocks;      /* 104: Blocks actually held. */
	int32_t         di_gen;         /* 108: Generation number. */
	u_int32_t       di_uid;         /* 112: File owner. */
	u_int32_t       di_gid;         /* 116: File group. */
	int32_t         di_spare[2];    /* 120: Reserved; currently unused */
};

struct ufs2_dinode {
	u_int16_t	di_mode;	/*   0: IFMT, permissions; see below. */
	int16_t		di_nlink;	/*   2: File link count. */
	u_int32_t	di_uid;		/*   4: File owner. */
	u_int32_t	di_gid;		/*   8: File group. */
	u_int32_t	di_blksize;	/*  12: Inode blocksize. */
	u_int64_t	di_size;	/*  16: File byte count. */
	u_int64_t	di_blocks;	/*  24: Bytes actually held. */
	ufs_time_t	di_atime;	/*  32: Last access time. */
	ufs_time_t	di_mtime;	/*  40: Last modified time. */
	ufs_time_t	di_ctime;	/*  48: Last inode change time. */
	ufs_time_t	di_birthtime;	/*  56: Inode creation time. */
	int32_t		di_mtimensec;	/*  64: Last modified time. */
	int32_t		di_atimensec;	/*  68: Last access time. */
	int32_t		di_ctimensec;	/*  72: Last inode change time. */
	int32_t		di_birthnsec;	/*  76: Inode creation time. */
	int32_t		di_gen;		/*  80: Generation number. */
	u_int32_t	di_kernflags;	/*  84: Kernel flags. */
	u_int32_t	di_flags;	/*  88: Status flags (chflags). */
	int32_t		di_extsize;	/*  92: External attributes block. */
	ufs2_daddr_t	di_extb[NXADDR];/*  96: External attributes block. */
	ufs2_daddr_t	di_db[NDADDR];	/* 112: Direct disk blocks. */
	ufs2_daddr_t	di_ib[NIADDR];	/* 208: Indirect disk blocks. */
	int64_t		di_spare[3];	/* 232: Reserved; currently unused */
};

#define	MAXNAMLEN	255

struct	direct {
	u_int32_t d_ino;		/* inode number of entry */
	u_int16_t d_reclen;		/* length of this record */
	u_int8_t  d_type; 		/* file type, see below */
	u_int8_t  d_namlen;		/* length of string in d_name */
	char	  d_name[MAXNAMLEN + 1];/* name with length <= MAXNAMLEN */
};

/*
 * File types
 */
#define DT_UNKNOWN       0
#define DT_FIFO          1
#define DT_CHR           2
#define DT_DIR           4
#define DT_BLK           6
#define DT_REG           8
#define DT_LNK          10
#define DT_SOCK         12
#define DT_WHT          14

/*
 * Superblock offsets
 */
#define SBLOCK_FLOPPY        0
#define SBLOCK_UFS1       8192
#define SBLOCK_UFS2      65536
#define SBLOCK_PIGGY    262144
#define SBLOCKSIZE        8192
#define SBLOCKSEARCH \
	{ SBLOCK_UFS2, SBLOCK_UFS1, SBLOCK_FLOPPY, SBLOCK_PIGGY, -1 }

#define MAXMNTLEN	512

#define	NOCSPTRS	((128 / sizeof(void *)) - 4)

/*
 * The maximum number of snapshot nodes that can be associated
 * with each filesystem. This limit affects only the number of
 * snapshot files that can be recorded within the superblock so
 * that they can be found when the filesystem is mounted. However,
 * maintaining too many will slow the filesystem performance, so
 * having this limit is a good idea.
 */
#define FSMAXSNAP 20
	
/*
 * Per cylinder group information; summarized in blocks allocated
 * from first cylinder group data blocks.  These blocks have to be
 * read in from fs_csaddr (size fs_cssize) in addition to the
 * super block.
 */
struct csum {
	int32_t	cs_ndir;		/* number of directories */
	int32_t	cs_nbfree;		/* number of free blocks */
	int32_t	cs_nifree;		/* number of free inodes */
	int32_t	cs_nffree;		/* number of free frags */
};

struct csum_total {
	int64_t	cs_ndir;		/* number of directories */
	int64_t	cs_nbfree;		/* number of free blocks */
	int64_t	cs_nifree;		/* number of free inodes */
	int64_t	cs_nffree;		/* number of free frags */
	int64_t	cs_numclusters;		/* number of free clusters */
	int64_t	cs_spare[3];		/* future expansion */
};

/*
 * Super block for an FFS filesystem.
 */
struct fs {
	int32_t	 fs_firstfield;		/* historic filesystem linked list, */
	int32_t	 fs_unused_1;		/*     used for incore super blocks */
	int32_t	 fs_sblkno;		/* offset of super-block in filesys */
	int32_t	 fs_cblkno;		/* offset of cyl-block in filesys */
	int32_t	 fs_iblkno;		/* offset of inode-blocks in filesys */
	int32_t	 fs_dblkno;		/* offset of first data after cg */
	int32_t	 fs_old_cgoffset;	/* cylinder group offset in cylinder */
	int32_t	 fs_old_cgmask;		/* used to calc mod fs_ntrak */
	int32_t  fs_old_time;		/* last time written */
	int32_t	 fs_old_size;		/* number of blocks in fs */
	int32_t	 fs_old_dsize;		/* number of data blocks in fs */
	int32_t	 fs_ncg;		/* number of cylinder groups */
	int32_t	 fs_bsize;		/* size of basic blocks in fs */
	int32_t	 fs_fsize;		/* size of frag blocks in fs */
	int32_t	 fs_frag;		/* number of frags in a block in fs */
/* these are configuration parameters */
	int32_t	 fs_minfree;		/* minimum percentage of free blocks */
	int32_t	 fs_old_rotdelay;	/* num of ms for optimal next block */
	int32_t	 fs_old_rps;		/* disk revolutions per second */
/* these fields can be computed from the others */
	int32_t	 fs_bmask;		/* ``blkoff'' calc of blk offsets */
	int32_t	 fs_fmask;		/* ``fragoff'' calc of frag offsets */
	int32_t	 fs_bshift;		/* ``lblkno'' calc of logical blkno */
	int32_t	 fs_fshift;		/* ``numfrags'' calc number of frags */
/* these are configuration parameters */
	int32_t	 fs_maxcontig;		/* max number of contiguous blks */
	int32_t	 fs_maxbpg;		/* max number of blks per cyl group */
/* these fields can be computed from the others */
	int32_t	 fs_fragshift;		/* block to frag shift */
	int32_t	 fs_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
	int32_t	 fs_sbsize;		/* actual size of super block */
	int32_t	 fs_spare1[2];		/* old fs_csmask */
					/* old fs_csshift */
	int32_t	 fs_nindir;		/* value of NINDIR */
	int32_t	 fs_inopb;		/* value of INOPB */
	int32_t	 fs_old_nspf;		/* value of NSPF */
/* yet another configuration parameter */
	int32_t	 fs_optim;		/* optimization preference, see below */
	int32_t	 fs_old_npsect;		/* # sectors/track including spares */
	int32_t	 fs_old_interleave;	/* hardware sector interleave */
	int32_t	 fs_old_trackskew;	/* sector 0 skew, per track */
	int32_t	 fs_id[2];		/* unique filesystem id */
/* sizes determined by number of cylinder groups and their sizes */
	int32_t	 fs_old_csaddr;		/* blk addr of cyl grp summary area */
	int32_t	 fs_cssize;		/* size of cyl grp summary area */
	int32_t	 fs_cgsize;		/* cylinder group size */
	int32_t	 fs_spare2;		/* old fs_ntrak */
	int32_t	 fs_old_nsect;		/* sectors per track */
	int32_t  fs_old_spc;		/* sectors per cylinder */
	int32_t	 fs_old_ncyl;		/* cylinders in filesystem */
	int32_t	 fs_old_cpg;		/* cylinders per group */
	int32_t	 fs_ipg;		/* inodes per group */
	int32_t	 fs_fpg;		/* blocks per group * fs_frag */
/* this data must be re-computed after crashes */
	struct	csum fs_old_cstotal;	/* cylinder summary information */
/* these fields are cleared at mount time */
	int8_t   fs_fmod;		/* super block modified flag */
	int8_t   fs_clean;		/* filesystem is clean flag */
	int8_t 	 fs_ronly;		/* mounted read-only flag */
	int8_t   fs_old_flags;		/* old FS_ flags */
	u_char	 fs_fsmnt[MAXMNTLEN];	/* name mounted on */
/* these fields retain the current block allocation info */
	int32_t	 fs_cgrotor;		/* last cg searched */
	void 	*fs_ocsp[NOCSPTRS];	/* padding; was list of fs_cs buffers */
	u_int8_t *fs_contigdirs;	/* # of contiguously allocated dirs */
	struct	csum *fs_csp;		/* cg summary info buffer for fs_cs */
	int32_t	*fs_maxcluster;		/* max cluster in each cyl group */
	u_int	*fs_active;		/* used by snapshots to track fs */
	int32_t	 fs_old_cpc;		/* cyl per cycle in postbl */
	int32_t	 fs_maxbsize;		/* maximum blocking factor permitted */
	int64_t	 fs_sparecon64[17];	/* old rotation block list head */
	int64_t	 fs_sblockloc;		/* byte offset of standard superblock */
	struct	csum_total fs_cstotal;	/* cylinder summary information */
	ufs_time_t fs_time;		/* last time written */
	int64_t	 fs_size;		/* number of blocks in fs */
	int64_t	 fs_dsize;		/* number of data blocks in fs */
	ufs2_daddr_t fs_csaddr;		/* blk addr of cyl grp summary area */
	int64_t	 fs_pendingblocks;	/* blocks in process of being freed */
	int32_t	 fs_pendinginodes;	/* inodes in process of being freed */
	int32_t	 fs_snapinum[FSMAXSNAP];/* list of snapshot inode numbers */
	int32_t	 fs_avgfilesize;	/* expected average file size */
	int32_t	 fs_avgfpdir;		/* expected # of files per directory */
	int32_t	 fs_save_cgsize;	/* save real cg size to use fs_bsize */
	int32_t	 fs_sparecon32[26];	/* reserved for future constants */
	int32_t  fs_flags;		/* see FS_ flags below */
	int32_t	 fs_contigsumsize;	/* size of cluster summary array */ 
	int32_t	 fs_maxsymlinklen;	/* max length of an internal symlink */
	int32_t	 fs_old_inodefmt;	/* format of on-disk inodes */
	u_int64_t fs_maxfilesize;	/* maximum representable file size */
	int64_t	 fs_qbmask;		/* ~fs_bmask for use with 64-bit size */
	int64_t	 fs_qfmask;		/* ~fs_fmask for use with 64-bit size */
	int32_t	 fs_state;		/* validate fs_clean field */
	int32_t	 fs_old_postblformat;	/* format of positional layout tables */
	int32_t	 fs_old_nrpos;		/* number of rotational positions */
	int32_t	 fs_spare5[2];		/* old fs_postbloff */
					/* old fs_rotbloff */
	int32_t	 fs_magic;		/* magic number */
};

/*
 * Filesystem identification
 */
#define FS_UFS1_MAGIC   0x011954        /* UFS1 fast filesystem magic number */
#define	FS_UFS2_MAGIC	0x19540119	/* UFS2 fast filesystem magic number */

/*
 * Turn filesystem block numbers into disk block addresses.
 * This maps filesystem blocks to device size blocks.
 */
#define fsbtodb(fs, b)	((b) << (fs)->fs_fsbtodb)
#define	dbtofsb(fs, b)	((b) >> (fs)->fs_fsbtodb)

/*
 * Cylinder group macros to locate things in cylinder groups.
 * They calc filesystem addresses of cylinder group data structures.
 */
#define	cgbase(fs, c)	((ufs2_daddr_t)((fs)->fs_fpg * (c)))
#define	cgimin(fs, c)	(cgstart(fs, c) + (fs)->fs_iblkno)	/* inode blk */
#define cgstart(fs, c)							\
       ((fs)->fs_magic == FS_UFS2_MAGIC ? cgbase(fs, c) :		\
       (cgbase(fs, c) + (fs)->fs_old_cgoffset * ((c) & ~((fs)->fs_old_cgmask))))

/*
 * Macros for handling inode numbers:
 *     inode number to filesystem block offset.
 *     inode number to cylinder group number.
 *     inode number to filesystem block address.
 */
#define	ino_to_cg(fs, x)	((x) / (fs)->fs_ipg)
#define	ino_to_fsba(fs, x)						\
	((ufs2_daddr_t)(cgimin(fs, ino_to_cg(fs, x)) +			\
	    (blkstofrags((fs), (((x) % (fs)->fs_ipg) / INOPB(fs))))))
#define	ino_to_fsbo(fs, x)	((x) % INOPB(fs))

/*
 * The following macros optimize certain frequently calculated
 * quantities by using shifts and masks in place of divisions
 * modulos and multiplications.
 */
#define blkoff(fs, loc)		/* calculates (loc % fs->fs_bsize) */ \
	((loc) & (fs)->fs_qbmask)

/* Use this only when `blk' is known to be small, e.g., < NDADDR. */
#define smalllblktosize(fs, blk)    /* calculates (blk * fs->fs_bsize) */ \
	((blk) << (fs)->fs_bshift)


#define lblkno(fs, loc)		/* calculates (loc / fs->fs_bsize) */ \
	((loc) >> (fs)->fs_bshift)

#define fragroundup(fs, size)	/* calculates roundup(size, fs->fs_fsize) */ \
	(((size) + (fs)->fs_qfmask) & (fs)->fs_fmask)

#define fragstoblks(fs, frags)	/* calculates (frags / fs->fs_frag) */ \
	((frags) >> (fs)->fs_fragshift)
#define blkstofrags(fs, blks)	/* calculates (blks * fs->fs_frag) */ \
	((blks) << (fs)->fs_fragshift)
#define fragnum(fs, fsb)	/* calculates (fsb % fs->fs_frag) */ \
	((fsb) & ((fs)->fs_frag - 1))
#define blknum(fs, fsb)		/* calculates rounddown(fsb, fs->fs_frag) */ \
	((fsb) &~ ((fs)->fs_frag - 1))

/*
 * Determining the size of a file block in the filesystem.
 */
#define blksize(fs, ip, lbn) \
	(((lbn) >= NDADDR || (ip)->i_size >= smalllblktosize(fs, (lbn) + 1)) \
	    ? (fs)->fs_bsize \
	    : (fragroundup(fs, blkoff(fs, (ip)->i_size))))
#define sblksize(fs, size, lbn) \
	(((lbn) >= NDADDR || (size) >= ((lbn) + 1) << (fs)->fs_bshift) \
	  ? (fs)->fs_bsize \
	  : (fragroundup(fs, blkoff(fs, (size)))))


/*
 * Number of inodes in a secondary storage block/fragment.
 */
#define	INOPB(fs)	((fs)->fs_inopb)
#define	INOPF(fs)	((fs)->fs_inopb >> (fs)->fs_fragshift)

/*
 * Number of indirects in a filesystem block.
 */
#define	NINDIR(fs)	((fs)->fs_nindir)

#define FS_UNCLEAN    0x01      /* filesystem not clean at mount */
#define FS_DOSOFTDEP  0x02      /* filesystem using soft dependencies */
#define FS_NEEDSFSCK  0x04      /* filesystem needs sync fsck before mount */
#define FS_INDEXDIRS  0x08      /* kernel supports indexed directories */
#define FS_ACLS       0x10      /* file system has ACLs enabled */
#define FS_MULTILABEL 0x20      /* file system is MAC multi-label */
#define FS_FLAGS_UPDATED 0x80   /* flags have been moved to new location */

#endif /* _GRUB_UFS2_H_ */
