#include <ctype.h>
#include <stdio.h>

#include "sptl.h"

enum Keywords {
    TOK_KW_TypeInt,
    TOK_KW_Const,
    TOK_KW_Static,
    TOK_KW_Return,
    TOK_KW_Extern,
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
    TOK_OP_Increment,
    TOK_OP_Decrement,
    TOK_OP_Unknown,
};

char* keywords[] = {
    [TOK_KW_TypeInt] = "int",
    [TOK_KW_Const] = "const",
    [TOK_KW_Static] = "static",
    [TOK_KW_Return] = "return",
    [TOK_KW_Extern] = "extern",
};
char* operators[] = {
    [TOK_OP_Left_Paren] = "(",
    [TOK_OP_Right_Paren] = ")",
    [TOK_OP_Left_Braces] = "{",
    [TOK_OP_Right_Braces] = "}",
    [TOK_OP_Newline] = "\n",
    [TOK_OP_Semicolon] = ";",
    [TOK_OP_Comma] = ",",
    [TOK_OP_Period] = ".",
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
    [TOK_OP_Increment] = "++",
    [TOK_OP_Decrement] = "--",
};

Sp_Hash_Table(int) Int_HT;

typedef enum {
    AST_KEYWORD,
    AST_OPERATOR,
    AST_IDENTIFIER,
} Sp_Lexer_Ast_Type;

typedef struct {
    Sp_Lexer_Ast_Type type;
    Sp_String_Builder str;
} Sp_Lexer_Ast_Node;

Sp_Dynamic_Array(Sp_Lexer_Ast_Node) Sp_Lexer_Ast;

typedef enum {
    SPLEXER_IDLE,
    SPLEXER_ALPHANUM,
    SPLEXER_OPERATOR,
    SPLEXER_TERMINATE,
} Sp_Lexer_State;

typedef struct {
    FILE* f;

    Int_HT kw_table;
    Int_HT op_table;
    Sp_String_Builder tok;

    Sp_Lexer_State state;
    Sp_Lexer_Ast ast;
} Sp_Lexer;

/*
 * Returns 1 (true) if `c` is a valid character in an identifier, and 0 (false) if not.
 */
int splexer_char_is_valid(char c) {
    if (isalnum(c) || c == '_') {
        return 1;
    }
    return 0;
}

void splexer_init(Sp_Lexer* splexer, const char* path, char** keywords, char** operators) {
    for (int i = 0; i < TOK_KW_Unknown; ++i) {
        if (!keywords[i]) {
            continue;
        }
        sp_ht_insert(&splexer->kw_table, keywords[i], i);
    }
    for (int i = 0; i < TOK_OP_Unknown; ++i) {
        if (!operators[i]) {
            continue;
        }
        sp_ht_insert(&splexer->op_table, operators[i], i);
    }

    splexer->f = fopen(path, "rb");
}

/*
 * Evaluate the lexer's state based on the given character.
 */
Sp_Lexer_State splexer_eval_state(char c) {
    if (splexer_char_is_valid(c)) {
        return SPLEXER_ALPHANUM;
    } else {
        return SPLEXER_OPERATOR;
    }
}

void splexer_tokenize(Sp_Lexer* splexer) {
    sp_ht_node_ptr(&splexer->kw_table) kw_query;
    sp_ht_node_ptr(&splexer->op_table) op_query;
    char buffer[2] = "\0";

    while (fread(buffer, 1, 1, splexer->f) != 0) {
        switch (splexer->state) {
            case SPLEXER_IDLE:
                if (*buffer == ' ') {
                    continue;
                }
                splexer->state = splexer_eval_state(*buffer);
                sp_sb_appendf(&splexer->tok, "%s", buffer);
                break;
            case SPLEXER_TERMINATE:
                fprintf(stderr, "The lexer has terminated! Cannot keep tokenizing...\n");
                return;
            default:
                if (splexer->state != splexer_eval_state(*buffer)) {
                    if (*buffer != ' ') {
                        fseek(splexer->f, -1, SEEK_CUR);
                    }
                    goto lex;
                }

                sp_sb_appendf(&splexer->tok, "%s", buffer);
                break;
        }
    }

lex:
    switch (splexer->state) {
        case SPLEXER_ALPHANUM:
            kw_query = sp_ht_get(&splexer->kw_table, splexer->tok.data);
            if (kw_query) {
                Sp_Lexer_Ast_Node node = (Sp_Lexer_Ast_Node) {
                    .type = AST_KEYWORD,
                    .str = {0},
                };
                sp_sb_appendf(&node.str, "\'%s\' ", kw_query->key);

                sp_da_push(&splexer->ast, node);
            } else {
                Sp_Lexer_Ast_Node node = (Sp_Lexer_Ast_Node) {
                    .type = AST_IDENTIFIER,
                    .str = {0},
                };
                sp_sb_appendf(&node.str, "\'%s\' ", splexer->tok.data);

                sp_da_push(&splexer->ast, node);
            }
            goto done;
        case SPLEXER_OPERATOR:
            while (splexer->tok.count) {
                op_query = sp_ht_get(&splexer->op_table, splexer->tok.data);

                if (op_query) {
                    Sp_Lexer_Ast_Node node = (Sp_Lexer_Ast_Node) {
                        .type = AST_OPERATOR,
                        .str = {0},
                    };

                    if (*op_query->key == '\n') {
                        sp_sb_appendf(&node.str, "\n");
                    } else {
                        sp_sb_appendf(&node.str, "\'%s\' ", op_query->key);
                    }

                    sp_da_push(&splexer->ast, node);
                    goto done;
                } else {
                    /* If the current token is NOT found, we pop one character off the end and attempt again */
                    sp_da_pop(&splexer->tok);
                    fseek(splexer->f, -1, SEEK_CUR);
                }
            }

            /* The operator does not exist */

            sp_log(SP_ERROR, "splexer_tokenize could not find any matching operator! Terminating...");
            splexer->state = SPLEXER_TERMINATE;
            return;
        default:
            sp_unreachable();
            break;
    }

done:
    sp_da_clear(&splexer->tok);
    splexer->state = feof(splexer->f) ? SPLEXER_TERMINATE : SPLEXER_IDLE;
}

void splexer_destroy(Sp_Lexer* splexer) {
    fclose(splexer->f);
    splexer->f = NULL;

    sp_ht_free(&splexer->kw_table);
    sp_ht_free(&splexer->op_table);

    sp_da_free(&splexer->tok);
    for (size_t i = 0; i < splexer->ast.count; ++i) {
        sp_da_free(&splexer->ast.data[i].str);
    }
    sp_da_free(&splexer->ast);
}

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    Sp_Lexer splexer = {0};

    splexer_init(&splexer, "hello.c", keywords, operators);

    // Sp_String_Builder parsed = {0};

    while (splexer.state != SPLEXER_TERMINATE) {
        splexer_tokenize(&splexer);
    }

    // printf("\n------------------------------------------------\n");
    // printf("%s<EOF>\n", parsed.data);
    // printf("\n------------------------------------------------\n");
    printf("IR (Ast):\n");
    for (size_t i = 0; i < splexer.ast.count; ++i) {
        printf("%s", splexer.ast.data[i].str.data);
    }
    // printf("\n------------------------------------------------\n");
    // printf("parsed.count: %ld\n", parsed.count);

    splexer_destroy(&splexer);
}
