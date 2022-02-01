#include <builtins.h>
#include <unistd.h>
#include <dc_util/path.h>

void builtin_cd(const struct dc_posix_env *env, struct dc_error *err,
                struct command *command, FILE *errstream)
{
    int fd;
    if (command->argv[1] == NULL)
    {
        char *expanded_path;
        dc_expand_path(env, err, &expanded_path,"~/");
        fd = chdir(expanded_path);
    }
    else
    {
        fd = chdir(command->argv[1]);
    }

    // Error checking
    if (fd == -1)
    {
        command->exit_code = 1;
    }
    // TODO: Need to work on error handling for cd.
    command->exit_code = 0;
}
