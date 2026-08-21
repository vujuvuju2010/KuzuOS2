#include "keymap_loader.h"
#include "filesystem.h"
#include "memory.h"

keymap_t current_keymap;

static char keymap_base_dir[256] = "/dev/keys/";

static int fn_list[MAX_KEYMAP_FN];
static int fn_count;
static int fn_list_defined;
static int max_line_syms;

static int my_strcmp(const char* a, const char* b) {
    while (*a && *b && *a == *b) { a++; b++; }
    return *a - *b;
}

static int my_strncmp(const char* a, const char* b, int n) {
    for (int i = 0; i < n; i++) {
        if (a[i] != b[i]) return a[i] - b[i];
        if (a[i] == 0) return 0;
    }
    return 0;
}

static int my_strlen(const char* s) {
    int n = 0;
    while (s && s[n]) n++;
    return n;
}

static int my_atoi(const char* s) {
    int n = 0;
    while (*s >= '0' && *s <= '9') {
        n = n * 10 + (*s - '0');
        s++;
    }
    return n;
}

static void my_memset(void* p, int c, int n) {
    unsigned char* d = (unsigned char*)p;
    while (n--) *d++ = (unsigned char)c;
}

static char my_tolower(char c) {
    if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
    return c;
}

static char my_toupper(char c) {
    if (c >= 'a' && c <= 'z') return c - 'a' + 'A';
    return c;
}

static int mod_mask(int shift, int ctrl, int alt, int altgr) {
    int w = 0;
    if (shift) w |= 1;
    if (altgr) w |= 2;
    if (ctrl) w |= 4;
    if (alt) w |= 8;
    return w;
}

static void set_base_dir_from_path(const char* path) {
    int len = my_strlen(path);
    int slash = -1;
    for (int i = 0; i < len; i++) {
        if (path[i] == '/') slash = i;
    }
    if (slash < 0) {
        keymap_base_dir[0] = '/';
        keymap_base_dir[1] = '\0';
        return;
    }
    int i = 0;
    while (i <= slash && i < 255) {
        keymap_base_dir[i] = path[i];
        i++;
    }
    keymap_base_dir[i] = '\0';
}

static void build_include_path(const char* name, char* out, int out_len) {
    int pos = 0;
    for (int i = 0; keymap_base_dir[i] && pos < out_len - 1; i++)
        out[pos++] = keymap_base_dir[i];
    for (int i = 0; name[i] && name[i] != '"' && pos < out_len - 1; i++)
        out[pos++] = name[i];
    out[pos] = '\0';
}

static void set_fn_key(int fn, int keycode, uint16_t val) {
    if (fn < 0 || fn >= MAX_KEYMAP_FN || keycode < 0 || keycode >= MAX_KEYCODE)
        return;
    current_keymap.fn_map[fn][keycode] = val;
}

static void finalize_fn_list(void) {
    if (fn_list_defined) return;
    fn_count = max_line_syms;
    if (fn_count <= 0) fn_count = 1;
    if (fn_count > MAX_KEYMAP_FN) fn_count = MAX_KEYMAP_FN;
    for (int i = 0; i < fn_count; i++)
        fn_list[i] = i;
    fn_list_defined = 1;
}

static void parse_keymaps_directive(const char* line) {
    const char* p = line + 7;
    fn_count = 0;

    while (*p && fn_count < MAX_KEYMAP_FN) {
        while (*p == ' ' || *p == '\t' || *p == ',') p++;
        if (*p == '\0' || *p == '\n' || *p == '\r') break;

        int start = my_atoi(p);
        while (*p >= '0' && *p <= '9') p++;
        int end = start;
        if (*p == '-') {
            p++;
            end = my_atoi(p);
            while (*p >= '0' && *p <= '9') p++;
        }
        for (int fn = start; fn <= end && fn_count < MAX_KEYMAP_FN; fn++)
            fn_list[fn_count++] = fn;
    }
    fn_list_defined = 1;
}

