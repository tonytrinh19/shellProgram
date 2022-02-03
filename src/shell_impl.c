#include <string.h>
#include <unistd.h>
#include <dc_posix/dc_string.h>
#include <stdlib.h>
#include <dc_util/filesystem.h>
#include "shell_impl.h"
#include "input.h"
#include "util.h"
#include "builtins.h"

int init_state(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    char *path_str;
    int status;
    regex_t *in_regex;
    regex_t *out_regex;
    regex_t *err_regex;

    state                  = (struct state*) arg;
    state->fatal_error     = false;
    state->max_line_length = (size_t) sysconf(_SC_ARG_MAX);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    in_regex  = dc_calloc(env, err, 1, sizeof(regex_t));
    out_regex = dc_calloc(env, err, 1, sizeof(regex_t));
    err_regex = dc_calloc(env, err, 1, sizeof(regex_t));

    status = regcomp(in_regex, "[ \\t\\f\\v]<.*", REG_EXTENDED);
    if (status != 0)
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->in_redirect_regex = in_regex;
    status = regcomp(out_regex, "[ \\t\\f\\v][1^2]?>[>]?.*", REG_EXTENDED);
    if (status != 0)
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->out_redirect_regex = out_regex;

    status = regcomp(err_regex, "[ \\t\\f\\v]2>[>]?.*", REG_EXTENDED);
    if (status != 0)
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->err_redirect_regex = err_regex;

    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    path_str = get_path(env, err);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->path  = parse_path(env, err, path_str);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    state->prompt = get_prompt(env, err);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    state->current_line = NULL;
    state->current_line_length = 0;
    state->command = NULL;
    free(path_str);

    return READ_COMMANDS;
}

int destroy_state(const struct dc_posix_env *env, struct dc_error *err,
                  void *arg)
{
    struct state *state;
    size_t index;

    state = (struct state*) arg;

    state->fatal_error = false;

    regfree(state->in_redirect_regex);
    free(state->in_redirect_regex);
    state->in_redirect_regex = NULL;

    regfree(state->out_redirect_regex);
    free(state->out_redirect_regex);
    state->out_redirect_regex = NULL;

    regfree(state->err_redirect_regex);
    free(state->err_redirect_regex);
    state->err_redirect_regex = NULL;

    free(state->prompt);
    state->prompt = NULL;

    free(state->current_line);
    state->current_line = NULL;

    state->max_line_length = 0;
    state->current_line_length = 0;

    index = 0;
    while (state->path[index])
    {
        free(state->path[index]);
        state->path[index] = NULL;
    }
    free(state->path[index]);
    free(state->path);
    state->path = NULL;
    free(err->message);
    if (state->command)
    {
        destroy_command(env, state->command);
        free(state->command);
        state->command = NULL;
    }
    return DC_FSM_EXIT;
}

int reset_state(const struct dc_posix_env *env, struct dc_error *err,
                void *arg)
{
    struct state *state;
    state = (struct state *) arg;
    do_reset_state(env, err, state);
    return READ_COMMANDS;
}

int read_commands(const struct dc_posix_env *env, struct dc_error *err,
                  void *arg)
{
    struct state *state;
    state = (struct state*) arg;
    size_t len;
    char *str;

    char *cwd = dc_get_working_dir(env, err);

    fprintf(state->stdout, "[%s] %s", cwd, state->prompt);

    str = read_command_line(env, err, state->stdin, &len);
    if(dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->current_line = strdup(str);
    state->current_line_length = dc_strlen(env, str);

    free(cwd);
    free(str);
    if (len == 0)
    {
        return RESET_STATE;
    }
    return SEPARATE_COMMANDS;
}

int separate_commands(const struct dc_posix_env *env, struct dc_error *err,
                      void *arg)
{
    struct state *state;

    state = (struct state *) arg;

    state->fatal_error = false;

    state->command = dc_calloc(env, err, 1, sizeof(struct command));

    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->command->line = strdup(state->current_line);
    state->command->command = NULL;
    state->command->argc = 0;
    state->command->argv = NULL;
    state->command->stdin_file = NULL;
    state->command->stdout_file = NULL;
    state->command->stderr_file = NULL;
    state->command->stderr_overwrite = false;
    state->command->stdout_overwrite = false;
    state->command->exit_code = 0;
    state->command->stderr_file = NULL;

    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    return PARSE_COMMANDS;
}

int parse_commands(const struct dc_posix_env *env, struct dc_error *err,
                   void *arg)
{
    struct state *state;
    state = (struct state *) arg;

    parse_command(env, err, state, state->command);

    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }

    if (state->fatal_error == true)
    {
        return ERROR;
    }
    return EXECUTE_COMMANDS;
}

int execute_commands(const struct dc_posix_env *env, struct dc_error *err,
                     void *arg)
{
    struct state *state;
    state = (struct state *)arg;
    if (dc_strcmp(env, state->command->command, "cd") == 0)
    {
        builtin_cd(env, err, state->command, state->stderr);
    }
    else if(dc_strcmp(env, state->command->command, "exit") == 0)
    {
        return EXIT;
    }
    else
    {
        execute(env, err, state->command, state->path);
    }

    fprintf(state->stdout, "%d\n", state->command->exit_code);

    if (state->fatal_error)
    {
        return ERROR;
    }
    return RESET_STATE;
}

int do_exit(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    state = (struct state*) arg;
    do_reset_state(env, err, state);
    return DESTROY_STATE;
}


int handle_error(const struct dc_posix_env *env, struct dc_error *err,
                 void *arg)
{
    struct state *state;
    state = (struct state *) arg;

    if (state->current_line == NULL)
    {
        fprintf(state->stderr,"internal error (%d) %s\n", err->err_code, err->message);
    }
    else
    {
        fprintf(state->stderr, "internal error (%d) %s: \"%s\"\n", err->err_code, err->message, state->current_line);
    }

    if (state->fatal_error)
    {
        return DESTROY_STATE;
    }
    return RESET_STATE;
}
