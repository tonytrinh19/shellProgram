#include <string.h>
#include "shell_impl.h"
#include "input.h"

int init_state(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    const char *path_str;
    char **parsed_path;
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
    parsed_path = parse_path(env, err, path_str);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }
    state->path   = parsed_path;
    state->prompt = get_prompt(env, err);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    state->current_line = NULL;
    state->current_line_length = 0;
    state->command = NULL;
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

    state->max_line_length = 0;
    state->current_line_length = 0;

    index = 0;
    while (state->path[index])
    {
        free(state->path[index]);
        state->path[index] = NULL;
    }
    state->path = NULL;
    return DC_FSM_EXIT;
}

int reset_state(const struct dc_posix_env *env, struct dc_error *err,
                void *arg)
{
    struct state *state;
    state = (struct state*) arg;
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

    const char *cwd = dc_get_working_dir(env, err);
    if (dc_error_has_error(err))
    {
        state->fatal_error = true;
        return ERROR;
    }

    char *fullPrompt = malloc(1 + strlen(cwd) + 1 + 1 + strlen(state->prompt) + 1);
    sprintf(fullPrompt, "[%s] %s", cwd, state->prompt);
    size_t leng = strlen(fullPrompt);
    fullPrompt[leng] = '\0';
    fprintf(state->stdout, "%s", fullPrompt);

    str = read_command_line(env, err, state->stdin, &len);
    state->current_line = str;
    state->current_line_length = dc_strlen(env, str);
    if (dc_strcmp(env, str, "") == 0 || !str)
    {
        return RESET_STATE;
    }

    return SEPARATE_COMMANDS;
}
