#include <ctype.h>
#include <stdio.h>

#include "sptl.h"

enum Token {
    TOK_Unknown = 0,
    TOK_TypeInt,
    TOK_Return,
    TOK_Extern,
    TOK_Left_Paren,
    TOK_Right_Paren,
    TOK_Left_Braces,
    TOK_Right_Braces,
    TOK_Newline,
    TOK_Semicolon,
    TOK_Comma,
    TOK_Period,
    TOK_Equal,
    TOK_Asterisk,
    __TOKEN_OPTIONS__,
};

char* token_matches[__TOKEN_OPTIONS__] = {
    [TOK_TypeInt]      = "int",
    [TOK_Return]       = "return",
    [TOK_Extern]       = "extern",
    [TOK_Left_Paren]   = "(",
    [TOK_Right_Paren]  = ")",
    [TOK_Left_Braces]  = "{",
    [TOK_Right_Braces] = "}",
    [TOK_Newline]      = "\n",
    [TOK_Semicolon]    = ";",
    [TOK_Comma]        = ",",
    [TOK_Period]       = ".",
    [TOK_Equal]        = "=",
    [TOK_Asterisk]     = "*",
};

Sp_Hash_Table(int) Int_HT;
typedef struct {
    Int_HT token_table;
} Sp_Lexer;

void splexer_init(Sp_Lexer* splexer, char** matches) {
    for (int i = 0; i < __TOKEN_OPTIONS__; ++i) {
        if (!matches[i]) {
            printf("continuing\n");
            continue;
        }
        sp_ht_insert(&splexer->token_table, matches[i], i);
    }
    sp_ht_print(&splexer->token_table);
}

void splexer_destroy(Sp_Lexer* splexer) {
    sp_ht_free(&splexer->token_table);
    // END
}

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    Sp_Lexer splexer = {0};

    splexer_init(&splexer, token_matches);

    FILE* f = fopen("hello.c", "rb");

    Sp_String_Builder parsed = {0};
    Sp_String_Builder tok    = {0};
    Sp_String_Builder ast    = {0};

    char c[2] = "0";
    while (fread(c, 1, 1, f) != 0) {
        if (isalnum(*c) || *c == '_') {
            sp_sb_appendf(&tok, "%s", c);
        } else {
            if (tok.count > 0) {
                sp_ht_node_ptr(&splexer.token_table) tok_query = sp_ht_get(&splexer.token_table, tok.data);

                if (!tok_query) {
                    sp_sb_appendf(&ast, "Identifier ");
                } else {
                    sp_sb_appendf(&ast, "%s ", token_matches[tok_query->value]);
                }

                sp_sb_appendf(&parsed, "%s", tok.data);
                sp_da_clear(&tok);
            }

            if (*c != ' ') {
                sp_ht_node_ptr(&splexer.token_table) tok_query = sp_ht_get(&splexer.token_table, c);

                if (!tok_query) {
                    printf("Token \"%s\" not implemented! Terminating...\n", c);
                    goto done;
                }
                sp_sb_appendf(&ast, "%s ", token_matches[tok_query->value]);
            }

            sp_sb_appendf(&parsed, "%s", c);
        }
    }

done:
    printf("------------------------------------------------\n");
    printf("%s<EOF>\n", parsed.data);
    printf("------------------------------------------------\n");
    printf("IR (Ast):\n");
    printf("%s", ast.data);

    printf("parsed.count: %ld\n", parsed.count);

    sp_da_free(&tok);
    sp_da_free(&parsed);
    sp_da_free(&ast);

    splexer_destroy(&splexer);
    fclose(f);
}
