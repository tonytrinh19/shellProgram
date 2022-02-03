#include <builtins.h>
#include <dc_posix/dc_unistd.h>
#include <dc_util/path.h>
#include <dc_posix/dc_string.h>
#include <stdlib.h>

void builtin_cd(const struct dc_posix_env *env, struct dc_error *err,
                struct command *command, FILE *errstream)
{
    char *path;
    if (command->argv[1] == NULL)
    {
        char *expanded_path;
        dc_expand_path(env, err, &expanded_path,"~/");
        path = dc_strdup(env, err, expanded_path);
        dc_chdir(env, err, expanded_path);
    }
    else
    {
        path = dc_strdup(env, err, command->argv[1]);
        dc_chdir(env, err, command->argv[1]);
    }
    // No errors exit code is 0.
    command->exit_code = 0;
    if (dc_error_has_error(err))
    {
        // Error -> exit code is 1.
        command->exit_code = 1;
        if (dc_error_is_errno(err, EACCES))
        {
            fprintf(errstream,"%s: permission denied\n", path);
        }
        else if (dc_error_is_errno(err, ELOOP))
        {
            fprintf(errstream,"%s: loop exists in symbolic links\n", path);
        }
        else if (dc_error_is_errno(err, ENAMETOOLONG))
        {
            fprintf(errstream,"%s: file name too long\n", path);
        }
        else if (dc_error_is_errno(err, ENOENT))
        {
            fprintf(errstream,"%s: does not exist\n", path);
        }
        else if (dc_error_is_errno(err, ENOTDIR))
        {
            fprintf(errstream,"%s: is not a directory\n", path);
        }
        else
        {
            fprintf(errstream,"Fatal error\n");
        }
    }
    free(path);
}
