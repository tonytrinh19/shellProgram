#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include "execute.h"
#include <dc_posix/dc_fcntl.h>
#include <dc_posix/dc_unistd.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_stdlib.h>

/**
 * Setup any I/O redirections for the process.
 * @param env the posix environment.
 * @param err the err object.
 * @param command the command to execute
 */
void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command);

/**
 * Run the process
 * @param env the posix environment.
 * @param err the err object.
 * @param command the command to execute
 * @param path array of PATH directories to search for the program
 * @return Only returns if all of the calls to execv fail
 */
int run(const struct dc_posix_env *env, struct dc_error *err, struct command *command, char **path);

/**
 * Handles error for run function (execv).
 * @param err the err object.
 * @param command the command to execute
 * @return the error code for each error
 */
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
            exit(err->err_code);
        }
        run(env, err, command, path);
        status = handle_run_error(err, command);
        if (status == 127)
        {
            fprintf(stderr, "command: %s not found\n", command->command);
        }
        exit(status);
    }
    else
    {
        // Main process
        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status))
        {
            int es = WEXITSTATUS(status);
            command->exit_code = es;
        }
    }
}

void redirect(const struct dc_posix_env *env, struct dc_error *err, struct command *command)
{
    if (command->stdin_file)
    {
        int fd;
        fd = dc_open(env, err, command->stdin_file, DC_O_RDONLY, 0);
        if (dc_error_has_no_error(err))
        {
            dc_dup2(env, err, fd, STDIN_FILENO);
        }
        dc_close(env, err, fd);
    }
    if (command->stdout_file)
    {
        unsigned int option = DC_O_TRUNC;
        int fd;
        if (command->stdout_overwrite)
        {
            option = DC_O_APPEND;
        }
        fd = dc_open(env, err, command->stdout_file, DC_O_CREAT | DC_O_WRONLY | option, S_IRWXU);
        if (dc_error_has_no_error(err))
        {
            dc_dup2(env, err, fd, STDOUT_FILENO);
        }
        dc_close(env, err, fd);
    }
    if (command->stderr_file)
    {
        unsigned int option = DC_O_TRUNC;
        int fd;
        if (command->stderr_overwrite)
        {
            option = DC_O_APPEND;
        }
        fd = dc_open(env, err, command->stderr_file, DC_O_CREAT | DC_O_WRONLY | option, S_IRWXU);
        if (dc_error_has_no_error(err))
        {
            dc_dup2(env, err, fd, STDERR_FILENO);
        }
        dc_close(env, err, fd);
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
        if (!path[0])
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
                if (dc_error_has_error(err))
                {
                    if (!dc_error_is_errno(err, ENOENT))
                    {
                        break;
                    }
                }
                index++;
                free(cmd);
                free(temp);
            }
        }
    }
    return EXIT_FAILURE;
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
