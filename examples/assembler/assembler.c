#include "splexer.h"

// TODO: line number support in splexer
#define TODO_LINE 0

typedef enum { ARG_REG, ARG_IMM } Arg;
typedef enum { REG_NIL, REG_A, REG_B } Register;

typedef struct {
    Sp_Lexer *splexer;
    size_t i;
    bool isReady;
    const char *MSA;
    const char *MSB;
    const char *MSC;
    struct {
        Register lhs;
        Arg rhs_arg;
        union {
            Register rhs;
            uint8_t rhs_imm;
        };
    };
    Sp_String_Builder generated;
} Parser;

static inline const Sp_Lexer_Token *parser_curr_token(Parser *parser) {
    if (parser->i >= parser->splexer->tokens.count) {
        return NULL;
    }
    return &parser->splexer->tokens.data[parser->i];
}

static inline const Sp_Lexer_Token *parser_next_token(Parser *parser) {
    if (parser->i < parser->splexer->tokens.count) {
        ++parser->i;
        return parser_curr_token(parser);
    }
    return NULL;
}

static inline int parser_interpret_lhs(Parser *parser) {
    const Sp_Lexer_Token *token = parser_curr_token(parser);
    if (!token) {
        sp_log(SP_ERROR, "Line %d: Failed to interpret; end of file", TODO_LINE);
        return 1;
    }

    if (token->type != TOK_ID) {
        sp_log(SP_ERROR, "Line %d: Unknown symbol at lhs", TODO_LINE);
        return 1;
    }

    if (strcmp(token->sb.data, "REGA") == 0) {
        parser->lhs = REG_A;
        return 0;
    } else if (strcmp(token->sb.data, "REGB") == 0) {
        parser->lhs = REG_B;
        return 0;
    } else {
        sp_log(SP_ERROR, "Line %d: Unknown register \"%s\" at lhs", TODO_LINE, token->sb.data);
        return 1;
    }
}

static inline int parser_interpret_rhs(Parser *parser) {
    const Sp_Lexer_Token *token = parser_curr_token(parser);

    if (!token) {
        sp_log(SP_ERROR, "Line %d: Failed to interpret; end of file", TODO_LINE);
        return 1;
    }

    switch (token->type) {
        case TOK_ID:
            parser->rhs_arg = ARG_REG;

            if (strcmp(token->sb.data, "REGA") == 0) {
                parser->rhs = REG_A;
                return 0;
            } else if (strcmp(token->sb.data, "REGB") == 0) {
                parser->rhs = REG_B;
                return 0;
            } else {
                sp_log(SP_ERROR, "Line %d: Unknown register \"%s\" at rhs", TODO_LINE, token->sb.data);
                return 1;
            }
            break;
        case TOK_IntLiteral:
            parser->rhs_arg = ARG_IMM;
            parser->rhs_imm = (uint8_t) token->int_lit.value;
            return 0;
        default:
            sp_log(SP_ERROR, "Line %d: Unknown symbol at rhs", TODO_LINE);
            return 1;
    }
}

static inline int parser_interpret(Parser *parser) {
    if (parser->lhs == REG_NIL) {
        return parser_interpret_lhs(parser);
    } else {
        return parser_interpret_rhs(parser);
    }
}

static inline void parser_generate(Parser *parser) {
    sp_sb_appendf(&parser->generated, "%s%s%s ", parser->MSA, parser->MSB, parser->MSC);
}

static inline void parser_reset(Parser *parser) {
    parser->MSA = NULL;
    parser->MSB = NULL;
    parser->MSC = NULL;
    parser->lhs = REG_NIL;
    parser->rhs = REG_NIL;
}

static inline int load(Parser *parser) {
    parser_next_token(parser);
    if (parser_interpret(parser) != 0) {
        return 1;
    }

    parser_next_token(parser);
    if (parser_interpret(parser) != 0) {
        return 1;
    }

    const char **load_reg;
    switch (parser->lhs) {
        case REG_A:
            // keep REG_B constant
            load_reg = &parser->MSA;
            parser->MSB = "10";
            parser->MSC = "000";
            break;
        case REG_B:
            // keep REG_A constant
            parser->MSA = "01";
            load_reg = &parser->MSB;
            parser->MSC = "001";
            break;
        case REG_NIL:
        default:
            sp_unreachable();
    }
    switch (parser->rhs_arg) {
        case ARG_REG:
            switch (parser->rhs) {
                case REG_A:
                    *load_reg = "01";
                    break;
                case REG_B:
                    *load_reg = "10";
                    break;
                default:
                    sp_unreachable();
            }
            break;
        case ARG_IMM:
            *load_reg = "00";
            break;
    }
    parser_generate(parser);
    parser_reset(parser);

    return 0;
}

static inline int add(Parser *parser) { 
    parser_next_token(parser);
    if (parser_interpret(parser) != 0) {
        return 1;
    }

    parser->MSC = "011";
    switch (parser->lhs) {
        case REG_A:
            parser->MSA = "11";
            parser->MSB = "10";
            break;
        case REG_B:
            parser->MSA = "01";
            parser->MSB = "11";
            break;
        case REG_NIL:
        default:
            sp_unreachable();
    }

    parser_generate(parser);
    parser_reset(parser);

    return 0;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        sp_log(SP_ERROR, "Please provide path to an assembly file!");
        return 1;
    }
    if (argc < 2) {
        sp_log(SP_ERROR, "Too many arguments");
        return 1;
    }

    Parser parser = {0};
    parser.splexer = &(Sp_Lexer) {0};

    if (splexer_init(parser.splexer, argv[1]) != 0) {
        sp_log(SP_ERROR, "Unable to open file: \"%s\"", argv[1]);
        return 1;
    }

    while (parser.splexer->state != SPLEXER_TERMINATE) {
        splexer_tokenize(parser.splexer);
    }

    const Sp_Lexer_Token *token = parser_curr_token(&parser);
    parser.isReady = true;
    do {
        if (token->type == TOK_Newline) {
            parser.isReady = true;
            continue;
        }

        if (!parser.isReady) { // some token after a completed instruction
            sp_log(SP_ERROR, "Line %d: Trailing tokens after instruction", TODO_LINE);
            return 1;
        }

        if (token->type != TOK_ID) {
            sp_log(SP_ERROR,
                   "Line %d: Failed to parse unknown symbol \"%s\"",
                   TODO_LINE,
                   SPLEXER_TOKENS_LITERAL[token->type]);
            return 1;
        }

        parser.isReady = false; // set to false to await next line for next instruction

        if (strcmp(token->sb.data, "LOAD") == 0) {
            if (load(&parser) != 0) {
                sp_log(SP_ERROR, "Line %d: Error assembling instruction: LOAD", TODO_LINE);
                return 1;
            }
        } else if (strcmp(token->sb.data, "ADD") == 0) {
            if (add(&parser) != 0) {
                sp_log(SP_ERROR, "Line %d: Error assembling instruction: ADD", TODO_LINE);
                return 1;
            }
        } else {
            sp_log(SP_ERROR, "Line %d: Unknown instruction: \"%s\"", TODO_LINE, token->sb.data);
            return 1;
        }
    } while ((token = parser_next_token(&parser)));

    sp_log(SP_INFO, "%s", parser.generated.data);

    sp_da_free(&parser.generated);
    splexer_destroy(parser.splexer);
    return 0;
}
