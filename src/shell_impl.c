#include "shell_impl.h"

int init_state(const struct dc_posix_env *env, struct dc_error *err, void *arg)
{
    struct state *state;
    const char *path_str;
    char **parsed_path;
    int status;
    regex_t *in_regex;
    regex_t *out_regex;
    regex_t *err_regex;

    state = (struct state*) arg;
    state->fatal_error = false;
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
    state->path = parsed_path;

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

//int destroy_state(const struct dc_posix_env *env, struct dc_error *err,
//                  void *arg)
//{
//    return 1;
//}
//
//int reset_state(const struct dc_posix_env *env, struct dc_error *err,
//                void *arg)
//{
//    return 1;
//}
//
//int read_commands(const struct dc_posix_env *env, struct dc_error *err,
//                  void *arg)
//{
//    return 1;
//}
