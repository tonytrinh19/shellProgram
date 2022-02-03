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
        redirect(env, err, command);
        if (dc_error_has_error(err))
        {
            exit(126);
        }
        run(env, err, command, path);
        status = handle_run_error(err, command);
        exit(status);
    }
    else
    {
        // Main process
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status))
        {
            const int es = WEXITSTATUS(status);
            command->exit_code = es;
        }
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
    if (dc_strchr(env, command->command, '/'))
    {
        command->argv[0] = dc_strdup(env, err, command->command);
        dc_execv(env, err, command->argv[0], command->argv);
    }
    else
    {
        if (!path)
        {
            DC_ERROR_RAISE_ERRNO(err, ENOENT);
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
                dc_execv(env, err, cmd, command->argv);
                if (!dc_error_is_errno(err, ENOENT))
                {
                    break;
                }
                index++;
                free(cmd);
                free(temp);
            }
        }
    }
    return 0;
}

int handle_run_error(struct dc_error *err, struct command *command)
{
    switch (err->err_code)
    {
        case E2BIG:
            return 1;
        case EACCES:
            return 2;
        case EINVAL:
            return 3;
        case ELOOP:
            return 4;
        case ENAMETOOLONG:
            return 5;
        case ENOENT:
            return 127;
        case ENOTDIR:
            return 6;
        case ENOEXEC:
            return 7;
        case ENOMEM:
            return 8;
        case ETXTBSY:
            return 9;
        default:
            return 125;
    }
}
