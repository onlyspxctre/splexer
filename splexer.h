#ifndef SPLEXER_H
#define SPLEXER_H

#include <stdbool.h>
#include <stdio.h>

#include <sptl.h>

typedef enum {
    TOK_ID,
    TOK_IntLiteral,
    TOK_FloatLiteral,
    TOK_DQStringLiteral,
    TOK_SQStringLiteral,
    TOK_LeftParen,
    TOK_RightParen,
    TOK_LeftBraces,
    TOK_RightBraces,
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
} Sp_Lexer_Tokens;

static const char *SPLEXER_TOKEN_REGISTRY[] = {
    [TOK_LeftParen] = "(",
    [TOK_RightParen] = ")",
    [TOK_LeftBraces] = "{",
    [TOK_RightBraces] = "}",
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

static const char* SPLEXER_TOKENS_LITERAL[] = {
    [TOK_ID]                = "TOK_ID",
    [TOK_IntLiteral]        = "TOK_IntLiteral",
    [TOK_FloatLiteral]      = "TOK_FloatLiteral",
    [TOK_DQStringLiteral]   = "TOK_DQStringLiteral",
    [TOK_SQStringLiteral]   = "TOK_SQStringLiteral",
    [TOK_LeftParen]         = "TOK_LeftParen",
    [TOK_RightParen]        = "TOK_RightParen",
    [TOK_LeftBraces]        = "TOK_LeftBraces",
    [TOK_RightBraces]       = "TOK_RightBraces",
    [TOK_Semicolon]         = "TOK_Semicolon",
    [TOK_Comma]             = "TOK_Comma",
    [TOK_Period]            = "TOK_Period",
    [TOK_Arrow]             = "TOK_Arrow",
    [TOK_Assign]            = "TOK_Assign",
    [TOK_Equality]          = "TOK_Equality",
    [TOK_Inequality]        = "TOK_Inequality",
    [TOK_Less]              = "TOK_Less",
    [TOK_Greater]           = "TOK_Greater",
    [TOK_LessEq]            = "TOK_LessEq",
    [TOK_GreaterEq]         = "TOK_GreaterEq",
    [TOK_Plus]              = "TOK_Plus",
    [TOK_Minus]             = "TOK_Minus",
    [TOK_Asterisk]          = "TOK_Asterisk",
    [TOK_Slash]             = "TOK_Slash",
    [TOK_PlusEq]            = "TOK_PlusEq",
    [TOK_MinusEq]           = "TOK_MinusEq",
    [TOK_AsteriskEq]        = "TOK_AsteriskEq",
    [TOK_SlashEq]           = "TOK_SlashEq",
    [TOK_Increment]         = "TOK_Increment",
    [TOK_Decrement]         = "TOK_Decrement",
    [TOK_ShiftLeft]         = "TOK_ShiftLeft",
    [TOK_ShiftRight]        = "TOK_ShiftRight",
    [TOK_ShiftLeftEq]       = "TOK_ShiftLeftEq",
    [TOK_ShiftRightEq]      = "TOK_ShiftRightEq",
    [TOK_AndAnd]            = "TOK_AndAnd",
    [TOK_OrOr]              = "TOK_OrOr",
    [TOK_Pound]             = "TOK_Pound",
    [TOK_Unknown]           = "TOK_Unknown",
};

typedef struct {
    Sp_Lexer_Tokens type;
    Sp_String_View sv;
    union {
        struct {
            long value;
            Sp_String_View suffixes;
        } int_lit;
        struct {
            double value;
            Sp_String_View suffixes;
        } float_lit;
    };
} Sp_Lexer_Token;

typedef struct {
    size_t line;
    size_t col;
} Sp_Lexer_Token_Line;

typedef enum {
    SPLEXER_IDLE,
    SPLEXER_TOKENIZE,
    SPLEXER_COMMENT,
    SPLEXER_MULTICOMMENT,
    SPLEXER_TERMINATE,
} Sp_Lexer_State;

typedef struct {
    Sp_String_Builder file;
    size_t file_idx;
    Sp_Dynamic_Array(size_t) eols;

    Sp_Hash_Table(Sp_String_View, Sp_Lexer_Tokens) tok_table;

    Sp_Lexer_Token tok;

    Sp_Lexer_State state;
    Sp_Dynamic_Array(Sp_Lexer_Token) tokens;
} Sp_Lexer;

/*
 * Returns 1 (true) if `c` is a valid character in an identifier, and 0 (false) if not.
 */
SPExtern bool splexer_char_is_valid_id(char c);

/*
 * Initializes an instance of Sp_Lexer. Returns 1 if splexer is NULL or path is invalid, and 0 otherwise.
 */
SPExtern int splexer_init(Sp_Lexer *splexer, const char *path);

/* Evaluates whether a given character `c` could be appended to the current working token.
 *
 * Returns 1 if the given character was appended, or 2 if the given character was consumed.
 * If the given character cannot be inserted nor was consumed, this function returns 0.
 * */
SPExtern int splexer_token_append(Sp_Lexer *splexer, const char *c);

SPExtern Sp_Lexer_Token_Line splexer_token_get_line(Sp_Lexer *splexer, const Sp_Lexer_Token *token);

SPExtern void splexer_token_clear(Sp_Lexer *splexer);

SPExtern void splexer_tokenize(Sp_Lexer *splexer);
SPExtern void splexer_destroy(Sp_Lexer *splexer);

#endif
