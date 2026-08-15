#include "splexer.h"
#include <ctype.h>
#include <stddef.h>

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
        case 'n': return '\n';  // Newline
        case 't': return '\t';  // Horizontal tab
        case 'r': return '\r';  // Carriage return
        case 'b': return '\b';  // Backspace
        case 'a': return '\a';  // Alert/Bell
        case 'f': return '\f';  // Form feed
        case 'v': return '\v';  // Vertical tab
        case '\\': return '\\'; // Literal backslash
        case '\'': return '\''; // Single quote
        case '\"': return '\"'; // Double quote
        case '?': return '\?';  // Question mark
        default: return 0;
    }
}

int splexer_init(Sp_Lexer *splexer, const char *path) {
    if (!splexer) {
        return 1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        return 1;
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
                sp_ht_insert(&splexer->tok_table, sp_cstr_slice(SPLEXER_TOKEN_REGISTRY[i]), i);
                break;
        }
    }

    splexer_token_clear(splexer);

    char buffer;
    for (size_t count = 0; fread(&buffer, 1, 1, f) != 0; ++count) {
        if (buffer == '\n') {
            sp_da_push(&splexer->newlines, count);
        }
        sp_sb_appendf(&splexer->file, "%c", buffer);
    }

    fclose(f);
    return 0;
}

int splexer_token_append(Sp_Lexer *splexer, const char *c) {
    if (!c) return 0;

    if (splexer->tok.sv.count > 0 && (*c == '\n' || *c == ';')) {
        return 0;
    }
    switch (splexer->tok.type) {
        case TOK_ID:
            if (!splexer_char_is_valid_id(*c)) {
                return 0;
            }
            break;
        case TOK_IntLiteral:
            if ((splexer->tok.int_lit.suffixes.count > 0 && isalnum(*c)) || isalpha(*c)) {
                if (!splexer->tok.int_lit.suffixes.ptr) {
                    splexer->tok.int_lit.suffixes.ptr = c;
                    splexer->tok.int_lit.suffixes.count = 1;
                } else {
                    ++splexer->tok.int_lit.suffixes.count;
                }
                return 1;
            } else if (isdigit(*c)) {
                break;
            } else if (*c == '.') {
                splexer->tok.type = TOK_FloatLiteral;
                break;
            }
            return 0;
        case TOK_FloatLiteral:
            if ((splexer->tok.float_lit.suffixes.count > 0 && isalnum(*c)) || isalpha(*c)) {
                if (!splexer->tok.float_lit.suffixes.ptr) {
                    splexer->tok.float_lit.suffixes.ptr = c;
                    splexer->tok.float_lit.suffixes.count = 1;
                } else {
                    ++splexer->tok.float_lit.suffixes.count;
                }
                return 1;
            } else if (isdigit(*c)) {
                break;
            }
            return 0;
        case TOK_DQStringLiteral:
        case TOK_SQStringLiteral:
            // TODO: reimplement escape codes support with string view
            // if (splexer->tok.str.ptr[splexer->tok.str.count - 1] == '\\') {
            //     char escaped;
            //     if ((escaped = splexer_char_interpret_escape(*c))) {
            //         splexer->tok.str.ptr[splexer->tok.str.count - 1] = escaped;
            //         return 1;
            //     }
            // }

            // ending the string literal
            if (splexer->tok.type == TOK_DQStringLiteral && *c == '\"' && splexer->tok.sv.ptr[splexer->tok.sv.count - 1] != '\\') {
                return 2;
            }
            if (splexer->tok.type == TOK_SQStringLiteral && *c == '\'' && splexer->tok.sv.ptr[splexer->tok.sv.count - 1] != '\\') {
                return 2;
            }
            break;
        case TOK_Period:
            if (isdigit(*c) || tolower(*c) == 'f' || tolower(*c) == 'l') {
                splexer->tok.type = TOK_FloatLiteral;
                break;
            }
            goto def; // we go to default generic operator handling
        case TOK_Slash:
            if (*c == '/') {
                splexer->state = SPLEXER_COMMENT;
                splexer_token_clear(splexer);
                return 1;
            } else if (*c == '*') {
                splexer->state = SPLEXER_MULTICOMMENT;
                splexer_token_clear(splexer);
                return 1;
            }
            goto def;
        case TOK_Unknown:
            if (splexer->tok.sv.count == 0) {
                if (isdigit(*c)) {
                    splexer->tok.type = TOK_IntLiteral;
                    break;
                }
                switch (*c) {
                    case '\"':
                        splexer->tok.type = TOK_DQStringLiteral;
                        return 2; // consume the quote
                    case '\'':
                        splexer->tok.type = TOK_SQStringLiteral;
                        return 2;
                    default:
                        break;
                }
                if (splexer_char_is_valid_id(*c)) {
                    splexer->tok.type = TOK_ID;
                    break;
                }
            }
#ifdef GRANULAR_TOK_UNKNOWN
            else {
                return 0;
            }
#endif
            goto def;
        default:
        def:
            if (splexer_char_is_valid_id(*c)) {
                return 0;
            }
            if (!splexer->tok.sv.ptr) {
                splexer->tok.sv.ptr = c;
                splexer->tok.sv.count = 1;

            } else {
                ++splexer->tok.sv.count;
            }
            sp_ht_node_t(&splexer->tok_table) *query = NULL;
            sp_ht_get(&splexer->tok_table, splexer->tok.sv, &query);
            if (query) {
                splexer->tok.type = query->value;
            }
            return 1;
    }

    if (!splexer->tok.sv.ptr) {
        splexer->tok.sv.ptr = c;
        splexer->tok.sv.count = 1;

    } else {
        ++splexer->tok.sv.count;
    }
    return 1;
}

