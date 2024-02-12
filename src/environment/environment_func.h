#ifndef ENVIRONMENT_FUNC_H
#define ENVIRONMENT_FUNC_H

#define HASH_SIZE 16

#include "ast/ast.h"

void init_hash_map_func(void);

struct hash_map_func *get_hash_map_func(void);

void free_hash_map_func(void);

void free_ast_to_free(void);

void add_ast_to_free(struct ast_node *ast);

#endif /* ! ENVIRONMENT_FUNC_H */
