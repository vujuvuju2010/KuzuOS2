#include "z_syscalls.h"

static int strlen(const char* s) {
    int len = 0;
    while (s[len]) len++;
    return len;
}

static void print(const char* s) {
    z_write(1, s, strlen(s));
}

static void print_num(int n) {
    if (n == 0) {
        print("0");
        return;
    }
    
    if (n < 0) {
        print("-");
        n = -n;
    }
    
    char buf[16];
    int pos = 0;
    while (n > 0) {
        buf[pos++] = '0' + (n % 10);
        n /= 10;
    }
    
    // FAH
    while (pos > 0) {
        char c = buf[--pos];
        z_write(1, &c, 1);
    }
}

// intsel yknow
static int str_to_int(const char* s) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    // ya bunadda coment isteme bizahmmet 
    while (s[i] == ' ') i++;
    
    
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (s[i] == '+') {
        i++;
    }
    
    
    while (s[i] >= '0' && s[i] <= '9') {
        result = result * 10 + (s[i] - '0');
        i++;
    }
    
    return result * sign;
}

static int calculate(const char* input) {
    int num1 = 0, num2 = 0;
    char op = 0;
    int i = 0;
    
    while (input[i] == ' ') i++;
    
    int sign1 = 1;
    if (input[i] == '-') {
        sign1 = -1;
        i++;
    }
    while (input[i] >= '0' && input[i] <= '9') {
        num1 = num1 * 10 + (input[i] - '0');
        i++;
    }
    num1 *= sign1;
    
    while (input[i] == ' ') i++;
    
    if (input[i] == '+' || input[i] == '-' || input[i] == '*' || 
        input[i] == '/' || input[i] == '%') {
        op = input[i];
        i++;
    } else {
        return num1;  
    }
    
    while (input[i] == ' ') i++;
    
    int sign2 = 1;
    if (input[i] == '-') {
        sign2 = -1;
        i++;
    }
    while (input[i] >= '0' && input[i] <= '9') {
        num2 = num2 * 10 + (input[i] - '0');
        i++;
    }
    num2 *= sign2;
    
    switch (op) {
        case '+': return num1 + num2;
        case '-': return num1 - num2;
        case '*': return num1 * num2;
        case '/': return (num2 != 0) ? (num1 / num2) : 0;
        case '%': return (num2 != 0) ? (num1 % num2) : 0;
        default: return 0;
    }
}
// EFN 
void _start(void) {
    print("Simple Calculator\n");
    print("=================\n");
    print("Enter expressions like: 10 + 5\n");
    print("Supported: + - * / %\n");
    print("Type 'exit' to quit\n\n");
    
    char buffer[256];
    
    while (1) {
        print("> ");
        
        int bytes_read = z_read(0, buffer, sizeof(buffer) - 1);
        
        if (bytes_read <= 0) {
            break;
        }
        
        buffer[bytes_read] = '\0';
        
        if (buffer[0] == 'e' && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't') {
            print("Goodbye!\n");
            break;
        }
        
        if (bytes_read <= 1) {
            continue;
        }
        
        int result = calculate(buffer);
        
        print("= ");
        print_num(result);
        print("\n");
    }
    
    z_exit(0);
}

// AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAĞĞĞĞĞĞĞĞĞĞĞĞĞĞHHHHHHHHHHHHHHH PUT ME OUR OF MY MISERYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY