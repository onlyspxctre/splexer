#include "splexer.h"

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    Sp_Lexer splexer = {0};

    splexer_init(&splexer, "hello.c");

    // Sp_String_Builder parsed = {0};

    Sp_Lexer_Return_Code code;
    while ((code = splexer_tokenize(&splexer)) == SPLEXER_OK);

    switch (code) {
        case SPLEXER_ERROR:
            if (splexer.state == SPLEXER_STATE_MULTICOMMENT) {
                sp_log(SP_ERROR, "Unescaped multiline comment!");
            } else {
                sp_log(SP_ERROR, "Error occurred while tokenizing!");
            }
            return 1;
        case SPLEXER_EOF:
            break;
        default:
            sp_unreachable();
    }

    // printf("\n------------------------------------------------\n");
    // printf("%s<EOF>\n", parsed.data);
    // printf("\n------------------------------------------------\n");
    printf("IR (Ast):\n");
    for (size_t i = 0; i < splexer.tokens.count; ++i) {
        Sp_Lexer_Token_Line data = splexer_token_get_line(&splexer, &splexer.tokens.data[i]);
        printf("%ld:%ld =>", data.line, data.col);
        printf("\'" SP_SV_FMT "\': %s", sp_sv_arg(splexer.tokens.data[i].sv), SPLEXER_TOKENS_LITERAL[splexer.tokens.data[i].type]);
        if (splexer.tokens.data[i].type == TOK_FloatLiteral) {
            printf(" | value: %.2f | suffixes: \'" SP_SV_FMT "\'", splexer.tokens.data[i].float_lit.value, sp_sv_arg(splexer.tokens.data[i].float_lit.suffixes));
        }
        else if (splexer.tokens.data[i].type == TOK_IntLiteral) {
            printf(" | value: %ld | suffixes: \'" SP_SV_FMT "\'", splexer.tokens.data[i].int_lit.value, sp_sv_arg(splexer.tokens.data[i].int_lit.suffixes));
        }
        putchar('\n');
    }
    // printf("\n------------------------------------------------\n");
    // printf("parsed.count: %ld\n", parsed.count);

    splexer_destroy(&splexer);
}
