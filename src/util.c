#include "util.h"

const char *get_prompt(const struct dc_posix_env *env, struct dc_error *err)
{
    char *prompt;
    prompt = dc_getenv(env, "PS1");

    if (!prompt)
    {
        return "$ ";
    }
    return prompt;
}

char *get_path(const struct dc_posix_env *env, struct dc_error *err)
{
    return dc_getenv(env, "PATH");
}
