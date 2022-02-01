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
    wordexp_t exp;
    int status;
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
    status = wordexp(string, &exp, 0);
    if (status == 0)
    {
        command->argc = exp.we_wordc;
        command->argv = dc_calloc(env, err, (exp.we_wordc + 2), sizeof(char *));

        for (size_t i = 1; i < exp.we_wordc; ++i)
        {
            command->argv[i] = strdup(exp.we_wordv[i]);
        }
        command->command = strdup(exp.we_wordv[0]);
        wordfree(&exp);
    }
    else
    {
        state->fatal_error = true;
    }

    free(string);
}

void destroy_command(const struct dc_posix_env *env, struct command *command)
{

}


//    sizeArray = 1;
//    index     = 1;
//    rest = strdup(string);
//    temp = rest;
//
//    // Count number of elements by counting spaces
//    for (size_t i = 0; i < dc_strlen(env, string); ++i)
//    {
//        if (isspace(string[i])) sizeArray++;
//    }
//
//    // +1 for NULL element at the end
//    argv = dc_calloc(env, err, sizeArray + 1, sizeof(char *));
//
//    if (dc_error_has_error(err))
//    {
//        state->fatal_error = true;
//    }
//
//    token = strtok_r(rest, " ", &rest);
//    command->command = strdup(token);
//
//    while((token = strtok_r(rest, " ", &rest)))
//    {
//        argv[index] = strdup(token);
//        index++;
//    }
