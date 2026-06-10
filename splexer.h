#ifndef SPLEXER_H
#define SPLEXER_H

#include <stdio.h>
#include <stdbool.h>

#include "sptl.h"

enum Sp_Tokens {
    TOK_ID,
    TOK_LeftParen,
    TOK_RightParen,
    TOK_LeftBraces,
    TOK_RightBraces,
    TOK_Newline,
    TOK_Semicolon,
    TOK_Comma,
    TOK_Period,
    TOK_Arrow,
    TOK_Assign,
    TOK_Equality,
    TOK_Inequality,
    TOK_Less,
    TOK_Greater,
    TOK_LessEq,
    TOK_GreaterEq,
    TOK_Plus,
    TOK_Minus,
    TOK_Asterisk,
    TOK_Slash,
    TOK_PlusEq,
    TOK_MinusEq,
    TOK_AsteriskEq,
    TOK_SlashEq,
    TOK_Increment,
    TOK_Decrement,
    TOK_ShiftLeft,
    TOK_ShiftRight,
    TOK_ShiftLeftEq,
    TOK_ShiftRightEq,
    TOK_AndAnd,
    TOK_OrOr,
    TOK_Pound,
    TOK_Unknown,
};

static const char* SP_TOKEN_REGISTRY[] = {
    [TOK_LeftParen] = "(",
    [TOK_RightParen] = ")",
    [TOK_LeftBraces] = "{",
    [TOK_RightBraces] = "}",
    [TOK_Newline] = "\n",
    [TOK_Semicolon] = ";",
    [TOK_Comma] = ",",
    [TOK_Period] = ".",
    [TOK_Arrow] = "->",
    [TOK_Assign] = "=",
    [TOK_Equality] = "==",
    [TOK_Inequality] = "!=",
    [TOK_Less] = "<",
    [TOK_Greater] = ">",
    [TOK_LessEq] = "<=",
    [TOK_GreaterEq] = ">=",
    [TOK_Plus] = "+",
    [TOK_Minus] = "-",
    [TOK_Asterisk] = "*",
    [TOK_Slash] = "/",
    [TOK_PlusEq] = "+=",
    [TOK_MinusEq] = "-=",
    [TOK_AsteriskEq] = "*=",
    [TOK_SlashEq] = "/=",
    [TOK_Increment] = "++",
    [TOK_Decrement] = "--",
    [TOK_ShiftLeft] = "<<",
    [TOK_ShiftRight] = ">>",
    [TOK_ShiftLeftEq] = "<<=",
    [TOK_ShiftRightEq] = ">>=",
    [TOK_AndAnd] = "&&",
    [TOK_OrOr] = "||",
    [TOK_Pound] = "#",
};

static const char* SP_TOKENS_LITERAL[] = {
    [TOK_ID] = "TOK_ID",
    [TOK_LeftParen]    = "TOK_LeftParen",
    [TOK_RightParen]   = "TOK_RightParen",
    [TOK_LeftBraces]   = "TOK_LeftBraces",
    [TOK_RightBraces]  = "TOK_RightBraces",
    [TOK_Newline]      = "TOK_Newline",
    [TOK_Semicolon]    = "TOK_Semicolon",
    [TOK_Comma]        = "TOK_Comma",
    [TOK_Period]       = "TOK_Period",
    [TOK_Arrow]        = "TOK_Arrow",
    [TOK_Assign]       = "TOK_Assign",
    [TOK_Equality]     = "TOK_Equality",
    [TOK_Inequality]   = "TOK_Inequality",
    [TOK_Less]         = "TOK_Less",
    [TOK_Greater]      = "TOK_Greater",
    [TOK_LessEq]       = "TOK_LessEq",
    [TOK_GreaterEq]    = "TOK_GreaterEq",
    [TOK_Plus]         = "TOK_Plus",
    [TOK_Minus]        = "TOK_Minus",
    [TOK_Asterisk]     = "TOK_Asterisk",
    [TOK_Slash]        = "TOK_Slash",
    [TOK_PlusEq]       = "TOK_PlusEq",
    [TOK_MinusEq]      = "TOK_MinusEq",
    [TOK_AsteriskEq]   = "TOK_AsteriskEq",
    [TOK_SlashEq]      = "TOK_SlashEq",
    [TOK_Increment]    = "TOK_Increment",
    [TOK_Decrement]    = "TOK_Decrement",
    [TOK_ShiftLeft]    = "TOK_ShiftLeft",
    [TOK_ShiftRight]   = "TOK_ShiftRight",
    [TOK_ShiftLeftEq]  = "TOK_ShiftLeftEq",
    [TOK_ShiftRightEq] = "TOK_ShiftRightEq",
    [TOK_AndAnd]       = "TOK_AndAnd",
    [TOK_OrOr]         = "TOK_OrOr",
    [TOK_Pound]        = "TOK_Pound",
};

typedef enum {
    TOK_TYPE_UNKNOWN,
    TOK_TYPE_KEYWORD,
    TOK_TYPE_OPERATOR,
    TOK_TYPE_IDENTIFIER,
    TOK_TYPE_INTLITERAL,
    TOK_TYPE_FLOATLITERAL,
} Sp_Lexer_Token_Type;

typedef struct {
    Sp_Lexer_Token_Type type;
    Sp_String_Builder sb;
} Sp_Lexer_Token;

typedef enum {
    SPLEXER_IDLE,
    SPLEXER_TOKENIZE,
    SPLEXER_TERMINATE,
} Sp_Lexer_State;

typedef struct {
    FILE* f;

    Sp_Hash_Table(const char*, int) tok_table;

    Sp_Lexer_Token tok;

    Sp_Lexer_State state;
    Sp_Dynamic_Array(Sp_Lexer_Token) tokens;
} Sp_Lexer;

/*
 * Returns 1 (true) if `c` is a valid character in an identifier, and 0 (false) if not.
 */
bool splexer_char_is_valid(char c);

void splexer_init(Sp_Lexer* splexer, const char* path, const char** tokens);

int splexer_token_append(Sp_Lexer_Token* token, char c);
void splexer_token_clear(Sp_Lexer_Token* token);

void splexer_tokenize(Sp_Lexer* splexer);
void splexer_destroy(Sp_Lexer* splexer);

#endif
