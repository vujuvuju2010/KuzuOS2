/* TinyCC for KuzuOS5 - Lexer Implementation */
#include "compiler.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

static const char* keywords[] = {
    "int", "char", "float", "double", "void",
    "if", "else", "while", "for", "return",
    "break", "continue", "struct", "union", "enum",
    "typedef", "static", "extern", "const", "volatile", "sizeof",
    NULL
};

static TokenType keyword_token_types[] = {
    TOK_INT, TOK_CHAR, TOK_FLOAT, TOK_DOUBLE, TOK_VOID,
    TOK_IF, TOK_ELSE, TOK_WHILE, TOK_FOR, TOK_RETURN,
    TOK_BREAK, TOK_CONTINUE, TOK_STRUCT, TOK_UNION, TOK_ENUM,
    TOK_TYPEDEF, TOK_STATIC, TOK_EXTERN, TOK_CONST, TOK_VOLATILE, TOK_SIZEOF,
};

Lexer* lexer_new(const char* input) {
    Lexer* lexer = (Lexer*)malloc(sizeof(Lexer));
    if (!lexer) return NULL;
    
    lexer->input = (char*)input;
    lexer->pos = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->current_char = lexer->input[0];
    
    return lexer;
}

void lexer_free(Lexer* lexer) {
    if (lexer) free(lexer);
}

static void advance(Lexer* lexer) {
    if (lexer->current_char == '\n') {
        lexer->line++;
        lexer->column = 1;
    } else {
        lexer->column++;
    }
    lexer->pos++;
    lexer->current_char = lexer->input[lexer->pos];
}

static char peek(Lexer* lexer, int offset) {
    return lexer->input[lexer->pos + offset];
}

void lexer_skip_whitespace(Lexer* lexer) {
    while (lexer->current_char == ' ' || lexer->current_char == '\t' ||
           lexer->current_char == '\n' || lexer->current_char == '\r') {
        advance(lexer);
    }
    
    /* Skip comments */
    while (lexer->current_char == '/' && peek(lexer, 1) == '/') {
        while (lexer->current_char && lexer->current_char != '\n') {
            advance(lexer);
        }
        if (lexer->current_char == '\n') advance(lexer);
        while (lexer->current_char == ' ' || lexer->current_char == '\t' ||
               lexer->current_char == '\n' || lexer->current_char == '\r') {
            advance(lexer);
        }
    }
    
    /* Skip block comments */
    while (lexer->current_char == '/' && peek(lexer, 1) == '*') {
        advance(lexer); advance(lexer);
        while (lexer->current_char && !(lexer->current_char == '*' && peek(lexer, 1) == '/')) {
            advance(lexer);
        }
        if (lexer->current_char == '*') advance(lexer);
        if (lexer->current_char == '/') advance(lexer);
        while (lexer->current_char == ' ' || lexer->current_char == '\t' ||
               lexer->current_char == '\n' || lexer->current_char == '\r') {
            advance(lexer);
        }
    }
}

static Token read_identifier(Lexer* lexer) {
    Token token;
    token.type = TOK_IDENTIFIER;
    token.line = lexer->line;
    token.column = lexer->column;
    
    int start = lexer->pos;
    while (isalnum(lexer->current_char) || lexer->current_char == '_') {
        advance(lexer);
    }
    
    int len = lexer->pos - start;
    char* ident = (char*)malloc(len + 1);
    strncpy(ident, &lexer->input[start], len);
    ident[len] = '\0';
    
    /* Check if it's a keyword */
    for (int i = 0; keywords[i]; i++) {
        if (strcmp(ident, keywords[i]) == 0) {
            token.type = keyword_token_types[i];
            free(ident);
            return token;
        }
    }
    
    token.strval = ident;
    return token;
}

static Token read_number(Lexer* lexer) {
    Token token;
    token.type = TOK_NUMBER;
    token.line = lexer->line;
    token.column = lexer->column;
    
    int start = lexer->pos;
    while (isdigit(lexer->current_char)) {
        advance(lexer);
    }
    
    int len = lexer->pos - start;
    char num_str[32];
    strncpy(num_str, &lexer->input[start], len);
    num_str[len] = '\0';
    
    token.intval = 0;
    for (int i = 0; num_str[i]; i++) {
        token.intval = token.intval * 10 + (num_str[i] - '0');
    }
    
    return token;
}

static Token read_string(Lexer* lexer) {
    Token token;
    token.type = TOK_STRING;
    token.line = lexer->line;
    token.column = lexer->column;
    
    advance(lexer); /* Skip opening quote */
    
    int start = lexer->pos;
    while (lexer->current_char && lexer->current_char != '"') {
        if (lexer->current_char == '\\') {
            advance(lexer);
        }
        advance(lexer);
    }
    
    int len = lexer->pos - start;
    char* str = (char*)malloc(len + 1);
    strncpy(str, &lexer->input[start], len);
    str[len] = '\0';
    
    if (lexer->current_char == '"') advance(lexer);
    
    token.strval = str;
    return token;
}

Token lexer_next_token(Lexer* lexer) {
    Token token;
    
    lexer_skip_whitespace(lexer);
    
    token.line = lexer->line;
    token.column = lexer->column;
    
    if (!lexer->current_char) {
        token.type = TOK_EOF;
        return token;
    }
    
    /* Identifiers and keywords */
    if (isalpha(lexer->current_char) || lexer->current_char == '_') {
        return read_identifier(lexer);
    }
    
    /* Numbers */
    if (isdigit(lexer->current_char)) {
        return read_number(lexer);
    }
    
    /* Strings */
    if (lexer->current_char == '"') {
        return read_string(lexer);
    }
    
    /* Two-character operators */
    if (lexer->current_char == '-' && peek(lexer, 1) == '>') {
        token.type = TOK_ARROW;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '+' && peek(lexer, 1) == '=') {
        token.type = TOK_PLUS_ASSIGN;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '-' && peek(lexer, 1) == '=') {
        token.type = TOK_MINUS_ASSIGN;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '*' && peek(lexer, 1) == '=') {
        token.type = TOK_STAR_ASSIGN;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '/' && peek(lexer, 1) == '=') {
        token.type = TOK_SLASH_ASSIGN;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '=' && peek(lexer, 1) == '=') {
        token.type = TOK_EQ;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '!' && peek(lexer, 1) == '=') {
        token.type = TOK_NE;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '<' && peek(lexer, 1) == '=') {
        token.type = TOK_LE;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '>' && peek(lexer, 1) == '=') {
        token.type = TOK_GE;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '&' && peek(lexer, 1) == '&') {
        token.type = TOK_LOGICAL_AND;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '|' && peek(lexer, 1) == '|') {
        token.type = TOK_LOGICAL_OR;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '<' && peek(lexer, 1) == '<') {
        token.type = TOK_LSHIFT;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '>' && peek(lexer, 1) == '>') {
        token.type = TOK_RSHIFT;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '+' && peek(lexer, 1) == '+') {
        token.type = TOK_INCREMENT;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '-' && peek(lexer, 1) == '-') {
        token.type = TOK_DECREMENT;
        advance(lexer); advance(lexer);
        return token;
    }
    
    if (lexer->current_char == '.' && peek(lexer, 1) == '.' && peek(lexer, 2) == '.') {
        token.type = TOK_ELLIPSIS;
        advance(lexer); advance(lexer); advance(lexer);
        return token;
    }
    
    /* Single-character tokens */
    token.type = lexer->current_char;
    advance(lexer);
    return token;
}
