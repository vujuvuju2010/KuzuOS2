/*TTY DRIVERS FOR KUZUOS 2 YAYYYYAYAYAYAYİ
tbh im really bored of this usb thing so this is written by gpt enjoy
*/

#include "tty.h"
#include "z_utils.h"

static char input_buffer[256];
static int input_pos = 0;
static int line_ready = 0;

void tty_putchar(char c)
{
    z_printf("%c", c);
}

void tty_backspace(void)
{
    if(input_pos > 0)
    {
        input_pos--;

        // erase visually
        z_printf("\b \b");
    }
}

void tty_keyboard_input(char c)
{
    if(c == '\b')
    {
        tty_backspace();
        return;
    }

    if(c == '\n')
    {
        input_buffer[input_pos] = 0;

        tty_putchar('\n');

        line_ready = 1;
        return;
    }

    if(input_pos < 255)
    {
        input_buffer[input_pos++] = c;
        tty_putchar(c);
    }
}

void tty_getline(char* buf, int max)
{
    while(!line_ready);

    for(int i=0;i<input_pos && i<max-1;i++)
        buf[i] = input_buffer[i];

    buf[input_pos] = 0;

    input_pos = 0;
    line_ready = 0;
}