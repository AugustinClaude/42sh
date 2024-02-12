#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "ast/ast_simple_cmd.h"
#include "builtin/builtin.h"
#include "environment/arguments.h"
#include "environment/environment.h"
#include "environment/environment_func.h"
#include "environment/var.h"
#include "error_handler/error_handler.h"
#include "execution.h"
#include "expansion/expansion.h"
#include "redirection/redirection.h"
#include "utils/argv_utils.h"
#include "utils/hash_map.h"
#include "utils/hash_map_func.h"

static int get_exec_status(pid_t pid)
{
    int status;
    waitpid(pid, &status, 0);
    int exit_status = WEXITSTATUS(status);
    // Error should be <= 127
    if (exit_status > 127)
        exit_status = 127;
    return exit_status;
}

// Restore the words array from a backup
static void restore_words(struct ast_simple_cmd *ast, char **backup)
{
    free_argv(ast->words);
    ast->words = backup;
}

static enum expansion_status var_and_expansion(struct var *var, char ***words)
{
    // Words expansion
    enum expansion_status status = words_expansion(words);
    if (status != EXPANSION_OK)
        return status;

    // Expand and assign variables
    return var_assignation(var);
}

// check if it's a function and call it
static int exec_func(char *key, struct exit_status *return_func,
                     struct ast_simple_cmd *ast_simple_cmd)
{
    struct hash_map_func *hash_map = get_hash_map_func();
    struct ast_node *ast = hash_map_func_get(hash_map, key);
    if (ast == NULL)
        return 0;
    char **ex_arg = get_current_args();
    set_current_args(ast_simple_cmd->words + 1);
    (void)ast_simple_cmd;
    *return_func = execute_function(ast);
    set_current_args(ex_arg);
    return 1;
}

// In case of empty words array with assignments, we need to restore the old
// values of the variables assigned for the command
static void restore_var(struct var *begin)
{
    if (!begin)
        return;
    struct hash_map *var_hash = get_hash_map();
    struct var *tmp = begin;
    while (tmp)
    {
        char *env = getenv(tmp->key);
        int b = 0;
        if (tmp->prev_value)
        {
            if (!env)
                hash_map_insert(var_hash, strdup(tmp->key),
                                strdup(tmp->prev_value), &b);
            else
                setenv(tmp->key, tmp->prev_value, 1);
        }
        else
        {
            if (!env)
                hash_map_remove(var_hash, tmp->key);
            else
                unsetenv(tmp->key);
        }
        tmp = tmp->next;
    }
}

static struct exit_status run_simple_cmd(struct ast_simple_cmd *ast,
                                         int *fd_backup, char **copy_words)
{
    struct exit_status status = { 0, EXIT_OK };

    // Empty words array, only prefix(s) case
    // Don't restore_var in this case ! Variables assignments are permanent here
    if (ast->words[0] == NULL)
    {
        restore_words(ast, copy_words);
        restore_fd(fd_backup);
        return status;
    }

    // Check for builtin
    built_run run = get_builtin(ast->words[0]);
    struct exit_status b;
    if (exec_func(ast->words[0], &b, ast))
    {
        status = b;
    }
    else if (run != NULL)
        status = run(ast->words);
    else // Not a builtin, execvp
    {
        // If not a builtin, fork and execvp the command
        pid_t pid = fork();
        if (pid == -1) // Error forking
        {
            fprintf(stderr, "error forking\n");
            status = (struct exit_status){ 127, EXIT_ERROR };
        }
        else if (pid == 0) // Child
        {
            // If execvp didn't stop the code, means error
            int error = execvp(ast->words[0], ast->words);
            fprintf(stderr, "command not found\n");
            status = (struct exit_status){ error, EXIT_ERROR };
        }
        else // Parent
        {
            // Wait for child process to finish and get its exit code
            int exit_code = get_exec_status(pid);
            status.exit_code = exit_code;
        }
    }

    restore_var(ast->var);
    restore_words(ast, copy_words);
    restore_fd(fd_backup);

    return status;
}

struct exit_status execution_simple_cmd(struct ast_node *ast)
{
    struct ast_simple_cmd *ast_simple_cmd = (struct ast_simple_cmd *)ast;

    // Backup the words array to restore it after expansion + execution
    char **copy_words = argv_copy(ast_simple_cmd->words);
    struct exit_status exit_status = { 0, EXIT_OK };

    // Variables assignment + expansion
    // can result in an error or a normal exit if a fork is done
    enum expansion_status status =
        var_and_expansion(ast_simple_cmd->var, &ast_simple_cmd->words);
    if (status == EXPANSION_ERROR)
    {
        free_argv(copy_words);
        fprintf(stderr, "bad substitution\n");
        return (struct exit_status){ 1, EXIT_ERROR };
    }
    if (status == EXPANSION_EXIT)
    {
        free_argv(copy_words);
        return (struct exit_status){ 0, EXIT_NORMALLY };
    }

    // Redirections
    // We make a backup array to store the original file descriptors value
    // before redirections
    int fd_backup[MAX_FD];
    memset(fd_backup, -1, sizeof(int) * MAX_FD);

    // Execute all redirections left to right
    redir_execute_all((struct ast_redir *)ast_simple_cmd->redir, fd_backup);

    exit_status = run_simple_cmd(ast_simple_cmd, fd_backup, copy_words);
    return exit_status;
}
