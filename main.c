#include "splexer.h"

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
    for (size_t i = 0; i < splexer.tokens.count; ++i) {
        printf("%s", splexer.tokens.data[i].sb.data);
        if (splexer.tokens.data[i].type == TOK_TYPE_INTLITERAL) {
            printf("(int literal) ");
        }
        else if (splexer.tokens.data[i].type == TOK_TYPE_FLOATLITERAL) {
            printf("(float literal) ");
        }
        else if (splexer.tokens.data[i].type == TOK_TYPE_IDENTIFIER) {
            printf("(identifier) ");
        }
    }
    // printf("\n------------------------------------------------\n");
    // printf("parsed.count: %ld\n", parsed.count);

    splexer_destroy(&splexer);
}
