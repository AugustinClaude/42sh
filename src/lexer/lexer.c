#define _POSIX_C_SOURCE 200809L

#include "lexer.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "environment/environment.h"
#include "error_handler/error_handler.h"
#include "io_backend/io.h"
#include "token_recognition.h"
#include "utils/fifo.h"
#include "utils/hash_map.h"
#include "utils/string_utils.h"

struct lexer *new_lexer(void)
{
    struct lexer *lexer = malloc(sizeof(struct lexer));
    if (!lexer)
        return NULL;

    lexer->buffer = calloc(1, sizeof(char));
    if (!lexer->buffer)
        return NULL;
    lexer->buffer[0] = '\0';
    lexer->size = 0;
    lexer->current_tok = (struct token){ .type = TOKEN_EMPTY,
                                         .id = TOKEN_ID_ERROR,
                                         .value = NULL };
    lexer->next_tok = (struct token){ .type = TOKEN_EMPTY,
                                      .id = TOKEN_ID_ERROR,
                                      .value = NULL };
    lexer->token_buf = fifo_init();

    return lexer;
}

void free_lexer(struct lexer *lexer)
{
    if (!lexer)
        return;

    free(lexer->buffer);
    free(lexer->current_tok.value);
    fifo_destroy(lexer->token_buf);
    free(lexer);
}

static void reset_lexer(struct lexer *lexer)
{
    if (!lexer)
        return;

    lexer->size = 0;
    lexer->buffer = realloc(lexer->buffer, lexer->size + 1);
    lexer->buffer[0] = '\0';
}

int append_buffer(struct lexer *lexer, char c)
{
    if (string_append_char(&(lexer->buffer), lexer->size, c) == -1)
        return -1;

    lexer->size++;
    return 0;
}

static void expand_alias(struct lexer *lexer, int could_be_alias)
{
    // Expand alias if the current token is a word
    if (could_be_alias && lexer->current_tok.id == TOKEN_ID_WORD)
    {
        struct hash_map *alias_hash_map = get_alias_hash_map();
        char *value = hash_map_get(alias_hash_map, lexer->current_tok.value);

        // If alias exists
        if (value)
        {
            struct lexer *alias_lexer = new_lexer();
            struct io_backend *alias_io = new_io_string(value, 0);

            struct token tok = lexer_peek(alias_lexer, alias_io);
            while (tok.type != TOKEN_EOF)
            {
                struct token tmp = (struct token){ .type = tok.type,
                                                   .id = tok.id,
                                                   .value = strdup(tok.value) };
                fifo_push(lexer->token_buf, tmp);
                lexer_pop(alias_lexer);
                tok = lexer_peek(alias_lexer, alias_io);
            }

            free_lexer(alias_lexer);
            free_backend(alias_io);

            if (lexer->token_buf->size != 0)
            {
                char *val = hash_map_get(alias_hash_map, lexer->token_buf->head->data.value);
                while (val)
                {
                    val = hash_map_get(alias_hash_map, lexer->token_buf->head->data.value);
                }
            }
        }
    }
}

static void lexer_next(struct lexer *lexer, struct io_backend *io,
                       int could_be_alias)
{
    if (!lexer)
        return;

    free(lexer->current_tok.value);
    lexer->current_tok.type = TOKEN_ERROR;
    lexer->current_tok.id = TOKEN_ID_ERROR;
    lexer->current_tok.value = NULL;

    char c = io_peek(io);
    if (c == ' ')
    {
        io_next(io);
        c = io_peek(io);
    }

    // If there is no more to read in the io backend return NULL to tell parser
    // to stop aswell
    if (c == EOF)
    {
        lexer->current_tok.type = TOKEN_EOF;
        lexer->current_tok.id = TOKEN_ID_EOF;
        return;
    }

    // Main lexer loop
    enum lexer_status status = LEXER_OK;
    while (status == LEXER_OK && c != EOF)
    {
        c = io_peek(io);
        if (c == '\'')
            status = lex_single_quotes(lexer, io);
        else if (c == '"')
            status = lex_double_quotes(lexer, io);
        else if (c == '\\')
            status = lex_backslash(lexer, io);
        else if (c == '$')
            status = lex_dollar(lexer, io);
        else if (c == '`')
            status = lex_backtick(lexer, io);
        else
        {
            enum lexer_status stat = lexer_token_recognition(lexer, c, io);
            if (stat == LEXER_DELIMIT)
                break;

            io_next(io);
            c = io_peek(io);
        }
    }

    // If lexer had an error, throw it
    if (status == LEXER_ERROR || lexer->current_tok.type == TOKEN_ERROR)
        exit_print(2, "lexer error");

    // Duplicates buffer string to avoid dangling freed pointers issues
    lexer->current_tok.value = strdup(lexer->buffer);

    // Set token ID
    token_recognize_id(lexer);

    // Expand alias if needed
    expand_alias(lexer, could_be_alias);

    // Reset lexer for next token
    reset_lexer(lexer);
}