static uint16_t parse_keysym(const char* sym_in) {
    const char* sym = sym_in;
    if (sym[0] == '+') sym++;

    if (sym[0] >= 'a' && sym[0] <= 'z' && sym[1] == '\0') return sym[0];
    if (sym[0] >= 'A' && sym[0] <= 'Z' && sym[1] == '\0') return sym[0];
    if (sym[0] >= '0' && sym[0] <= '9' && sym[1] == '\0') return sym[0];

    if (my_strcmp(sym, "VoidSymbol") == 0) return 0;

    if (my_strcmp(sym, "zero") == 0) return '0';
    if (my_strcmp(sym, "one") == 0) return '1';
    if (my_strcmp(sym, "two") == 0) return '2';
    if (my_strcmp(sym, "three") == 0) return '3';
    if (my_strcmp(sym, "four") == 0) return '4';
    if (my_strcmp(sym, "five") == 0) return '5';
    if (my_strcmp(sym, "six") == 0) return '6';
    if (my_strcmp(sym, "seven") == 0) return '7';
    if (my_strcmp(sym, "eight") == 0) return '8';
    if (my_strcmp(sym, "nine") == 0) return '9';

    if (my_strcmp(sym, "Escape") == 0) return 0x1B;
    if (my_strcmp(sym, "Return") == 0) return '\n';
    if (my_strcmp(sym, "Tab") == 0) return '\t';
    if (my_strcmp(sym, "Delete") == 0) return 0x7F;
    if (my_strcmp(sym, "BackSpace") == 0) return '\b';
    if (my_strcmp(sym, "Backtab") == 0) return '\t';
    if (my_strcmp(sym, "space") == 0) return ' ';

    if (my_strcmp(sym, "exclam") == 0) return '!';
    if (my_strcmp(sym, "at") == 0) return '@';
    if (my_strcmp(sym, "numbersign") == 0) return '#';
    if (my_strcmp(sym, "dollar") == 0) return '$';
    if (my_strcmp(sym, "percent") == 0) return '%';
    if (my_strcmp(sym, "asciicircum") == 0) return '^';
    if (my_strcmp(sym, "ampersand") == 0) return '&';
    if (my_strcmp(sym, "asterisk") == 0) return '*';
    if (my_strcmp(sym, "parenleft") == 0) return '(';
    if (my_strcmp(sym, "parenright") == 0) return ')';
    if (my_strcmp(sym, "minus") == 0) return '-';
    if (my_strcmp(sym, "underscore") == 0) return '_';
    if (my_strcmp(sym, "equal") == 0) return '=';
    if (my_strcmp(sym, "plus") == 0) return '+';
    if (my_strcmp(sym, "bracketleft") == 0) return '[';
    if (my_strcmp(sym, "bracketright") == 0) return ']';
    if (my_strcmp(sym, "braceleft") == 0) return '{';
    if (my_strcmp(sym, "braceright") == 0) return '}';
    if (my_strcmp(sym, "backslash") == 0) return '\\';
    if (my_strcmp(sym, "bar") == 0) return '|';
    if (my_strcmp(sym, "semicolon") == 0) return ';';
    if (my_strcmp(sym, "colon") == 0) return ':';
    if (my_strcmp(sym, "apostrophe") == 0) return '\'';
    if (my_strcmp(sym, "quotedbl") == 0) return '"';
    if (my_strcmp(sym, "grave") == 0) return '`';
    if (my_strcmp(sym, "asciitilde") == 0) return '~';
    if (my_strcmp(sym, "comma") == 0) return ',';
    if (my_strcmp(sym, "less") == 0) return '<';
    if (my_strcmp(sym, "period") == 0) return '.';
    if (my_strcmp(sym, "greater") == 0) return '>';
    if (my_strcmp(sym, "slash") == 0) return '/';
    if (my_strcmp(sym, "question") == 0) return '?';
    if (my_strcmp(sym, "diaeresis") == 0) return '"';
    if (my_strcmp(sym, "acute") == 0) return '\'';
    if (my_strcmp(sym, "sterling") == 0) return 0xA3;
    if (my_strcmp(sym, "onehalf") == 0) return 0xBD;
    if (my_strcmp(sym, "currency") == 0) return 0xA4;
    if (my_strcmp(sym, "cent") == 0) return 0xA2;
    if (my_strcmp(sym, "euro") == 0) return 0xA4;

    if (my_strcmp(sym, "dotlessi") == 0) return 0xFD;
    if (my_strcmp(sym, "I") == 0) return 'I';
    if (my_strcmp(sym, "Idotabove") == 0) return 0xDD;
    if (my_strcmp(sym, "gbreve") == 0) return 0xF0;
    if (my_strcmp(sym, "Gbreve") == 0) return 0xD0;
    if (my_strcmp(sym, "scedilla") == 0) return 0xFE;
    if (my_strcmp(sym, "Scedilla") == 0) return 0xDE;
    if (my_strcmp(sym, "odiaeresis") == 0) return 0xF6;
    if (my_strcmp(sym, "Odiaeresis") == 0) return 0xD6;
    if (my_strcmp(sym, "ccedilla") == 0) return 0xE7;
    if (my_strcmp(sym, "Ccedilla") == 0) return 0xC7;
    if (my_strcmp(sym, "udiaeresis") == 0) return 0xFC;
    if (my_strcmp(sym, "Udiaeresis") == 0) return 0xDC;
    if (my_strcmp(sym, "eacute") == 0) return 0xE9;
    if (my_strcmp(sym, "icircumflex") == 0) return 0xEE;
    if (my_strcmp(sym, "Icircumflex") == 0) return 0xCE;
    if (my_strcmp(sym, "ocircumflex") == 0) return 0xF6;
    if (my_strcmp(sym, "Ocircumflex") == 0) return 0xD6;
    if (my_strcmp(sym, "ucircumflex") == 0) return 0xFB;
    if (my_strcmp(sym, "Ucircumflex") == 0) return 0xDB;
    if (my_strcmp(sym, "acircumflex") == 0) return 0xE2;
    if (my_strcmp(sym, "Acircumflex") == 0) return 0xC2;

    if (my_strncmp(sym, "KP_", 3) == 0) {
        if (my_strcmp(sym, "KP_0") == 0) return '0';
        if (my_strcmp(sym, "KP_1") == 0) return '1';
        if (my_strcmp(sym, "KP_2") == 0) return '2';
        if (my_strcmp(sym, "KP_3") == 0) return '3';
        if (my_strcmp(sym, "KP_4") == 0) return '4';
        if (my_strcmp(sym, "KP_5") == 0) return '5';
        if (my_strcmp(sym, "KP_6") == 0) return '6';
        if (my_strcmp(sym, "KP_7") == 0) return '7';
        if (my_strcmp(sym, "KP_8") == 0) return '8';
        if (my_strcmp(sym, "KP_9") == 0) return '9';
        if (my_strcmp(sym, "KP_Add") == 0) return '+';
        if (my_strcmp(sym, "KP_Subtract") == 0) return '-';
        if (my_strcmp(sym, "KP_Multiply") == 0) return '*';
        if (my_strcmp(sym, "KP_Divide") == 0) return '/';
        if (my_strcmp(sym, "KP_Enter") == 0) return '\n';
        if (my_strcmp(sym, "KP_Period") == 0) return '.';
        if (my_strcmp(sym, "KP_Comma") == 0) return ',';
        if (my_strcmp(sym, "KP_Equal") == 0) return '=';
    }

    if (my_strncmp(sym, "Ascii_", 6) == 0) {
        int n = my_atoi(sym + 6);
        if (n >= 0 && n < 256) return (uint16_t)n;
    }

    if (my_strncmp(sym, "Hex_", 4) == 0) {
        const char* h = sym + 4;
        if (h[0] >= '0' && h[0] <= '9' && h[1] == '\0') return h[0];
        if (h[0] >= 'A' && h[0] <= 'F' && h[1] == '\0') return h[0];
    }

    if (sym[0] == 'F' && sym[1] >= '1' && sym[1] <= '9') {
        if (sym[2] == '\0') return 0xF000 + (sym[1] - '0');
        if (sym[1] == '1' && (sym[2] == '0' || sym[2] == '1' || sym[2] == '2') && sym[3] == '\0')
            return 0xF000 + 10 + (sym[2] - '0');
    }

    if (my_strcmp(sym, "nul") == 0) return 0;
    if (my_strcmp(sym, "Control_backslash") == 0) return 0x1C;
    if (my_strcmp(sym, "Control_underscore") == 0) return 0x1F;
    if (my_strcmp(sym, "Control_bracketright") == 0) return 0x1D;
    if (my_strcmp(sym, "Control_asciicircum") == 0) return 0x1E;
    if (my_strcmp(sym, "Control_g") == 0) return 0x07;

    if (my_strncmp(sym, "Control_", 8) == 0) {
        char ch = sym[8];
        if (ch >= 'a' && ch <= 'z') return (uint16_t)(ch - 0x60);
        if (ch >= 'A' && ch <= 'Z') return (uint16_t)(my_tolower(ch) - 0x60);
    }

    if (my_strncmp(sym, "Shift", 5) == 0) return 0;
    if (my_strncmp(sym, "Control", 7) == 0) return 0;
    if (my_strncmp(sym, "Alt", 3) == 0) return 0;
    if (my_strncmp(sym, "Meta_", 5) == 0) return 0;
    if (my_strcmp(sym, "Caps_Lock") == 0) return 0;
    if (my_strcmp(sym, "Num_Lock") == 0) return 0;
    if (my_strcmp(sym, "Scroll_Lock") == 0) return 0;
    if (my_strcmp(sym, "AltGr") == 0) return 0;
    if (my_strcmp(sym, "Compose") == 0) return 0;
    if (my_strncmp(sym, "Console_", 8) == 0) return 0;
    if (my_strcmp(sym, "Boot") == 0) return 0;
    if (my_strcmp(sym, "Show_Registers") == 0) return 0;

    return 0;
}

