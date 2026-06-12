#include "splexer.h"
#include <ctype.h>

bool splexer_char_is_valid(char c) {
    if (isalnum(c) || c == '_') {
        return true;
    }
    return false;
}

void splexer_init(Sp_Lexer *splexer, const char *path) {
    if (!splexer) {
        return;
    }
    for (Sp_Lexer_Tokens i = 0; i < TOK_Unknown; ++i) {
        switch (i) {
            case TOK_ID:
            case TOK_IntLiteral:
            case TOK_FloatLiteral:
                break;
            default:
                if (!SPLEXER_TOKEN_REGISTRY[i]) {
                    sp_log(SP_WARNING, "No matching token found in kw_table! Skipping...");
                    continue;
                }
                sp_ht_insert(&splexer->tok_table, SPLEXER_TOKEN_REGISTRY[i], i);
                break;
        }
    }
    splexer->f = fopen(path, "rb");
    splexer_token_clear(splexer);
}

/* Evaluates whether a given character `c` could be appended to the current working token.
 *
 * Returns 1 if the given character was appended, or 2 if the given character was consumed.
 * If the given character cannot be inserted nor was consumed, this function returns 0.
 * */
int splexer_token_append(Sp_Lexer *splexer, char c) {
    if (splexer->tok.sb.count > 0 && (c == '\n' || c == ';')) {
        return 0;
    }
    switch (splexer->tok.type) {
        case TOK_ID:
            if (!splexer_char_is_valid(c)) {
                return 0;
            }
            break;
        case TOK_IntLiteral:
            if (!isdigit(c)) {
                if (c == '.') {
                    splexer->tok.type = TOK_FloatLiteral;
                } else {
                    return 0;
                }
            }
            break;
        case TOK_FloatLiteral:
            if (splexer->tok.float_lit.suffixes_count > 0) {
                return 2;
            }

            if (isdigit(c)) {
                break;
            }

            if (isalpha(c)) {
                splexer->tok.float_lit.suffixes[splexer->tok.float_lit.suffixes_count++] = c;
                splexer->tok.float_lit.suffixes[splexer->tok.float_lit.suffixes_count] = '\0';
                return 1;
            }
            break;
        case TOK_Period:
            if (isdigit(c) || tolower(c) == 'f' || tolower(c) == 'l') {
                splexer->tok.type = TOK_FloatLiteral;
                break;
            }
            goto def;
        case TOK_Unknown:
            if (isdigit(c)) {
                splexer->tok.type = TOK_IntLiteral;
                break;
            }
            if (splexer_char_is_valid(c)) {
                splexer->tok.type = TOK_ID;
                break;
            }
            goto def;
        default:
        def:
            if (splexer_char_is_valid(c)) {
                return 0;
            }
            sp_sb_appendf(&splexer->tok.sb, "%c", c);
            sp_ht_node_t(&splexer->tok_table) *query = NULL;
            sp_ht_get(&splexer->tok_table, splexer->tok.sb.data, &query);
            if (query) {
                splexer->tok.type = query->value;
            }
            return 1;
    }

    sp_sb_appendf(&splexer->tok.sb, "%c", c);
    return 1;
}

void splexer_token_clear(Sp_Lexer* splexer) {
    sp_da_clear(&splexer->tok.sb);
    splexer->tok = (Sp_Lexer_Token) {
        .type = TOK_Unknown,
    };
}

void splexer_tokenize(Sp_Lexer *splexer) {
    sp_ht_node_t(&splexer->tok_table) *tok_query = NULL;
    char buffer[2] = "\0";

    Sp_Lexer_Token token = (Sp_Lexer_Token) {
        .type = TOK_Unknown,
        .sb = {0},
    };

    int tok_status;
    while (fread(buffer, 1, 1, splexer->f) != 0) {
        switch (splexer->state) {
            case SPLEXER_IDLE:
                if (*buffer == ' ') {
                    continue;
                }
                splexer->state = SPLEXER_TOKENIZE;
                splexer_token_append(splexer, *buffer);
                break;
            case SPLEXER_TOKENIZE:
                /* TODO: Revamp the system yet again, smart type evaluation and float literal handling */
                if (!(tok_status = splexer_token_append(splexer, *buffer))) {
                    fseek(splexer->f, -1, SEEK_CUR);
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
        case TOK_Unknown:
            token.type = TOK_Unknown;
            sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            sp_da_push(&splexer->tokens, token);
            goto done;
        case TOK_ID:
            token.type = TOK_ID;
            sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            sp_da_push(&splexer->tokens, token);
            goto done;
        case TOK_IntLiteral:
        case TOK_FloatLiteral:
            token.type = splexer->tok.type;
            token.float_lit = splexer->tok.float_lit;
            sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            sp_da_push(&splexer->tokens, token);
            goto done;
        default:
            while (splexer->tok.sb.count) {
                sp_ht_get(&splexer->tok_table, splexer->tok.sb.data, &tok_query);
                if (tok_query) {
                    Sp_Lexer_Token token = (Sp_Lexer_Token) {
                        .type = tok_query->value,
                        .sb = {0},
                    };

                    if (*tok_query->key == '\n') {
                        sp_sb_appendf(&token.sb, "\\n");
                    } else {
                        sp_sb_appendf(&token.sb, "%s", tok_query->key);
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
    }

done:
    splexer_token_clear(splexer);
    splexer->state = feof(splexer->f) ? SPLEXER_TERMINATE : SPLEXER_IDLE;
}

void splexer_destroy(Sp_Lexer *splexer) {
    if (splexer->f) {
        fclose(splexer->f);
    }
    splexer->f = NULL;

    sp_ht_free(&splexer->tok_table);

    sp_da_free(&splexer->tok.sb);
    for (size_t i = 0; i < splexer->tokens.count; ++i) {
        sp_da_free(&splexer->tokens.data[i].sb);
    }
    sp_da_free(&splexer->tokens);
}
