#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "sptl.h"

enum Token {
    Identifier,
    Int_Literal,
    Left_Paren = '(',
    Right_Paren = ')',
    Left_Braces = '{',
    Right_Braces = '}',
    Newline = '\n',
    Semicolon = ';',
};

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    FILE* f = fopen("hello.c", "rb");

    Sp_String_Builder parsed = {0};
    Sp_String_Builder tok = {0};
    Sp_String_Builder ast = {0};

    char c;
    while (fread(&c, 1, 1, f) != 0) {
        if (isalnum(c)) {
            sp_sb_appendf(&tok, "%c", c);
        } else {
            if (tok.count > 0) {
                if (strcmp(tok.data, "int") == 0) {
                    sp_sb_appendf(&ast, "Int_Literal ");
                } else {
                    sp_sb_appendf(&ast, "Identifier ");
                }

                sp_sb_appendf(&parsed, "%s", tok.data);
                sp_da_clear(&tok);
            }

            switch (c) {
                case Left_Paren:
                    sp_sb_appendf(&ast, "( ");
                    break;
                case Right_Paren:
                    sp_sb_appendf(&ast, ") ");
                    break;
                case Left_Braces:
                    sp_sb_appendf(&ast, "{ ");
                    break;
                case Right_Braces:
                    sp_sb_appendf(&ast, "} ");
                    break;
                case Semicolon:
                case Newline:
                    sp_sb_appendf(&ast, "\n");
                case ' ':
                    break;
                default:
                    printf("Token not implemented, terminating tokenization...\n");
                    goto done;
            }

            sp_sb_appendf(&parsed, "%c", c);
        }
    }

done:
    printf("------------------------------------------------\n");
    printf("%s<EOF>\n", parsed.data);
    printf("------------------------------------------------\n");
    printf("IR (Ast):\n");
    printf("%s", ast.data);

    printf("parsed.count: %ld\n", parsed.count);

    free(tok.data);
    free(parsed.data);
    free(ast.data);
    fclose(f);
}
