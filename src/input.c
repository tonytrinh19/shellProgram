#include <dc_util/strings.h>
#include <dc_posix/dc_string.h>
#include <dc_posix/dc_stdio.h>
#include "input.h"

char *read_command_line(const struct dc_posix_env *env, struct dc_error *err, FILE *stream, size_t *line_size) {
    char *line = NULL;
    dc_getline(env, err, &line, line_size, stream);
    line = dc_strdup(env, err, dc_str_trim(env, line));
    *line_size = dc_strlen(env, line);
    return line;
}
