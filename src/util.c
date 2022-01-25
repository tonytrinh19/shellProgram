#include "util.h"

char *get_prompt(const struct dc_posix_env *env, struct dc_error *err)
{
    char *dollarSign = strdup("$ ");
    char *prompt;
    prompt = dc_getenv(env, "PS1");

    if (!prompt)
    {
        return dollarSign;
    }
    return prompt;
}

char *get_path(const struct dc_posix_env *env, struct dc_error *err)
{
    return dc_getenv(env, "PATH");
}

void do_reset_state(const struct dc_posix_env *env, struct dc_error *err, struct state *state)
{

}

void display_state(const struct dc_posix_env *env, const struct state *state, FILE *stream)
{
    char *str;
    struct dc_error err;
    str = state_to_string(env, &err, state);
    fprintf(stream, "%s\n", str);
    free(str);
}

char *state_to_string(const struct dc_posix_env *env,  struct dc_error *err, const struct state *state)
{
    size_t len;
    char *line;

    if(state->current_line == NULL)
    {
        len = strlen("current_line = NULL");
    }
    else
    {
        len = strlen("current_line = \"\"");
        len += state->current_line_length;
    }

    len += strlen(", fatal_error = ");
    // +1 for 0 or 1 for the fatal_error and +1 for the null byte
    line = malloc(len + 1 + 1);

    if(state->current_line == NULL)
    {
        sprintf(line, "current_line = NULL, fatal_error = %d", state->fatal_error);
    }
    else
    {
        sprintf(line, "current_line = \"%s\", fatal_error = %d", state->current_line, state->fatal_error);
    }

    return line;
}
