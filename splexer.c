#include "splexer.h"
#include <ctype.h>

bool splexer_char_is_valid(char c) {
    if (isalnum(c) || c == '_') {
        return true;
    }
    return false;
}

void splexer_init(Sp_Lexer* splexer, const char* path, const char** keywords, const char** operators) {
    if (!splexer) return;
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

int splexer_token_append(Sp_Lexer_Token* token, char c) {
    switch (token->type) {
        case TOK_TYPE_IDENTIFIER:
            if (!splexer_char_is_valid(c)) {
                return 0; /* False */
            }
            break;
        case TOK_TYPE_INTLITERAL:
            if (!isdigit(c)) {
                if (c == '.') {
                    token->type = TOK_TYPE_FLOATLITERAL;
                }
                else if (splexer_char_is_valid(c)) {
                    token->type = TOK_TYPE_IDENTIFIER;
                }
                else {
                    return 0;
                }
            }
            break;
        case TOK_TYPE_FLOATLITERAL: /* TODO: suffixes to determine literal type */
            if (!isdigit(c)) {
                if (tolower(c) == 'f' || tolower(c) == 'l') { // suffix literal type
                    break;
                }

                // Cannot append non-digit character after decimal point on a float literal
                return 0;
            }
            break;
        case TOK_TYPE_OPERATOR:
            if (token->sb.data[token->sb.count - 1] == '.') {
                if (isdigit(c) || tolower(c) == 'f' || tolower(c) == 'l') {
                    token->type = TOK_TYPE_FLOATLITERAL;
                    break;
                }
            }
            if (splexer_char_is_valid(c)) {
                return 0;
            }
            break;
        case TOK_TYPE_UNKNOWN:
            if (isdigit(c)) {
                token->type = TOK_TYPE_INTLITERAL;
            }
            else if (splexer_char_is_valid(c)) {
                token->type = TOK_TYPE_IDENTIFIER;
            }
            else {
                token->type = TOK_TYPE_OPERATOR;
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
    token->type = TOK_TYPE_UNKNOWN;
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
        case TOK_TYPE_IDENTIFIER:
            kw_query = sp_ht_get(&splexer->kw_table, splexer->tok.sb.data);
            if (kw_query) {
                Sp_Lexer_Token token = (Sp_Lexer_Token) {
                    .type = TOK_TYPE_KEYWORD,
                    .sb = {0},
                };
                sp_sb_appendf(&token.sb, "\'%s\' ", kw_query->key);

                sp_da_push(&splexer->tokens, token);
            } else {
                Sp_Lexer_Token token = (Sp_Lexer_Token) {
                    .type = TOK_TYPE_KEYWORD,
                    .sb = {0},
                };
                sp_sb_appendf(&token.sb, "\'%s\' ", splexer->tok.sb.data);

                sp_da_push(&splexer->tokens, token);
            }
            goto done;
        case TOK_TYPE_INTLITERAL:
        case TOK_TYPE_FLOATLITERAL:
            1 + 2; // TODO: Remove
            Sp_Lexer_Token token = (Sp_Lexer_Token) {
                .type = splexer->tok.type,
                .sb = {0},
            };
            sp_sb_appendf(&token.sb, "\'%s\' ", splexer->tok.sb.data);

            sp_da_push(&splexer->tokens, token);
            goto done;
        case TOK_TYPE_OPERATOR:
            while (splexer->tok.sb.count) {
                op_query = sp_ht_get(&splexer->op_table, splexer->tok.sb.data);

                if (op_query) {
                    Sp_Lexer_Token token = (Sp_Lexer_Token) {
                        .type = TOK_TYPE_OPERATOR,
                        .sb = {0},
                    };

                    if (*op_query->key == '\n') {
                        sp_sb_appendf(&token.sb, "\n");
                    } else {
                        sp_sb_appendf(&token.sb, "\'%s\' ", op_query->key);
                    }

                    sp_da_push(&splexer->tokens, token);
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
    if (splexer->f) {
        fclose(splexer->f);
    }
    splexer->f = NULL;

    sp_ht_free(&splexer->kw_table);
    sp_ht_free(&splexer->op_table);

    sp_da_free(&splexer->tok.sb);
    for (size_t i = 0; i < splexer->tokens.count; ++i) {
        sp_da_free(&splexer->tokens.data[i].sb);
    }
    sp_da_free(&splexer->tokens);
}