static void apply_single_letter_rule(int keycode, uint16_t val) {
    finalize_fn_list();
    char lower = my_tolower((char)val);
    char upper = my_toupper((char)val);

    for (int i = 0; i < fn_count; i++) {
        int fn = fn_list[i];
        if (fn == 0)
            set_fn_key(fn, keycode, lower);
        else if (fn == 1)
            set_fn_key(fn, keycode, upper);
        else
            set_fn_key(fn, keycode, 0);
    }
}

static void apply_keycode_syms(int keycode, uint16_t* syms, int sym_count, int explicit_fn)
{
    finalize_fn_list();

    if (sym_count == 1 && explicit_fn < 0) {
        uint16_t val = syms[0];
        if (val >= 'a' && val <= 'z') {
            apply_single_letter_rule(keycode, val);
            return;
        }
        if (val >= 'A' && val <= 'Z') {
            for (int i = 0; i < fn_count; i++) {
                int fn = fn_list[i];
                if (fn == 0)
                    set_fn_key(fn, keycode, val);
                else if (fn == 1)
                    set_fn_key(fn, keycode, my_tolower((char)val));
                else
                    set_fn_key(fn, keycode, 0);
            }
            return;
        }
        for (int i = 0; i < fn_count; i++)
            set_fn_key(fn_list[i], keycode, val);
        return;
    }

    if (explicit_fn >= 0) {
        set_fn_key(explicit_fn, keycode, syms[0]);
        return;
    }

    for (int i = 0; i < sym_count && i < fn_count; i++)
        set_fn_key(fn_list[i], keycode, syms[i]);
}

