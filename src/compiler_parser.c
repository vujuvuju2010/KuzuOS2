/* TinyCC for KuzuOS5 - Parser/Codegen Implementation */
#include "compiler.h"
#include <string.h>
#include <stdlib.h>

extern int z_write(int fd, const void* buf, size_t count);
extern int z_open(const char* path, int flags);
extern int z_close(int fd);

CompilerState* compiler_new(void) {
    CompilerState* state = (CompilerState*)malloc(sizeof(CompilerState));
    if (!state) return NULL;
    
    state->code = (uint8_t*)malloc(65536);
    state->data = (uint8_t*)malloc(65536);
    
    if (!state->code || !state->data) {
        free(state->code);
        free(state->data);
        free(state);
        return NULL;
    }
    
    state->code_size = 65536;
    state->data_size = 65536;
    state->code_pos = 0;
    state->data_pos = 0;
    state->symbols = NULL;
    state->stack_offset = 0;
    state->level = 0;
    
    return state;
}

void compiler_free(CompilerState* state) {
    if (!state) return;
    
    Symbol* sym = state->symbols;
    while (sym) {
        Symbol* next = sym->next;
        if (sym->name) free(sym->name);
        free(sym);
        sym = next;
    }
    
    free(state->code);
    free(state->data);
    free(state);
}

void emit_byte(CompilerState* state, uint8_t byte) {
    if (state->code_pos >= state->code_size) return;
    state->code[state->code_pos++] = byte;
}

void emit_dword(CompilerState* state, uint32_t dword) {
    if (state->code_pos + 4 > state->code_size) return;
    state->code[state->code_pos++] = (dword >> 0) & 0xFF;
    state->code[state->code_pos++] = (dword >> 8) & 0xFF;
    state->code[state->code_pos++] = (dword >> 16) & 0xFF;
    state->code[state->code_pos++] = (dword >> 24) & 0xFF;
}

void emit_mov_eax_immediate(CompilerState* state, uint32_t value) {
    /* mov eax, imm32 */
    emit_byte(state, 0xB8);
    emit_dword(state, value);
}

void emit_push_eax(CompilerState* state) {
    /* push eax */
    emit_byte(state, 0x50);
}

void emit_pop_eax(CompilerState* state) {
    /* pop eax */
    emit_byte(state, 0x58);
}

void emit_call(CompilerState* state, uint32_t address) {
    /* call address (rel32) */
    emit_byte(state, 0xE8);
    int32_t offset = (int32_t)address - (int32_t)(state->code_pos + 4);
    emit_dword(state, (uint32_t)offset);
}

void emit_ret(CompilerState* state) {
    /* ret */
    emit_byte(state, 0xC3);
}

static Symbol* lookup_symbol(CompilerState* state, const char* name) {
    Symbol* sym = state->symbols;
    while (sym) {
        if (strcmp(sym->name, name) == 0) {
            return sym;
        }
        sym = sym->next;
    }
    return NULL;
}

static Symbol* define_symbol(CompilerState* state, const char* name, int type, int storage_class) {
    Symbol* sym = (Symbol*)malloc(sizeof(Symbol));
    if (!sym) return NULL;
    
    sym->name = (char*)malloc(strlen(name) + 1);
    strcpy(sym->name, name);
    sym->type = type;
    sym->storage_class = storage_class;
    sym->level = state->level;
    sym->address = 0;
    
    sym->next = state->symbols;
    state->symbols = sym;
    
    return sym;
}

static void parse_expression(CompilerState* state);
static void parse_statement(CompilerState* state);

static void parse_primary(CompilerState* state) {
    Token* tok = &state->current_token;
    
    if (tok->type == TOK_NUMBER) {
        emit_mov_eax_immediate(state, tok->intval);
    } else if (tok->type == TOK_IDENTIFIER) {
        /* Variable reference - for now just allocate */
        define_symbol(state, tok->strval, 1, 0);
    }
}

static void parse_expression(CompilerState* state) {
    parse_primary(state);
}

static void parse_statement(CompilerState* state) {
    Token* tok = &state->current_token;
    
    if (tok->type == TOK_LBRACE) {
        /* Block statement */
        state->level++;
        while (tok->type != TOK_RBRACE && tok->type != TOK_EOF) {
            parse_statement(state);
        }
        state->level--;
    } else if (tok->type == TOK_RETURN) {
        /* Return statement */
        parse_expression(state);
        emit_ret(state);
    } else if (tok->type == TOK_IF) {
        /* If statement */
        parse_expression(state);
    } else {
        /* Expression statement */
        parse_expression(state);
    }
}

int compiler_compile(CompilerState* state, const char* source) {
    /* Initialize lexer */
    state->lexer = *(Lexer*)malloc(sizeof(Lexer));
    state->lexer.input = (char*)source;
    state->lexer.pos = 0;
    state->lexer.line = 1;
    state->lexer.column = 1;
    state->lexer.current_char = state->lexer.input[0];
    
    /* Emit ELF header for 32-bit x86 */
    /* ELF magic */
    emit_byte(state, 0x7F);
    emit_byte(state, 'E');
    emit_byte(state, 'L');
    emit_byte(state, 'F');
    
    /* ELF class (32-bit) */
    emit_byte(state, 1);
    
    /* ELF data (little endian) */
    emit_byte(state, 1);
    
    /* ELF version */
    emit_byte(state, 1);
    
    /* ELF OS/ABI (UNIX System V) */
    emit_byte(state, 0);
    
    /* Pad to offset 16 */
    while (state->code_pos < 16) {
        emit_byte(state, 0);
    }
    
    /* e_type (executable) */
    emit_byte(state, 2);
    emit_byte(state, 0);
    
    /* e_machine (Intel 80386) */
    emit_byte(state, 3);
    emit_byte(state, 0);
    
    /* e_version */
    emit_dword(state, 1);
    
    return 0;
}

int compiler_write_binary(CompilerState* state, const char* filename) {
    /* Write binary to file using syscalls */
    int fd = z_open(filename, 0x0241);  /* O_WRONLY | O_CREAT | O_TRUNC */
    if (fd < 0) {
        return -1;
    }
    
    z_write(fd, state->code, state->code_pos);
    z_close(fd);
    
    return 0;
}
