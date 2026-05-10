#ifndef SPLEXER_H
#define SPLEXER_H

#include <stdio.h>
#include <stdbool.h>

#include "sptl.h"

enum Keywords {
    TOK_KW_TypeInt,
    TOK_KW_TypeFloat,
    TOK_KW_Const,
    TOK_KW_Static,
    TOK_KW_Return,
    TOK_KW_Extern,
    TOK_KW_If,
    TOK_KW_Else,
    TOK_KW_For,
    TOK_KW_While,
    TOK_KW_Unknown,
};

enum Operators {
    TOK_OP_Left_Paren,
    TOK_OP_Right_Paren,
    TOK_OP_Left_Braces,
    TOK_OP_Right_Braces,
    TOK_OP_Newline,
    TOK_OP_Semicolon,
    TOK_OP_Comma,
    TOK_OP_Period,
    TOK_OP_Arrow,
    TOK_OP_Assign,
    TOK_OP_Equality,
    TOK_OP_Inequality,
    TOK_OP_Less,
    TOK_OP_Greater,
    TOK_OP_LessEq,
    TOK_OP_GreaterEq,
    TOK_OP_Plus,
    TOK_OP_Minus,
    TOK_OP_Asterisk,
    TOK_OP_Slash,
    TOK_OP_PlusEq,
    TOK_OP_MinusEq,
    TOK_OP_AsteriskEq,
    TOK_OP_SlashEq,
    TOK_OP_Increment,
    TOK_OP_Decrement,
    TOK_OP_ShiftLeft,
    TOK_OP_ShiftRight,
    TOK_OP_ShiftLeftEq,
    TOK_OP_ShiftRightEq,
    TOK_OP_AndAnd,
    TOK_OP_OrOr,
    TOK_OP_Unknown,
};

static const char* keywords[] = {
    [TOK_KW_TypeInt] = "int",
    [TOK_KW_TypeFloat] = "float",
    [TOK_KW_Const] = "const",
    [TOK_KW_Static] = "static",
    [TOK_KW_Return] = "return",
    [TOK_KW_Extern] = "extern",
    [TOK_KW_If] = "if",
    [TOK_KW_Else] = "else",
    [TOK_KW_For] = "for",
    [TOK_KW_While] = "while",
};
static const char* operators[] = {
    [TOK_OP_Left_Paren] = "(",
    [TOK_OP_Right_Paren] = ")",
    [TOK_OP_Left_Braces] = "{",
    [TOK_OP_Right_Braces] = "}",
    [TOK_OP_Newline] = "\n",
    [TOK_OP_Semicolon] = ";",
    [TOK_OP_Comma] = ",",
    [TOK_OP_Period] = ".",
    [TOK_OP_Arrow] = "->",
    [TOK_OP_Assign] = "=",
    [TOK_OP_Equality] = "==",
    [TOK_OP_Inequality] = "!=",
    [TOK_OP_Less] = "<",
    [TOK_OP_Greater] = ">",
    [TOK_OP_LessEq] = "<=",
    [TOK_OP_GreaterEq] = ">=",
    [TOK_OP_Plus] = "+",
    [TOK_OP_Minus] = "-",
    [TOK_OP_Asterisk] = "*",
    [TOK_OP_Slash] = "/",
    [TOK_OP_PlusEq] = "+=",
    [TOK_OP_MinusEq] = "-=",
    [TOK_OP_AsteriskEq] = "*=",
    [TOK_OP_SlashEq] = "/=",
    [TOK_OP_Increment] = "++",
    [TOK_OP_Decrement] = "--",
    [TOK_OP_ShiftLeft] = "<<",
    [TOK_OP_ShiftRight] = ">>",
    [TOK_OP_ShiftLeftEq] = "<<=",
    [TOK_OP_ShiftRightEq] = ">>=",
    [TOK_OP_AndAnd] = "&&",
    [TOK_OP_OrOr] = "||",
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

    Sp_Hash_Table(int) kw_table;
    Sp_Hash_Table(int) op_table;

    Sp_Lexer_Token tok;

    Sp_Lexer_State state;
    Sp_Dynamic_Array(Sp_Lexer_Token) tokens;
} Sp_Lexer;

/*
 * Returns 1 (true) if `c` is a valid character in an identifier, and 0 (false) if not.
 */
bool splexer_char_is_valid(char c);

void splexer_init(Sp_Lexer* splexer, const char* path, const char** keywords, const char** operators);

int splexer_token_append(Sp_Lexer_Token* token, char c);
void splexer_token_clear(Sp_Lexer_Token* token);

void splexer_tokenize(Sp_Lexer* splexer);
void splexer_destroy(Sp_Lexer* splexer);

#endif
