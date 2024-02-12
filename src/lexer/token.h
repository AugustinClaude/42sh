#ifndef TOKEN_H
#define TOKEN_H

enum token_type
{
    TOKEN_OPERATOR,
    TOKEN_WORD,
    TOKEN_IO_NUMBER,
    TOKEN_EOF,
    TOKEN_ERROR,
    TOKEN_EMPTY,
};

enum token_id
{
    TOKEN_ID_IF,
    TOKEN_ID_THEN,
    TOKEN_ID_ELIF,
    TOKEN_ID_ELSE,
    TOKEN_ID_FI,
    TOKEN_ID_WHILE,
    TOKEN_ID_UNTIL,
    TOKEN_ID_FOR,
    TOKEN_ID_IN,
    TOKEN_ID_DO,
    TOKEN_ID_DONE,
    TOKEN_ID_LBRACE,
    TOKEN_ID_RBRACE,
    TOKEN_ID_LPAR,
    TOKEN_ID_RPAR,
    TOKEN_ID_NEG,
    TOKEN_ID_AND,
    TOKEN_ID_OR,
    TOKEN_ID_SEMI,
    TOKEN_ID_LF,
    TOKEN_ID_PIPE,
    TOKEN_ID_LESS,
    TOKEN_ID_GREAT,
    TOKEN_ID_CLOBBER,
    TOKEN_ID_DGREAT,
    TOKEN_ID_LESSAND,
    TOKEN_ID_GREATAND,
    TOKEN_ID_LESSGREAT,
    TOKEN_ID_IO_NUMBER,
    TOKEN_ID_ASSIGNMENT_WORD,
    TOKEN_ID_WORD,
    TOKEN_ID_EOF,
    TOKEN_ID_ERROR,
};

struct token
{
    enum token_type type;
    enum token_id id;
    char *value;
};

#endif /* ! TOKEN_H */
