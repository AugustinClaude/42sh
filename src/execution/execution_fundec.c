#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast/ast.h"
#include "ast/ast_fundec.h"
#include "environment/environment.h"
#include "environment/environment_func.h"
#include "execution.h"
#include "redirection/redirection.h"
#include "utils/hash_map_func.h"

struct exit_status execution_fundec(struct ast_node *ast)
{
    struct ast_fundec *ast_fundec = (struct ast_fundec *)ast;

    struct hash_map_func *hash_map = get_hash_map_func();
    int b = 0;
    hash_map_func_insert(hash_map, strdup(ast_fundec->name), ast, &b);

    struct exit_status exit_status;
    exit_status.exit_code = 0;
    exit_status.exit_action = EXIT_OK;
    return exit_status;
}

struct exit_status execute_function(struct ast_node *ast)
{
    struct ast_fundec *ast_fundec = (struct ast_fundec *)ast;

    // Redirections
    // We make a backup array to store the original file descriptors value
    // before redirections (only affects the scope of this node)
    int fd_backup[MAX_FD];

    memset(fd_backup, -1, sizeof(fd_backup));

    redir_execute_all((struct ast_redir *)ast_fundec->redir, fd_backup);

    // Execute child node
    struct exit_status exit_status = execution_ast(ast_fundec->cmd);

    restore_fd(fd_backup);
    return exit_status;
}