void splexer_token_clear(Sp_Lexer *splexer) {
    splexer->tok = (Sp_Lexer_Token) {
        .type = TOK_Unknown,
        .sv = {0},
    };
}

// TODO: replace with more efficient binary search later
Sp_Lexer_Token_Line splexer_token_get_line(Sp_Lexer *splexer, const Sp_Lexer_Token *token) {
    if (!splexer || !token) {
        return (Sp_Lexer_Token_Line) {
            .line = SIZE_MAX,
            .col = SIZE_MAX
        };
    }
    if (token->sv.ptr < splexer->file.data || token->sv.ptr >= splexer->file.data + splexer->file.count) {
        return (Sp_Lexer_Token_Line) {0};
    }
    size_t token_start = (size_t) (token->sv.ptr - splexer->file.data);

    Sp_Lexer_Token_Line data = {0};
    while (token_start > splexer->newlines.data[data.line]) {
        ++data.line;
    }

    if (data.line > 0) {
        data.col = token_start - splexer->newlines.data[data.line - 1];
    } else {
        data.col = token_start + 1;
    }

    ++data.line; // data.line is zero-indexed; we want to return the line number one-indexed

    return data;
}

void splexer_tokenize(Sp_Lexer *splexer) {
    sp_ht_node_t(&splexer->tok_table) *tok_query = NULL;
    int tok_status;
    for (; splexer->file_idx < splexer->file.count; ++splexer->file_idx) {

        switch (splexer->state) {
            case SPLEXER_IDLE:
                if (splexer->file.data[splexer->file_idx] == ' ') {
                    continue;
                } else if (splexer->file.data[splexer->file_idx] == '\n') {
                    continue;
                }
                splexer->state = SPLEXER_TOKENIZE;
                splexer_token_append(splexer, splexer->file.data + splexer->file_idx);
                break;
            case SPLEXER_TOKENIZE:
                // if token was not consumed or inserted, we begin lexing current token
                if ((tok_status = splexer_token_append(splexer, splexer->file.data + splexer->file_idx)) != 1) {
                    if (tok_status == 2) ++splexer->file_idx;
                    goto lex;
                }
                break;
            case SPLEXER_COMMENT:
                if (splexer->file.data[splexer->file_idx] != '\n') {
                    continue;
                }
                splexer->state = SPLEXER_IDLE;
                break;
            case SPLEXER_MULTICOMMENT:
                if (splexer->file.data[splexer->file_idx - 1] == '*' && splexer->file.data[splexer->file_idx] == '/') {
                    splexer->state = SPLEXER_IDLE;
                }
                break;
            case SPLEXER_TERMINATE:
                sp_log(SP_ERROR, "splexer_tokenize cannot continue as the lexer as terminated!");
                return;
        }
    }

    if (!splexer->tok.sv.ptr) {
        goto done;
    }

lex:
    switch (splexer->tok.type) {
        case TOK_Unknown:
        case TOK_ID:
        case TOK_DQStringLiteral:
        case TOK_SQStringLiteral:
            sp_da_push(&splexer->tokens, splexer->tok);
            goto done;
            // token.type = TOK_ID;
            // sp_sb_appendf(&token.sb, "%s", splexer->tok.sb.data);
            // sp_da_push(&splexer->tokens, token);
            // goto done;
        case TOK_IntLiteral:
            splexer->tok.int_lit.value = strtol(splexer->tok.sv.ptr, NULL, 10);
            sp_da_push(&splexer->tokens, splexer->tok);
            goto done;
        case TOK_FloatLiteral:
            splexer->tok.float_lit.value = strtof(splexer->tok.sv.ptr, NULL);
            sp_da_push(&splexer->tokens, splexer->tok);
            goto done;
        default:
            while (splexer->tok.sv.count) {
                sp_ht_get(&splexer->tok_table, splexer->tok.sv, &tok_query);
                if (tok_query) {
                    sp_da_push(&splexer->tokens, splexer->tok);
                    goto done;
                } else {
                    /* If the current token is NOT found, we pop one character off the end and attempt again */
                    --splexer->tok.sv.count;
                    --splexer->file_idx;
                }
            }

            /* The operator does not exist */
            sp_log(SP_ERROR, "splexer_tokenize could not find any matching operator! Terminating...");
            splexer->state = SPLEXER_TERMINATE;
            return;
    }

done:
    splexer_token_clear(splexer);
    splexer->state = splexer->file_idx >= splexer->file.count ? SPLEXER_TERMINATE : SPLEXER_IDLE;
}

void splexer_destroy(Sp_Lexer *splexer) {
    sp_da_free(&splexer->file);
    sp_da_free(&splexer->newlines);

    sp_ht_free(&splexer->tok_table);

    sp_da_free(&splexer->tokens);
}