struct token lexer_peek(struct lexer *lexer, struct io_backend *io)
{
    if (lexer->token_buf->size != 0 && lexer->current_tok.type == TOKEN_EMPTY)
    {
        free(lexer->current_tok.value);
        struct token tok = lexer->token_buf->head->data;
        lexer->current_tok.type = tok.type;
        lexer->current_tok.id = tok.id;
        lexer->current_tok.value = strdup(tok.value);
    }

    // Try to get current_tok
    if (lexer->current_tok.type != TOKEN_EMPTY)
        return lexer->current_tok;

    // If neither current_tok nor next_tok is filled, we need to fetch a token
    else
        lexer_next(lexer, io, 0);
    return lexer->current_tok;
}

struct token lexer_peek2(struct lexer *lexer, struct io_backend *io)
{
    if (lexer->token_buf->size > 1 && lexer->next_tok.type == TOKEN_EMPTY)
    {
        free(lexer->next_tok.value);
        struct token tok = lexer->token_buf->head->next->data;
        lexer->next_tok.type = tok.type;
        lexer->next_tok.id = tok.id;
        lexer->next_tok.value = strdup(tok.value);
    }

    if (lexer->next_tok.type != TOKEN_EMPTY)
        return lexer->next_tok;

    // Backup current_tok
    struct token tmp = lexer->current_tok;

    lexer->current_tok = (struct token){ TOKEN_EMPTY, TOKEN_ID_ERROR, NULL };
    lexer_next(lexer, io, 0);
    lexer->next_tok = lexer->current_tok;
    lexer->current_tok = tmp;
    return lexer->next_tok;
}

// Set to TOKEN_EMPTY to tell lexer_peek that it needs to do a lexing loop next
// time
void lexer_pop(struct lexer *lexer)
{
    if (lexer->token_buf->size != 0)
        fifo_pop(lexer->token_buf);

    reset_lexer(lexer);
    // If next_tok contains something, shift it in the current_tok and empty it
    if (lexer->next_tok.type != TOKEN_EMPTY)
    {
        free(lexer->current_tok.value);
        lexer->current_tok = lexer->next_tok;
        lexer->next_tok.type = TOKEN_EMPTY;
    }
    else
        lexer->current_tok.type = TOKEN_EMPTY;
}

// Lexer peek for alias expansion (parser_simple_cmd calls this function to
// give context to the lexer so it knows when to expand aliases)
struct token lexer_peek_alias(struct lexer *lexer, struct io_backend *io)
{
    if (lexer->token_buf->size != 0 && lexer->current_tok.type == TOKEN_EMPTY)
    {
        free(lexer->current_tok.value);
        struct token tok = lexer->token_buf->head->data;
        lexer->current_tok.type = tok.type;
        lexer->current_tok.id = tok.id;
        lexer->current_tok.value = strdup(tok.value);
    }

    // Try to get current_tok
    if (lexer->current_tok.type != TOKEN_EMPTY)
        return lexer->current_tok;

    // If neither current_tok nor next_tok is filled, we need to fetch a token
    else
        lexer_next(lexer, io, 1);
    return lexer->current_tok;
}
