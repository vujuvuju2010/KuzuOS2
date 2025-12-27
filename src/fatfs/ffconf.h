/*----------------------------------------------------------------------------/
/  FatFs - Configuration File for FatFs Module                                /
/-----------------------------------------------------------------------------/
/ Copyright (C) 2019, ChaN, all right reserved.                               /
/----------------------------------------------------------------------------*/
#ifndef FFCONF_H
#define FFCONF_H

/*---------------------------------------------------------------------------*/
/* FatFs Configuration Options                                               */
/*---------------------------------------------------------------------------*/

#define FFCONF_DEF  5380   /* Revision ID: R0.15a */

#define FF_FS_READONLY    0
#define FF_FS_MINIMIZE    0
#define FF_USE_STRFUNC    0
#define FF_USE_FIND       0
#define FF_USE_MKFS       1
#define FF_USE_FASTSEEK   0
#define FF_USE_EXPAND     0
#define FF_USE_CHMOD      0
#define FF_USE_LABEL      0
#define FF_USE_FORWARD    0

#define FF_CODE_PAGE      437
#define FF_USE_LFN        0
#define FF_MAX_LFN        255
#define FF_LFN_UNICODE    0
#define FF_LFN_BUF        255
#define FF_SFN_BUF        12

#define FF_FS_RPATH       0
#define FF_VOLUMES        1
#define FF_STR_VOLUME_ID  0
#define FF_VOLUME_STRS    "RAM","NAND","CF","SD1","SD2","USB1","USB2","USB3"
#define FF_MULTI_PARTITION 0
#define FF_MIN_SS         512
#define FF_MAX_SS         512
#define FF_LBA64          0
#define FF_MIN_GPT        0x10000000
#define FF_USE_TRIM       0

#define FF_FS_TINY        0
#define FF_FS_EXFAT       0
#define FF_FS_NORTC       1
#define FF_NORTC_MON      1
#define FF_NORTC_MDAY     1
#define FF_NORTC_YEAR     2020
#define FF_FS_NOFSINFO    0

#define FF_FS_LOCK        0
#define FF_FS_REENTRANT   0
#define FF_FS_TIMEOUT     1000
#define FF_SYNC_t         HANDLE

#endif /* FFCONF_H */ 