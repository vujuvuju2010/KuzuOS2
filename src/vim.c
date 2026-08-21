// KuzuOS için minnoş bir vim
// a cute vim clone for KuzuOS which can be called kvim or kuzuvim or whatever the fuck the knuckefuck thats reading this has decided to call it
// DEBUG MODE DONT FORGET TO MMAKE THE REAl VERSION
#include "z_syscalls.h"
#undef O_RDONLY
#undef O_WRONLY
#undef O_RDWR
#undef O_CREAT
#undef O_TRUNC
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR   0x0002
#define O_CREAT  0x0040
#define O_TRUNC  0x0200
#define MAX_LINES 2000
#define MAX_LINE_LENGTH 512
#define SCREEN_ROWS 23
#define SCREEN_COLS 80
#define MAX_UNDO 50

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}
static void* memset(void* s, int c, int n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}
static void* memcpy(void* dest, const void* src, int n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) *d++ = *s++;
    return dest;
}
static int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (unsigned char)*s1 - (unsigned char)*s2;
}
static int strncmp(const char* s1, const char* s2, int n) {
    while (n && *s1 && (*s1 == *s2)) { s1++; s2++; n--; }
    if (n == 0) return 0;
    return (unsigned char)*s1 - (unsigned char)*s2;
}
static char* strcpy(char* dest, const char* src) {
    char* ret = dest;
    while ((*dest++ = *src++));
    return ret;
}
static void ws(const char* s) { z_write(1, s, strlen(s)); }
static void wc(char c) { z_write(1, &c, 1); }
static void cls(void) { ws("\033[2J\033[H"); }
static void cpos(int row, int col) {
    char b[32]; char* p = b;
    *p++ = '\033'; *p++ = '[';
    if (row >= 10) *p++ = '0' + (row / 10);
    *p++ = '0' + (row % 10); *p++ = ';';
    if (col >= 10) *p++ = '0' + (col / 10);
    *p++ = '0' + (col % 10); *p++ = 'H';
    z_write(1, b, p - b);
}
static void clr_line(void) { ws("\033[K"); }
static void rev(void) { ws("\033[7m"); }
static void nrm(void) { ws("\033[0m"); }
typedef struct { char data[MAX_LINE_LENGTH]; int length; } Line;
typedef struct {
    Line lines[MAX_LINES];
    int num_lines;
    int cx, cy;
    int top;
    int mod;
    char fname[256];
    int vsx, vsy;
    Line yank[10];
    int ylines;
} Buf;
typedef struct { Line lines[MAX_LINES]; int n, cx, cy; } Undo;
static Buf b;
static Undo us[MAX_UNDO];
static int utop = 0, ucnt = 0;
typedef enum { M_NORM, M_INS, M_VIS, M_CMD, M_SRCH } Mode;
static Mode mode = M_NORM;
static char cmd[256];
static int clen = 0;
static char smsg[256] = "";
static char pend = 0;
static void binit(void) {
    memset(&b, 0, sizeof(b));
    b.num_lines = 1;
    b.lines[0].length = 0;
    b.lines[0].data[0] = '\0';
}
static void svu(void) {
    Undo* u = &us[utop];
    int n = b.num_lines > MAX_LINES ? MAX_LINES : b.num_lines;
    for (int i = 0; i < n; i++) u->lines[i] = b.lines[i];
    u->n = b.num_lines; u->cx = b.cx; u->cy = b.cy;
    utop = (utop + 1) % MAX_UNDO;
    if (ucnt < MAX_UNDO) ucnt++;
}
static void dou(void) {
    if (ucnt == 0) return;
    utop = (utop - 1 + MAX_UNDO) % MAX_UNDO; ucnt--;
    Undo* u = &us[utop];
    for (int i = 0; i < u->n; i++) b.lines[i] = u->lines[i];
    b.num_lines = u->n; b.cx = u->cx; b.cy = u->cy; b.mod = 1;
}
static void insc(char c) {
    svu(); Line* l = &b.lines[b.cy];
    if (l->length >= MAX_LINE_LENGTH - 1) return;
    for (int i = l->length; i > b.cx; i--) l->data[i] = l->data[i-1];
    l->data[b.cx] = c; l->length++; l->data[l->length] = '\0';
    b.cx++; b.mod = 1;
}
static void delc(void) {
    svu(); Line* l = &b.lines[b.cy];
    if (b.cx == 0) {
        if (b.cy > 0) {
            Line* p = &b.lines[b.cy - 1];
            int ol = p->length;
            if (p->length + l->length < MAX_LINE_LENGTH) {
                memcpy(p->data + p->length, l->data, l->length + 1);
                p->length += l->length;
                for (int i = b.cy; i < b.num_lines - 1; i++) b.lines[i] = b.lines[i+1];
                b.num_lines--; b.cy--; b.cx = ol; b.mod = 1;
            }
        }
        return;
    }
    for (int i = b.cx - 1; i < l->length - 1; i++) l->data[i] = l->data[i+1];
    l->length--; l->data[l->length] = '\0'; b.cx--; b.mod = 1;
}
static void delf(void) {
    svu(); Line* l = &b.lines[b.cy];
    if (b.cx >= l->length) {
        if (b.cy < b.num_lines - 1) {
            Line* n = &b.lines[b.cy + 1];
            if (l->length + n->length < MAX_LINE_LENGTH) {
                memcpy(l->data + l->length, n->data, n->length + 1);
                l->length += n->length;
                for (int i = b.cy + 1; i < b.num_lines - 1; i++) b.lines[i] = b.lines[i+1];
                b.num_lines--; b.mod = 1;
            }
        }
        return;
    }
    for (int i = b.cx; i < l->length - 1; i++) l->data[i] = l->data[i+1];
    l->length--; l->data[l->length] = '\0'; b.mod = 1;
}
static void newl(void) {
    svu();
    if (b.num_lines >= MAX_LINES) return;
    Line* l = &b.lines[b.cy];
    for (int i = b.num_lines; i > b.cy + 1; i--) b.lines[i] = b.lines[i-1];
    b.num_lines++;
    Line* nl = &b.lines[b.cy + 1];
    nl->length = l->length - b.cx;
    memcpy(nl->data, l->data + b.cx, nl->length + 1);
    l->length = b.cx; l->data[l->length] = '\0';
    b.cy++; b.cx = 0; b.mod = 1;
}
static void dell(void) {
    svu();
    if (b.num_lines == 1) {
        b.lines[0].length = 0; b.lines[0].data[0] = '\0'; b.cx = 0;
    } else {
        for (int i = b.cy; i < b.num_lines - 1; i++) b.lines[i] = b.lines[i+1];
        b.num_lines--;
        if (b.cy >= b.num_lines) b.cy = b.num_lines - 1;
        b.cx = 0;
    }
    b.mod = 1;
}
static void yankl(void) { b.ylines = 1; b.yank[0] = b.lines[b.cy]; }
static void pasteb(void) {
    svu();
    for (int i = 0; i < b.ylines && b.num_lines < MAX_LINES; i++) {
        for (int j = b.num_lines; j > b.cy + 1 + i; j--) b.lines[j] = b.lines[j-1];
        b.lines[b.cy + 1 + i] = b.yank[i]; b.num_lines++;
    }
    b.mod = 1;
}
static void pastea(void) {
    svu();
    for (int i = 0; i < b.ylines && b.num_lines < MAX_LINES; i++) {
        for (int j = b.num_lines; j > b.cy + i; j--) b.lines[j] = b.lines[j-1];
        b.lines[b.cy + i] = b.yank[i]; b.num_lines++;
    }
    b.mod = 1;
}
static void stat(void) {
    cpos(24, 1); rev();
    char s[SCREEN_COLS + 1]; memset(s, ' ', SCREEN_COLS); s[SCREEN_COLS] = '\0';
    if (mode == M_INS) memcpy(s, "-- INSERT --", 12);
    else if (mode == M_VIS) memcpy(s, "-- VISUAL --", 12);
    else if (mode == M_CMD) { s[0] = ';'; if (clen > 0) memcpy(s + 1, cmd, clen < SCREEN_COLS - 1 ? clen : SCREEN_COLS - 1); }
    else if (mode == M_SRCH) { s[0] = '/'; if (clen > 0) memcpy(s + 1, cmd, clen < SCREEN_COLS - 1 ? clen : SCREEN_COLS - 1); }
    else {
        char* p = s;
        if (b.fname[0]) { int fl = strlen(b.fname); if (fl > 30) fl = 30; memcpy(p, b.fname, fl); p += fl; }
        else { memcpy(p, "[No Name]", 9); p += 9; }
        if (b.mod) { *p++ = ' '; *p++ = '['; *p++ = '+'; *p++ = ']'; }
        *p++ = ' '; *p++ = 'L';
        int ln = b.cy + 1;
        if (ln >= 100) *p++ = '0' + (ln / 100);
        if (ln >= 10) *p++ = '0' + ((ln / 10) % 10);
        *p++ = '0' + (ln % 10); *p++ = '/';
        int t = b.num_lines;
        if (t >= 100) *p++ = '0' + (t / 100);
        if (t >= 10) *p++ = '0' + ((t / 10) % 10);
        *p++ = '0' + (t % 10);
    }
    z_write(1, s, SCREEN_COLS); nrm();
    cpos(25, 1);
    if (smsg[0]) ws(smsg);
    clr_line();
}
static int insel(int l, int c) {
    if (mode != M_VIS) return 0;
    int sy = b.vsy, ey = b.cy, sx = b.vsx, ex = b.cx;
    if (sy > ey || (sy == ey && sx > ex)) {
        int t = sy; sy = ey; ey = t;
        t = sx; sx = ex; ex = t;
    }
    if (l < sy || l > ey) return 0;
    if (l == sy && l == ey) return c >= sx && c <= ex;
    if (l == sy) return c >= sx;
    if (l == ey) return c <= ex;
    return 1;
}
static void refr(void) {
    ws("\033[H");
    for (int i = 0; i < SCREEN_ROWS; i++) {
        int fl = b.top + i;
        cpos(i + 1, 1); clr_line();
        if (fl < b.num_lines) {
            Line* l = &b.lines[fl];
            for (int c = 0; c < l->length && c < SCREEN_COLS; c++) {
                if (insel(fl, c)) { rev(); wc(l->data[c]); nrm(); }
                else wc(l->data[c]);
            }
        } else wc('~');
    }
    stat();
    int sy = b.cy - b.top + 1, sx = b.cx + 1;
    if (sx > SCREEN_COLS) sx = SCREEN_COLS;
    if (sy < 1) sy = 1;
    if (sy > SCREEN_ROWS) sy = SCREEN_ROWS;
    cpos(sy, sx);
}
static void load(const char* fn) {
    int fd = z_open(fn, O_RDONLY);
    if (fd < 0) { strcpy(b.fname, fn); return; }
    strcpy(b.fname, fn);
    char rb[4096]; int br, li = 0, c = 0;
    b.num_lines = 0;
    while ((br = z_read(fd, rb, sizeof(rb))) > 0) {
        for (int i = 0; i < br; i++) {
            char ch = rb[i];
            if (ch == '\n') {
                if (b.num_lines >= MAX_LINES) break;
                b.lines[li].data[c] = '\0'; b.lines[li].length = c;
                li++; b.num_lines++; c = 0;
            } else if (ch != '\r' && c < MAX_LINE_LENGTH - 1) b.lines[li].data[c++] = ch;
        }
    }
    if (c > 0 || b.num_lines == 0) {
        b.lines[li].data[c] = '\0'; b.lines[li].length = c; b.num_lines++;
    }
    z_close(fd); b.mod = 0;
}
static int save(void) {
    if (!b.fname[0]) { strcpy(smsg, "No filename"); return -1; }
    int fd = z_open(b.fname, O_WRONLY | O_CREAT | O_TRUNC);
    if (fd < 0) { strcpy(smsg, "Cannot write"); return -1; }
    for (int i = 0; i < b.num_lines; i++) {
        Line* l = &b.lines[i];
        if (l->length > 0) z_write(fd, l->data, l->length);
        if (i < b.num_lines - 1) z_write(fd, "\n", 1);
    }
    z_close(fd); b.mod = 0; strcpy(smsg, "Saved"); return 0;
}
static void xcmd(void) {
    if (clen == 0) { mode = M_NORM; return; }
    cmd[clen] = '\0';
    if (strcmp(cmd, "q") == 0) {
        if (b.mod) { strcpy(smsg, "Modified! Use ;q!"); mode = M_NORM; return; }
        cls(); z_exit(0);
    } else if (strcmp(cmd, "q!") == 0) { cls(); z_exit(0); }
    else if (strcmp(cmd, "w") == 0) { save(); mode = M_NORM; }
    else if (strcmp(cmd, "wq") == 0) { if (save() == 0) { cls(); z_exit(0); } mode = M_NORM; }
    else if (strncmp(cmd, "w ", 2) == 0) { strcpy(b.fname, cmd + 2); save(); mode = M_NORM; }
    else { strcpy(smsg, "Unknown"); mode = M_NORM; }
    clen = 0;
}
static void scrl(void) {
    if (b.cy >= b.top + SCREEN_ROWS) b.top = b.cy - SCREEN_ROWS + 1;
    if (b.cy < b.top) b.top = b.cy;
}
static void hnorm(char c) {
    smsg[0] = '\0';
    if (pend) {
        if (c == pend) {
            if (pend == 'd') dell();
            else if (pend == 'y') { yankl(); strcpy(smsg, "Yanked"); }
        }
        pend = 0; return;
    }
    if (c == 'i') { mode = M_INS; return; }
    if (c == 'a') { mode = M_INS; if (b.cx < b.lines[b.cy].length) b.cx++; return; }
    if (c == 'A') { mode = M_INS; b.cx = b.lines[b.cy].length; return; }
    if (c == 'I') { mode = M_INS; b.cx = 0; return; }
    if (c == 'o') { mode = M_INS; b.cx = b.lines[b.cy].length; newl(); return; }
    if (c == 'O') {
        mode = M_INS;
        if (b.num_lines >= MAX_LINES) return;
        for (int i = b.num_lines; i > b.cy; i--) b.lines[i] = b.lines[i-1];
        b.num_lines++; b.lines[b.cy].length = 0; b.lines[b.cy].data[0] = '\0';
        b.cx = 0; b.mod = 1; return;
    }
    if (c == 'h' && b.cx > 0) b.cx--;
    else if (c == 'j' && b.cy < b.num_lines - 1) {
        b.cy++; Line* l = &b.lines[b.cy];
        if (b.cx > l->length) b.cx = l->length;
        scrl();
    }
    else if (c == 'k' && b.cy > 0) {
        b.cy--; Line* l = &b.lines[b.cy];
        if (b.cx > l->length) b.cx = l->length;
        scrl();
    }
    else if (c == 'l' && b.cx < b.lines[b.cy].length) b.cx++;
    else if (c == '0') b.cx = 0;
    else if (c == '$') b.cx = b.lines[b.cy].length;
    else if (c == 'w') {
        if (b.cx < b.lines[b.cy].length) {
            b.cx++;
            while (b.cx < b.lines[b.cy].length && b.lines[b.cy].data[b.cx] != ' ') b.cx++;
            while (b.cx < b.lines[b.cy].length && b.lines[b.cy].data[b.cx] == ' ') b.cx++;
        }
    }
    else if (c == 'b') {
        if (b.cx > 0) {
            b.cx--;
            while (b.cx > 0 && b.lines[b.cy].data[b.cx] == ' ') b.cx--;
            while (b.cx > 0 && b.lines[b.cy].data[b.cx - 1] != ' ') b.cx--;
        }
    }
    else if (c == 'x') delf();
    else if (c == 'd') pend = 'd';
    else if (c == 'y') pend = 'y';
    else if (c == 'p') pasteb();
    else if (c == 'P') pastea();
    else if (c == 'u') { dou(); strcpy(smsg, "Undo"); }
    else if (c == 'v') { mode = M_VIS; b.vsx = b.cx; b.vsy = b.cy; }
    else if (c == ';') { mode = M_CMD; clen = 0; cmd[0] = '\0'; }
    else if (c == 'g') { b.cy = 0; b.cx = 0; b.top = 0; }
    else if (c == 'G') { b.cy = b.num_lines - 1; b.cx = 0; scrl(); }
}
static void hins(char c) {
    if (c == 27) {
        mode = M_NORM;
        if (b.cx > 0 && b.cx >= b.lines[b.cy].length) b.cx--;
    } else if (c == '\n' || c == '\r') { newl(); scrl(); }
    else if (c == 8 || c == 127) delc();
    else if (c >= 32 && c < 127) insc(c);
}
static void hvis(char c) {
    if (c == 27 || c == 'v') mode = M_NORM;
    else if (c == 'd') { dell(); mode = M_NORM; }
    else if (c == 'y') { yankl(); strcpy(smsg, "Yanked"); mode = M_NORM; }
    else { hnorm(c); mode = M_VIS; }
}
static void hcmd(char c) {
    if (c == 27) { mode = M_NORM; clen = 0; }
    else if (c == '\n' || c == '\r') xcmd();
    else if (c == 8 || c == 127) { if (clen > 0) { clen--; cmd[clen] = '\0'; } else mode = M_NORM; }
    else if (c >= 32 && c < 127 && clen < 255) { cmd[clen++] = c; cmd[clen] = '\0'; }
}
void _start(void) {
    binit();
    cls();
    strcpy(smsg, "KuzuVIM - Press i=INSERT v=VISUAL ;=COMMAND");
    char ib[256];
    while (1) {
        refr();
        int by = z_read(0, ib, sizeof(ib));
        if (by <= 0) continue;
        for (int i = 0; i < by; i++) {
            char c = ib[i];
            if (mode == M_NORM) hnorm(c);
            else if (mode == M_INS) hins(c);
            else if (mode == M_VIS) hvis(c);
            else if (mode == M_CMD) hcmd(c);
        }
    }
}