#include "splexer.h"

typedef enum {
    INSTR_NIL,
    INSTR_LOAD,
    INSTR_ADD,
    INSTR_SL,
    INSTR_SR,
    __INSTR_COUNT__,
} Instr;

typedef enum { ARG_REG, ARG_IMM } Arg;
typedef enum { REG_NIL, REG_A, REG_B } Register;

typedef struct {
    Instr instr_rn;
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
    Sp_Lexer splexer = {0};

    if (splexer_init(&splexer, argv[1]) != 0) {
        sp_log(SP_ERROR, "Unable to open file: \"%s\"", argv[1]);
        return 1;
    }

    while (splexer.state != SPLEXER_TERMINATE) {
        splexer_tokenize(&splexer);
    }

    const Sp_Lexer_Token *token = NULL;
    const char *MSA = NULL;
    const char *MSB = NULL;
    const char *MSC = NULL;
    for (size_t i = 0; i < splexer.tokens.count; ++i) {
        token = &splexer.tokens.data[i];

        if (token->type == TOK_Newline) {
            continue;
        }

        switch (parser.instr_rn) {
            case INSTR_NIL:
                if (token->type != TOK_ID) {
                    sp_log(SP_ERROR, "Failed to parse unknown symbol \"%s\"", SPLEXER_TOKENS_LITERAL[token->type]);
                    return 1;
                }

                if (strcmp(token->sb.data, "LOAD") == 0) {
                    parser.instr_rn = INSTR_LOAD;
                } else {
                    sp_log(SP_ERROR, "Unknown instruction: \"%s\"", token->sb.data);
                    return 1;
                }
                break;
            case INSTR_LOAD:
                if (parser.lhs == REG_NIL) {
                    if (token->type != TOK_ID) {
                        sp_log(SP_ERROR, "Unknown symbol at lhs of LOAD");
                        return 1;
                    }

                    if (strcmp(token->sb.data, "REGA") == 0) {
                        parser.lhs = REG_A;
                        break;
                    } else if (strcmp(token->sb.data, "REGB") == 0) {
                        parser.lhs = REG_B;
                        break;
                    } else {
                        sp_log(SP_ERROR, "Unknown register \"%s\" at lhs of LOAD", token->sb.data);
                        return 1;
                    }
                } else {
                    switch (token->type) {
                        case TOK_ID:
                            parser.rhs_arg = ARG_REG;

                            if (strcmp(token->sb.data, "REGA") == 0) {
                                parser.rhs = REG_A;
                                break;
                            } else if (strcmp(token->sb.data, "REGB") == 0) {
                                parser.rhs = REG_B;
                                break;
                            } else {
                                sp_log(SP_ERROR, "Unknown register \"%s\" at rhs of LOAD", token->sb.data);
                                return 1;
                            }
                            break;
                        case TOK_IntLiteral:
                            parser.rhs_arg = ARG_IMM;
                            parser.rhs_imm = (uint8_t) token->int_lit.value;
                            break;
                        default:
                            sp_log(SP_ERROR, "Unknown symbol at rhs of LOAD");
                            return 1;
                    }

                    const char **load_reg;
                    switch (parser.lhs) {
                        case REG_A:
                            // keep REG_B constant
                            load_reg = &MSA;
                            MSB = "10";
                            MSC = "000";
                            break;
                        case REG_B:
                            // keep REG_A constant
                            MSA = "01";
                            load_reg = &MSB;
                            MSC = "001";
                            break;
                        case REG_NIL:
                        default:
                            sp_unreachable();
                    }
                    switch (parser.rhs_arg) {
                        case ARG_REG:
                            switch (parser.rhs) {
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
                    sp_sb_appendf(&parser.generated, "%s%s%s ", MSA, MSB, MSC);
                    MSA = NULL;
                    MSB = NULL;
                    MSC = NULL;
                    parser.instr_rn = INSTR_NIL;
                    parser.lhs = REG_NIL;
                    parser.rhs = REG_NIL;
                }
                break;
            case INSTR_ADD:
                break;
            case INSTR_SL:
                break;
            case INSTR_SR:
                break;
            case __INSTR_COUNT__:
            default:
                sp_unreachable();
                break;
        }
    }

    sp_log(SP_INFO, "%s", parser.generated.data);

    splexer_destroy(&splexer);
    return 0;
}
