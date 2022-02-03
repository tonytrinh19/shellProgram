#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "execute.h"
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_stdlib.h>

void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command);
int run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path);
int handle_run_error(struct dc_error *err, struct command *command);

void execute(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        // Child process
        int status;
        int ret_val;
        redirect(env, err, command);
        if (dc_error_has_error(err))
        {
            exit(126);
        }
        // call run
        // TODO: IDK ?
        run(env, err, command, path);
        if (dc_error_has_error(err))
        {
            exit(err->err_code);
        }
        status = handle_run_error(err, command);
        exit(status);
    }
    else
    {
        // Main process
        int status;
        status = waitpid(pid, NULL, 0);
        command->exit_code = status;
    }
}

void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command)
{
    if (command->stdin_file)
    {
        int fd;
        fd = dc_open(env, err, command->stdin_file, DC_O_WRONLY, S_IRWXU);
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }
        dc_dup2(env, err, fd, STDIN_FILENO);
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }
    }
    if (command->stdout_file)
    {
        unsigned int option = DC_O_TRUNC;
        if (command->stdout_overwrite)
        {
            option = DC_O_APPEND;
        }
        int fd;
        fd = dc_open(env, err, command->stdout_file, DC_O_WRONLY | option, S_IRWXU);
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }
        dc_dup2(env, err, fd, STDOUT_FILENO);
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }
    }
    if (command->stderr_file)
    {
        unsigned int option = DC_O_TRUNC;
        if (command->stderr_overwrite)
        {
            option = DC_O_APPEND;
        }
        int fd;
        fd = dc_open(env, err, command->stderr_file, DC_O_WRONLY | option, S_IRWXU);
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }
        dc_dup2(env, err, fd, STDERR_FILENO);
        if (dc_error_has_error(err))
        {
            dc_close(env, err, fd);
            return;
        }
    }
}

int run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path)
{
    if (dc_strcmp(env, command->command, "/") == 0)
    {
        // Not freed
        command->argv[0] = dc_strdup(env, err, command->command);
        // TODO: something here
    }
    else
    {
        if (!path)
        {
            err->err_code = ENOENT;
        }
        else
        {
            size_t index = 0;
            while(path[index])
            {
                size_t len = dc_strlen(env, path[index]) + dc_strlen(env, command->command);
                char *cmd;
                char *temp = dc_malloc(env, err, len * sizeof(char));
                sprintf(temp, "%s/%s", path[index], command->command);
                cmd = dc_strdup(env, err, temp);
                command->argv[0] = dc_strdup(env, err, cmd);
                // TODO: something here as well
                // call execv for the command
                // If the error from execv is not ENOENT
                //                Exit the loop
                index++;
                free(cmd);
                free(temp);
            }
        }
    }
}

int handle_run_error(struct dc_error *err, struct command *command)
{
    if (dc_error_is_errno(err, E2BIG))
    {
        return 1;
    }
    if (dc_error_is_errno(err, EACCES))
    {
        return 2;
    }
    if (dc_error_is_errno(err, EINVAL))
    {
        return 3;
    }
    if (dc_error_is_errno(err, ELOOP))
    {
        return 4;
    }
    if (dc_error_is_errno(err, ENAMETOOLONG))
    {
        return 5;
    }
    if (dc_error_is_errno(err, ENOENT))
    {
        return 127;
    }
    if (dc_error_is_errno(err, ENOTDIR))
    {
        return 6;
    }
    if (dc_error_is_errno(err, ENOEXEC))
    {
        return 7;
    }
    if (dc_error_is_errno(err, ENOMEM))
    {
        return 8;
    }
    if (dc_error_is_errno(err, ETXTBSY))
    {
        return 9;
    }
    else
    {
        return 125;
    }
}
