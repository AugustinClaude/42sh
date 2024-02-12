#include "environment_func.h"

#include <stdlib.h>

#include "ast/ast.h"
#include "utils/hash_map_func.h"

static struct hash_map_func *function_hash_map;
static struct ast_node **ast_to_free;
static int size_ast_to_free;

void init_hash_map_func(void)
{
    size_ast_to_free = 0;
    function_hash_map = hash_map_func_init(HASH_SIZE);
    ast_to_free = calloc(1, sizeof(struct ast_node **));
}

struct hash_map_func *get_hash_map_func(void)
{
    return function_hash_map;
}

void free_hash_map_func(void)
{
    hash_map_func_free(function_hash_map);
    free(ast_to_free);
}

void free_ast_to_free(void)
{
    for (int i = 0; ast_to_free[i] != NULL; i++)
    {
        free_ast(ast_to_free[i]);
    }
    free(ast_to_free);
    ast_to_free = calloc(1, sizeof(struct ast_node **));
    size_ast_to_free = 0;
}

void add_ast_to_free(struct ast_node *ast)
{
    size_ast_to_free++;
    ast_to_free = realloc(ast_to_free,
                          sizeof(struct ast_node **) * (size_ast_to_free + 2));
    ast_to_free[size_ast_to_free - 1] = ast;
    ast_to_free[size_ast_to_free] = NULL;
}