static int try_load_path(const char* path, int depth);

static int load_include_file(const char* inc_name, int depth)
{
    char path[256];
    const char* suffixes[] = { "", ".inc", ".map", 0 };
    char alt_name[128];

    build_include_path(inc_name, path, sizeof(path));
    if (try_load_path(path, depth) == 0) return 0;

    for (int s = 0; suffixes[s]; s++) {
        build_include_path(inc_name, path, sizeof(path));
        int pos = my_strlen(path);
        const char* suf = suffixes[s];
        for (int i = 0; suf[i] && pos < 255; i++) path[pos++] = suf[i];
        path[pos] = '\0';
        if (try_load_path(path, depth) == 0) return 0;
    }

    int i = 0;
    while (inc_name[i] && i < 127) {
        alt_name[i] = (inc_name[i] == '-') ? '_' : inc_name[i];
        i++;
    }
    alt_name[i] = '\0';
    if (my_strcmp(alt_name, inc_name) != 0) {
        build_include_path(alt_name, path, sizeof(path));
        if (try_load_path(path, depth) == 0) return 0;
        for (int s = 0; suffixes[s]; s++) {
            build_include_path(alt_name, path, sizeof(path));
            int pos = my_strlen(path);
            const char* suf = suffixes[s];
            for (int j = 0; suf[j] && pos < 255; j++) path[pos++] = suf[j];
            path[pos] = '\0';
            if (try_load_path(path, depth) == 0) return 0;
        }
    }

    return 0;
}

static int parse_keymap_buffer(char* file_content, int file_size, int depth);
static int parse_keymap_line(char* line, int depth);

