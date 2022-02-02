#include <unistd.h>
#include <sys/wait.h>
#include "execute.h"

void execute(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        // TODO: NEED TO WORK ON THIS PART

    }
    else
    {
        // Main process
        int status;
        status = waitpid(pid, NULL, 0);
        command->exit_code = status;
    }
}

void redirect(struct command *command)
{

}

//int run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
//{
//
//    return -1;
//}

//int handle_run_error(const struct dc_posix_env *env, struct dc_error *err, struct command *command)
//{
//    return E2BIG;
//}
