#include "../libc/include/stdio.h"
#include "../libc/include/stdlib.h"
#include "../libc/include/string.h"
#include "../libc/include/fcntl.h"
#include "../libc/include/unistd.h"
#include "../src/z_syscalls.h"

#define DEFAULT_KEYS_DIR "/dev/keys/"
#define SYSCALL_LOADKEYMAP 251

void print_help(void) {
    printf("loadkeys - Load keyboard layout configuration\n\n");
    printf("Usage:\n");
    printf("  loadkeys <keymap>          Load keymap from /dev/keys/\n");
    printf("  loadkeys -d <dir> <keymap> Load keymap from custom directory\n");
    printf("  loadkeys -h                Display this help text\n\n");
    printf("Parameters:\n");
    printf("  -h         Show help text with supported parameters\n");
    printf("  -d <dir>   Specify custom directory instead of /dev/keys/\n\n");
    printf("Examples:\n");
    printf("  loadkeys us                Load US layout from /dev/keys/us.map\n");
    printf("  loadkeys -d /custom trq   Load from /custom/tr-q.map\n\n");
}

int syscall_loadkeymap(const char* path) {
    int result;
    __asm__ volatile(
        "int $0x80"
        : "=a"(result)
        : "a"(SYSCALL_LOADKEYMAP), "b"(path)
        : "memory"
    );
    return result;
}

int load_keymap_file(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        printf("Error: Cannot open keymap file: %s\n", path);
        return -1;
    }
    close(fd);

    int result = syscall_loadkeymap(path);
    if (result < 0) {
        printf("Error: Failed to load keymap (kernel returned %d)\n", result);
        return -1;
    }

    printf("Successfully loaded keymap: %s\n", path);
    return 0;
}

static int loadkeys_main(int argc, char* argv[]) {
    int argi = 1;

    /* After z_entry argv-shift bug: argc=1 argv[0]=/dev/loadkeys only */
    if (argc >= 2 && argv[0] && argv[0][0] == '/')
        argi = 1;
    else if (argc >= 3 && argv[1] && argv[1][0] == '/')
        argi = 2;

    if (argc <= argi) {
        printf("Error: Missing keymap argument\n\n");
        print_help();
        return 1;
    }

    const char* keys_dir = DEFAULT_KEYS_DIR;
    const char* keymap_name = NULL;

    for (int i = argi; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            print_help();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0) {
            if (i + 1 >= argc) {
                printf("Error: -d requires a directory argument\n");
                return 1;
            }
            keys_dir = argv[++i];
        } else {
            keymap_name = argv[i];
        }
    }

    if (keymap_name == NULL) {
        printf("Error: No keymap specified\n\n");
        print_help();
        return 1;
    }

    char full_path[256];
    int pos = 0;

    for (int i = 0; keys_dir[i] && pos < 255; i++)
        full_path[pos++] = keys_dir[i];

    if (pos > 0 && full_path[pos - 1] != '/' && pos < 255)
        full_path[pos++] = '/';

    for (int i = 0; keymap_name[i] && pos < 255; i++)
        full_path[pos++] = keymap_name[i];
    full_path[pos] = '\0';

    int name_len = strlen(keymap_name);
    if (name_len < 4 || strcmp(keymap_name + name_len - 4, ".map") != 0) {
        if (pos + 4 < 255) {
            full_path[pos++] = '.';
            full_path[pos++] = 'm';
            full_path[pos++] = 'a';
            full_path[pos++] = 'p';
            full_path[pos] = '\0';
        }
    }

    return load_keymap_file(full_path);
}

void _start(void) {
    int argc;
    char** argv;

    __asm__ volatile(
        "mov (%%rsp), %0\n"
        "lea 8(%%rsp), %1\n"
        : "=r"(argc), "=r"(argv)
    );

    int ret = loadkeys_main(argc, argv);
    z_exit(ret);
}
