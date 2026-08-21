/* Minimal config.h for TinyCC on 32-bit x86 */
/* rebuilt */

#ifndef _CONFIG_H
#define _CONFIG_H

/* TCC version */
#define TCC_VERSION "0.9.27"

/* Target architecture */
#define TCC_TARGET_I386 1
#undef TCC_TARGET_X86_64
#undef TCC_TARGET_ARM
#undef TCC_TARGET_ARM64
#undef TCC_TARGET_C67

/* Enable features */
#define ONE_SOURCE 1
#define CONFIG_TCC_ASM 1
#define CONFIG_TCC_BCHECK 0
#define CONFIG_TCC_PREDEFS 1  /* bake predefs as strings, no #include <tccdefs.h> at runtime */
#define CONFIG_TCC_SEMLOCK 0  // delete if something brakes

/* Platform */
#define CONFIG_SYSROOT ""
#define CONFIG_TCCDIR "/lib"
#define CONFIG_TCC_CRTPREFIX "/lib"   /* where crt1.o crti.o crtn.o live */
#define CONFIG_TCC_ELFINTERP ""
#define CONFIG_TCC_LIBPATHS "/lib"
#define CONFIG_TCC_LIBTCC1 ""

#endif /* _CONFIG_H */
