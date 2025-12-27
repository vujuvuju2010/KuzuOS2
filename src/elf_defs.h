// Minimal ELF definitions for kernel space
#ifndef ELF_DEFS_H
#define ELF_DEFS_H

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#define NULL ((void*)0)

// ELF magic
#define ELFMAG0     0x7F
#define ELFMAG1     'E'
#define ELFMAG2     'L'
#define ELFMAG3     'F'

// ELF class
#define ELFCLASS32  1
#define ELFCLASS64  2

// ELF version
#define EV_CURRENT  1

// ELF type
#define ET_EXEC     2
#define ET_DYN      3

// ELF machine
#define EM_386      3

// ELF identification indices
#define EI_MAG0     0
#define EI_MAG1     1
#define EI_MAG2     2
#define EI_MAG3     3
#define EI_CLASS    4
#define EI_VERSION  6

// Program header types
#define PT_NULL     0
#define PT_LOAD     1
#define PT_DYNAMIC  2
#define PT_INTERP   3

// Program header flags
#define PF_X        0x1
#define PF_W        0x2
#define PF_R        0x4

// ELF32 structures
typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_paddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

typedef struct {
    uint32_t a_type;
    union {
        uint32_t a_val;
        uint32_t a_ptr;
    } a_un;
} Elf32_auxv_t;

// Aux vector types
#define AT_NULL     0
#define AT_PHDR     3
#define AT_PHNUM    5
#define AT_PHENT    6
#define AT_ENTRY    9
#define AT_EXECFN   31
#define AT_BASE     7

// For 32-bit, use 32-bit structures
#define ELFCLASS ELFCLASS32
#define Elf_Ehdr Elf32_Ehdr
#define Elf_Phdr Elf32_Phdr
#define Elf_auxv_t Elf32_auxv_t

#endif