static int parse_keymap_line(char* line, int depth)
{
    if (line[0] == '#' || line[0] == '\n' || line[0] == '\r') return 0;
    if (my_strncmp(line, "keymaps", 7) == 0) {
        parse_keymaps_directive(line);
        return 0;
    }
    if (my_strncmp(line, "alt_is_meta", 11) == 0) return 0;
    if (my_strncmp(line, "strings", 7) == 0) return 0;
    if (my_strncmp(line, "compose", 7) == 0) return 0;
    if (my_strncmp(line, "charset", 7) == 0) return 0;

    if (my_strncmp(line, "include", 7) == 0) {
        char* q = line + 7;
        while (*q && *q != '"') q++;
        if (*q != '"') return 0;
        q++;
        char inc_name[128];
        int i = 0;
        while (*q && *q != '"' && i < 127) inc_name[i++] = *q++;
        inc_name[i] = '\0';
        if (i == 0) return 0;
        return load_include_file(inc_name, depth);
    }

    int is_shift = 0, is_ctrl = 0, is_alt = 0, is_altgr = 0;
    char* p = line;
    while (*p == ' ' || *p == '\t') p++;

    if (my_strncmp(p, "plain", 5) == 0) {
        p += 5;
        while (*p == ' ' || *p == '\t') p++;
    }

    while (1) {
        if (my_strncmp(p, "control", 7) == 0) {
            is_ctrl = 1;
            p += 7;
        } else if (my_strncmp(p, "shift", 5) == 0) {
            is_shift = 1;
            p += 5;
        } else if (my_strncmp(p, "altgr", 5) == 0) {
            is_altgr = 1;
            p += 5;
        } else if (my_strncmp(p, "alt", 3) == 0) {
            is_alt = 1;
            p += 3;
        } else {
            break;
        }
        while (*p == ' ' || *p == '\t') p++;
    }

    if (my_strncmp(p, "keycode", 7) != 0) return 0;
    p += 7;
    while (*p == ' ' || *p == '\t') p++;

    int keycode = my_atoi(p);
    if (keycode < 0 || keycode >= MAX_KEYCODE) return 0;

    while (*p && *p != '=') p++;
    if (*p != '=') return 0;
    p++;

    uint16_t syms[16];
    int sym_count = 0;
    while (*p && sym_count < 16) {
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\n' || *p == '\r' || *p == '\0') break;

        char sym[64];
        int i = 0;
        while (*p && *p != ' ' && *p != '\t' && *p != '\n' && *p != '\r' && i < 63) {
            sym[i++] = *p++;
        }
        sym[i] = '\0';
        if (i == 0) break;

        syms[sym_count++] = parse_keysym(sym);
    }

    if (sym_count == 0) return 0;
    if (sym_count > max_line_syms) max_line_syms = sym_count;

    int explicit_fn = -1;
    if (is_shift || is_ctrl || is_alt || is_altgr)
        explicit_fn = mod_mask(is_shift, is_ctrl, is_alt, is_altgr);

    apply_keycode_syms(keycode, syms, sym_count, explicit_fn);
    return 0;
}

static int parse_keymap_buffer(char* file_content, int file_size, int depth)
{
    if (depth > 16) return -1;

    char line[512];
    int file_pos = 0;

    while (file_pos < file_size) {
        int line_idx = 0;
        while (file_pos < file_size && line_idx < 511) {
            char c = file_content[file_pos++];
            line[line_idx++] = c;
            if (c == '\n') break;
        }
        line[line_idx] = '\0';
        if (line_idx == 0) break;
        if (parse_keymap_line(line, depth) < 0) return -1;
    }
    return 0;
}

static int try_load_path(const char* path, int depth)
{
    int file_size = fs_get_file_size((char*)path);
    if (file_size < 0) return -1;

    char* file_content = (char*)kmalloc(file_size + 1);
    if (!file_content) return -1;

    int bytes_read = fs_read_file((char*)path, file_content, file_size);
    if (bytes_read < 0) {
        kfree(file_content);
        return -1;
    }
    file_content[bytes_read] = '\0';
    int rc = parse_keymap_buffer(file_content, bytes_read, depth);
    kfree(file_content);
    return rc;
}

int keymap_load(const char* path) {
    set_base_dir_from_path(path);
    fn_count = 0;
    fn_list_defined = 0;
    max_line_syms = 0;
    my_memset(&current_keymap, 0, sizeof(keymap_t));

    int file_size = fs_get_file_size((char*)path);
    if (file_size < 0) return -1;

    char* file_content = (char*)kmalloc(file_size + 1);
    if (!file_content) return -1;

    int bytes_read = fs_read_file((char*)path, file_content, file_size);
    if (bytes_read < 0) {
        kfree(file_content);
        return -1;
    }
    file_content[bytes_read] = '\0';

    int rc = parse_keymap_buffer(file_content, bytes_read, 0);
    kfree(file_content);
    return rc;
}

uint16_t keymap_get_char(uint8_t keycode, uint8_t shift, uint8_t ctrl, uint8_t alt, uint8_t altgr) {
    if (keycode >= MAX_KEYCODE) return 0;

    int fn = mod_mask(shift, ctrl, alt, altgr);
    uint16_t c = current_keymap.fn_map[fn][keycode];
    if (c == 0 && fn != 0)
        return 0;
    return c;
}

void load_us_keymap(void)
{
    keymap_load("/dev/keys/us.map");
}
