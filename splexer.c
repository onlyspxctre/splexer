#include "splexer.h"

void splexer_ast_free(Sp_Lexer_Ast* ast) {
    for (size_t i = 0; i < ast->count; ++i) {
        ast->data[i].type = 0;
        sp_da_free(&ast->data[i].str);
    }
}

int splexer_char_is_valid(char c) {
    if (isalnum(c) || c == '_') {
        return 1;
    }
    return 0;
}

void splexer_init(Sp_Lexer* splexer, const char* path, char** keywords, char** operators) {
    for (int i = 0; i < TOK_KW_Unknown; ++i) {
        if (!keywords[i]) {
            sp_log(SP_WARNING, "No matching token found in kw_table! Skipping...");
            continue;
        }
        sp_ht_insert(&splexer->kw_table, keywords[i], i);
    }
    for (int i = 0; i < TOK_OP_Unknown; ++i) {
        if (!operators[i]) {
            sp_log(SP_WARNING, "No matching token found in op_table! Skipping...");
            continue;
        }
        sp_ht_insert(&splexer->op_table, operators[i], i);
    }

    splexer->f = fopen(path, "rb");
}

Sp_Lexer_Ast_Type splexer_ast_eval_type(char c) {
    if (splexer_char_is_valid(c)) {
        return AST_IDENTIFIER;
    } else {
        return AST_OPERATOR;
    }
}

int splexer_token_append(Sp_Lexer_Token* token, char c) {
    switch (token->type) {
        case AST_IDENTIFIER:
            if (!splexer_char_is_valid(c)) {
                return 0; /* False */
            }
            break;
        case AST_INTLITERAL:
            if (!isdigit(c)) {
                if (c == '.') {
                    token->type = AST_FLOATLITERAL;
                }
                else if (splexer_char_is_valid(c)) {
                    token->type = AST_IDENTIFIER;
                }
                else {
                    return 0;
                }
            }
            break;
        case AST_FLOATLITERAL: /* TODO: suffixes to determine literal type */
            if (!isdigit(c)) {
                // Cannot append non-digit character after decimal point on a float literal
                return 0;
            }
            break;
        case AST_OPERATOR:
            if (splexer_char_is_valid(c)) {
                return 0;
            }
            break;
        case AST_UNKNOWN:
            if (isdigit(c)) {
                token->type = AST_INTLITERAL;
            }
            else if (splexer_char_is_valid(c)) {
                token->type = AST_IDENTIFIER;
            }
            else {
                token->type = AST_OPERATOR;
            }
            break;
        default:
            sp_unreachable();
            break;
    }

    sp_sb_appendf(&token->sb, "%c", c);
    return 1;
}

void splexer_token_clear(Sp_Lexer_Token* token) {
    sp_da_clear(&token->sb);
    token->type = AST_UNKNOWN;
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
                splexer->state = SPLEXER_TOKENIZE;
                splexer_token_append(&splexer->tok, *buffer);
                break;
            case SPLEXER_TOKENIZE:
                /* TODO: Revamp the system yet again, smart type evaluation and float literal handling */
                if (!splexer_token_append(&splexer->tok, *buffer)) {
                    if (*buffer != ' ') {
                        fseek(splexer->f, -1, SEEK_CUR);
                    }
                    goto lex;
                }
                break;
            case SPLEXER_TERMINATE:
                sp_log(SP_ERROR, "splexer_tokenize cannot continue as the lexer as terminated!");
                return;
        }
    }

lex:
    switch (splexer->tok.type) {
        case AST_IDENTIFIER:
            kw_query = sp_ht_get(&splexer->kw_table, splexer->tok.sb.data);
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
                sp_sb_appendf(&node.str, "\'%s\' ", splexer->tok.sb.data);

                sp_da_push(&splexer->ast, node);
            }
            goto done;
        case AST_INTLITERAL:
        case AST_FLOATLITERAL:
            1 + 2; // TODO: Remove
            Sp_Lexer_Ast_Node node = (Sp_Lexer_Ast_Node) {
                .type = splexer->tok.type,
                .str = {0},
            };
            sp_sb_appendf(&node.str, "\'%s\' ", splexer->tok.sb.data);

            sp_da_push(&splexer->ast, node);
            goto done;
        case AST_OPERATOR:
            while (splexer->tok.sb.count) {
                op_query = sp_ht_get(&splexer->op_table, splexer->tok.sb.data);

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
                    sp_da_pop(&splexer->tok.sb);
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
    splexer_token_clear(&splexer->tok);
    splexer->state = feof(splexer->f) ? SPLEXER_TERMINATE : SPLEXER_IDLE;
}

void splexer_destroy(Sp_Lexer* splexer) {
    fclose(splexer->f);
    splexer->f = NULL;

    sp_ht_free(&splexer->kw_table);
    sp_ht_free(&splexer->op_table);

    sp_da_free(&splexer->tok.sb);
    splexer_ast_free(&splexer->ast);
}
