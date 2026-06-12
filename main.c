#include "splexer.h"

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    Sp_Lexer splexer = {0};

    splexer_init(&splexer, "hello.c");

    // Sp_String_Builder parsed = {0};

    while (splexer.state != SPLEXER_TERMINATE) {
        splexer_tokenize(&splexer);
    }

    // printf("\n------------------------------------------------\n");
    // printf("%s<EOF>\n", parsed.data);
    // printf("\n------------------------------------------------\n");
    printf("IR (Ast):\n");
    for (size_t i = 0; i < splexer.tokens.count; ++i) {
        printf("\'%s\': %s", splexer.tokens.data[i].sb.data, SPLEXER_TOKENS_LITERAL[splexer.tokens.data[i].type]);
        if (splexer.tokens.data[i].type == TOK_FloatLiteral) {
            printf(" | value: %.2f | suffixes: \'%s\'", splexer.tokens.data[i].float_lit.value, splexer.tokens.data[i].float_lit.suffixes);
        }
        else if (splexer.tokens.data[i].type == TOK_IntLiteral) {
            printf(" | value: %ld | suffixes: \'%s\'", splexer.tokens.data[i].int_lit.value, splexer.tokens.data[i].int_lit.suffixes);
        }
        putchar('\n');
    }
    // printf("\n------------------------------------------------\n");
    // printf("parsed.count: %ld\n", parsed.count);

    splexer_destroy(&splexer);
}
