#include "splexer.h"
#include <ctype.h>

bool splexer_char_is_valid_id(char c) {
    if (isalnum(c) || c == '_') {
        return true;
    }
    return false;
}

/* Takes an escaped character and returns its escaped equivalent, if available.
 *
 * If the escaped character is invalid, this function returns 0. */
static inline char splexer_char_interpret_escape(char escaped_char) {
    switch (escaped_char) {
        case 'n':  return '\n'; // Newline
        case 't':  return '\t'; // Horizontal tab
        case 'r':  return '\r'; // Carriage return
        case 'b':  return '\b'; // Backspace
        case 'a':  return '\a'; // Alert/Bell
        case 'f':  return '\f'; // Form feed
        case 'v':  return '\v'; // Vertical tab
        case '\\': return '\\'; // Literal backslash
        case '\'': return '\''; // Single quote
        case '\"': return '\"'; // Double quote
        case '?':  return '\?'; // Question mark
        default:   return 0;
    }
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
            case TOK_DQStringLiteral:
            case TOK_SQStringLiteral:
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

int splexer_token_append(Sp_Lexer *splexer, char c) {
    if (splexer->tok.sb.count > 0 && (c == '\n' || c == ';')) {
        return 0;
    }
    switch (splexer->tok.type) {
        case TOK_ID:
            if (!splexer_char_is_valid_id(c)) {
                return 0;
            }
            break;
        case TOK_IntLiteral:
            if (splexer->tok.int_lit.suffixes_count >= 31) {
                return 2;
            }

            if (splexer->tok.int_lit.suffixes_count > 0 || isalpha(c)) {
                splexer->tok.int_lit.suffixes[splexer->tok.int_lit.suffixes_count++] = c;
                splexer->tok.int_lit.suffixes[splexer->tok.int_lit.suffixes_count] = '\0';
                return 1;
            } else if (isdigit(c)) {
                break;
            }
            else if (c == '.') {
                splexer->tok.type = TOK_FloatLiteral;
                break;
            }
            return 0;
        case TOK_FloatLiteral:
            if (splexer->tok.float_lit.suffixes_count >= 31) {
                return 2;
            }

            if (splexer->tok.float_lit.suffixes_count > 0 || isalpha(c)) {
                splexer->tok.float_lit.suffixes[splexer->tok.float_lit.suffixes_count++] = c;
                splexer->tok.float_lit.suffixes[splexer->tok.float_lit.suffixes_count] = '\0';
                return 1;
            } else if (isdigit(c)) {
                break;
            }
            return 0;
        case TOK_DQStringLiteral:
        case TOK_SQStringLiteral:
            if (splexer->tok.sb.data[splexer->tok.sb.count - 1] == '\\') {
                char escaped;
                if ((escaped = splexer_char_interpret_escape(c))) {
                    splexer->tok.sb.data[splexer->tok.sb.count - 1] = escaped;
                    return 1;
                }
            }

            // ending the string literal
            if (splexer->tok.type == TOK_DQStringLiteral && c == '\"') {
                return 2;
            }
            if (splexer->tok.type == TOK_SQStringLiteral && c == '\'') {
                return 2;
            }
            break;
        case TOK_Period:
            if (isdigit(c) || tolower(c) == 'f' || tolower(c) == 'l') {
                splexer->tok.type = TOK_FloatLiteral;
                break;
            }
            goto def; // we go to default generic operator handling
        case TOK_Unknown:
            if (isdigit(c)) {
                splexer->tok.type = TOK_IntLiteral;
                break;
            }
            switch (c) {
                case '\"':
                    splexer->tok.type = TOK_DQStringLiteral;
                    return 2; // consume the quote
                case '\'':
                    splexer->tok.type = TOK_SQStringLiteral;
                    return 2;
                default:
                    break;
            }
            if (splexer_char_is_valid_id(c)) {
                splexer->tok.type = TOK_ID;
                break;
            }
            goto def;
        default:
        def:
            if (splexer_char_is_valid_id(c)) {
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

void splexer_token_clear(Sp_Lexer *splexer) {
    sp_da_clear(&splexer->tok.sb);
    splexer->tok = (Sp_Lexer_Token) {
        .type = TOK_Unknown,
        .sb = splexer->tok.sb,
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
                // if token was not consumed or inserted, we begin lexing current token
                if ((tok_status = splexer_token_append(splexer, *buffer)) != 1) {
                    if (tok_status == 0) fseek(splexer->f, -1, SEEK_CUR);
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
        case TOK_ID:
        case TOK_DQStringLiteral:
        case TOK_SQStringLiteral:
            token.type = splexer->tok.type;
            sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            sp_da_push(&splexer->tokens, token);
            goto done;
            token.type = TOK_ID;
            sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            sp_da_push(&splexer->tokens, token);
            goto done;
        case TOK_IntLiteral:
            token.type = splexer->tok.type;
            token.int_lit = splexer->tok.int_lit;
            token.int_lit.value = atol(splexer->tok.sb.data);
            sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            sp_da_push(&splexer->tokens, token);
            goto done;
        case TOK_FloatLiteral:
            token.type = splexer->tok.type;
            token.float_lit = splexer->tok.float_lit;
            token.float_lit.value = atof(splexer->tok.sb.data);
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
