#include <stdlib.h>
#include <string.h>
#include <dc_util/path.h>
#include <dc_util/strings.h>
#include <dc_posix/dc_string.h>
#include <ctype.h>
#include <dc_posix/dc_stdlib.h>
#include <wordexp.h>
#include "command.h"

void parse_command(const struct dc_posix_env *env, struct dc_error *err,
                   struct state *state, struct command *command)
{
    wordexp_t exp_main;
    int status_main;
    char *string;
    regmatch_t match;
    int matched;
    const char *append = ">>";
    char **argv;
    size_t sizeArray;
    size_t index;
    char* token;
    char* rest;
    char* temp;
    char *expanded_stdin_file;
    char *expanded_stdout_file;
    char *expanded_stderr_file;

    string  = strdup(command->line);
    matched = regexec(state->err_redirect_regex,
                      string,
                      1,
                      &match,
                      0);

    if (matched == 0)
    {
        char *str2;
        char *str;
        wordexp_t exp;
        int status;
        // Offset of the first character of the stderr file name from the start. " 2>"
        size_t offset = 3;

        regoff_t length = match.rm_eo - match.rm_so;

        str = malloc(length + 1);
        strncpy(str, &string[match.rm_so], length);
        string[match.rm_so] = '\0';
        str[length] = '\0';

        if (strstr(str, append))
        {
            offset++;
            command->stderr_overwrite = true;
        }

        size_t size = length - offset;
        str2 = dc_malloc(env, err, sizeof(char) * size);
        strncpy(str2, &str[offset], size);
        str2[size] = '\0';
        // wordexp
        status = wordexp(str2, &exp, 0);
        if (status == 0)
        {
            command->stderr_file = strdup(exp.we_wordv[0]);
            wordfree(&exp);
        }
        else
        {
            state->fatal_error = true;
        }
        free(str2);
        free(str);
    }

    matched = regexec(state->out_redirect_regex,
                      string,
                      1,
                      &match,
                      0);

    if (matched == 0)
    {
        char *str2;
        char *str;
        wordexp_t exp;
        int status;
        // Offset of the first character of the stdout file name from the start. " >"
        size_t offset = 2;

        regoff_t length = match.rm_eo - match.rm_so;

        str = malloc(length + 1);
        strncpy(str, &string[match.rm_so], length);
        string[match.rm_so] = '\0';
        str[length] = '\0';

        if (strstr(str, append))
        {
            offset++;
            command->stdout_overwrite = true;
        }

        size_t size = length - offset;
        str2 = dc_malloc(env, err, sizeof(char) * size);
        strncpy(str2, &str[offset], size);
        str2[size] = '\0';

        // wordexp
        status = wordexp(str2, &exp, 0);
        if (status == 0)
        {
            command->stdout_file = strdup(exp.we_wordv[0]);
            wordfree(&exp);
        }
        else
        {
            state->fatal_error = true;
        }
        free(str2);
        free(str);
    }

    matched = regexec(state->in_redirect_regex,
                      string,
                      1,
                      &match,
                      0);

    if (matched == 0)
    {
        char *str2;
        char *str;
        wordexp_t exp;
        int status;
        // Offset of the first character of the stdin file name from the start. " <"
        size_t offset = 2;

        regoff_t length = match.rm_eo - match.rm_so;

        str = malloc(length + 1);
        strncpy(str, &string[match.rm_so], length);
        string[match.rm_so] = '\0';
        str[length] = '\0';

        size_t size = length - offset;
        str2 = dc_malloc(env, err, sizeof(char) * size);
        strncpy(str2, &str[offset], size);
        str2[size] = '\0';
        // wordexp
        status = wordexp(str2, &exp, 0);
        if (status == 0)
        {
            command->stdin_file = strdup(exp.we_wordv[0]);
            wordfree(&exp);
        }
        else
        {
            state->fatal_error = true;
        }
        free(str2);
        free(str);
    }

    status_main = wordexp(string, &exp_main, 0);
    if (status_main == 0)
    {
        command->argc = exp_main.we_wordc;
        command->argv = dc_calloc(env, err, (exp_main.we_wordc + 2), sizeof(char *));

        for (size_t i = 1; i < exp_main.we_wordc; ++i)
        {
            command->argv[i] = strdup(exp_main.we_wordv[i]);
        }
        command->command = strdup(exp_main.we_wordv[0]);
        wordfree(&exp_main);
    }
    else
    {
        state->fatal_error = true;
    }

    free(string);
}

void destroy_command(const struct dc_posix_env *env, struct command *command)
{
    free(command->line);
    command->line = NULL;

    free(command->command);
    command->command = NULL;

    for (size_t i = 0; i < command->argc; ++i)
    {
        free(command->argv[i]);
        command->argv[i] = NULL;
    }

    free(command->argv);
    command->argv = NULL;

    command->argc = 0;
    free(command->stdin_file);
    command->stdin_file = NULL;

    free(command->stdout_file);
    command->stdout_file = NULL;

    command->stdout_overwrite = false;

    free(command->stderr_file);
    command->stderr_file = NULL;

    command->stderr_overwrite = false;
}
