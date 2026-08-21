/* TinyCC for KuzuOS5 - Main Compiler Header */
#ifndef COMPILER_H
#define COMPILER_H

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

/* Token types */
typedef enum {
    TOK_EOF = -1,
    TOK_UNKNOWN = 0,
    
    /* Keywords */
    TOK_INT = 1,
    TOK_CHAR = 2,
    TOK_FLOAT = 3,
    TOK_DOUBLE = 4,
    TOK_VOID = 5,
    TOK_IF = 6,
    TOK_ELSE = 7,
    TOK_WHILE = 8,
    TOK_FOR = 9,
    TOK_RETURN = 10,
    TOK_BREAK = 11,
    TOK_CONTINUE = 12,
    TOK_STRUCT = 13,
    TOK_UNION = 14,
    TOK_ENUM = 15,
    TOK_TYPEDEF = 16,
    TOK_STATIC = 17,
    TOK_EXTERN = 18,
    TOK_CONST = 19,
    TOK_VOLATILE = 20,
    TOK_SIZEOF = 21,
    
    /* Operators and punctuation */
    TOK_IDENTIFIER = 100,
    TOK_NUMBER = 101,
    TOK_STRING = 102,
    TOK_LPAREN = '(',
    TOK_RPAREN = ')',
    TOK_LBRACE = '{',
    TOK_RBRACE = '}',
    TOK_LBRACKET = '[',
    TOK_RBRACKET = ']',
    TOK_SEMICOLON = ';',
    TOK_COMMA = ',',
    TOK_DOT = '.',
    TOK_ARROW = 103,    /* -> */
    TOK_PLUS = '+',
    TOK_MINUS = '-',
    TOK_STAR = '*',
    TOK_SLASH = '/',
    TOK_PERCENT = '%',
    TOK_ASSIGN = '=',
    TOK_PLUS_ASSIGN = 104,   /* += */
    TOK_MINUS_ASSIGN = 105,  /* -= */
    TOK_STAR_ASSIGN = 106,   /* *= */
    TOK_SLASH_ASSIGN = 107,  /* /= */
    TOK_EQ = 108,       /* == */
    TOK_NE = 109,       /* != */
    TOK_LT = '<',
    TOK_GT = '>',
    TOK_LE = 110,       /* <= */
    TOK_GE = 111,       /* >= */
    TOK_AND = '&',
    TOK_OR = '|',
    TOK_XOR = '^',
    TOK_NOT = '!',
    TOK_TILDE = '~',
    TOK_QUESTION = '?',
    TOK_COLON = ':',
    TOK_LOGICAL_AND = 112,  /* && */
    TOK_LOGICAL_OR = 113,   /* || */
    TOK_LSHIFT = 114,   /* << */
    TOK_RSHIFT = 115,   /* >> */
    TOK_INCREMENT = 116,    /* ++ */
    TOK_DECREMENT = 117,    /* -- */
    TOK_ELLIPSIS = 118,     /* ... */
} TokenType;

typedef struct {
    TokenType type;
    int32_t intval;
    char* strval;
    int line;
    int column;
} Token;

typedef struct Lexer {
    char* input;
    int pos;
    int line;
    int column;
    char current_char;
} Lexer;

typedef struct {
    int type;
    int size;
    int align;
} Type;

typedef struct Symbol {
    char* name;
    int type;
    int storage_class;
    int level;
    int address;
    struct Symbol* next;
} Symbol;

typedef struct CompilerState {
    Lexer lexer;
    Token current_token;
    Token lookahead;
    Symbol* symbols;
    int code_pos;
    int data_pos;
    uint8_t* code;
    uint8_t* data;
    int code_size;
    int data_size;
    int stack_offset;
    int level;
} CompilerState;

/* Lexer functions */
Lexer* lexer_new(const char* input);
void lexer_free(Lexer* lexer);
Token lexer_next_token(Lexer* lexer);
void lexer_skip_whitespace(Lexer* lexer);

/* Parser/Compiler functions */
CompilerState* compiler_new(void);
void compiler_free(CompilerState* state);
int compiler_compile(CompilerState* state, const char* source);
int compiler_write_binary(CompilerState* state, const char* filename);

/* Code generation */
void emit_byte(CompilerState* state, uint8_t byte);
void emit_dword(CompilerState* state, uint32_t dword);
void emit_mov_eax_immediate(CompilerState* state, uint32_t value);
void emit_push_eax(CompilerState* state);
void emit_pop_eax(CompilerState* state);
void emit_call(CompilerState* state, uint32_t address);
void emit_ret(CompilerState* state);

#endif /* COMPILER_H */
