#include "ast_fundec.h"

#include <stdlib.h>

#include "ast.h"
#include "execution/execution.h"

struct ast_node *init_ast_fundec(void)
{
    struct ast_fundec *res = malloc(sizeof(struct ast_fundec));
    if (!res)
        return NULL;

    static struct ast_ftable ftable = {
        .run = &execution_fundec,
        .free = &free_ast_fundec,
    };

    res->base.type = AST_FUNDEC;
    res->base.ftable = &ftable;

    res->cmd = NULL;
    res->name = NULL;
    res->redir = NULL;
    res->is_in_hash_map = 0;

    struct ast_node *base_res = &(res->base);
    return base_res;
}

void free_ast_fundec(struct ast_node *ast)
{
    struct ast_fundec *ast_fundec = (struct ast_fundec *)ast;
    if (!ast_fundec->is_in_hash_map)
    {
        free_ast(ast_fundec->cmd);
        free(ast_fundec->name);
        if (ast_fundec->redir)
            free_ast((struct ast_node *)ast_fundec->redir);
        free(ast_fundec);
    }
}
