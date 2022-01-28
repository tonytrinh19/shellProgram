#include <dc_posix/dc_string.h>
#include "input.h"

char *read_command_line(const struct dc_posix_env *env, struct dc_error *err, FILE *stream, size_t *line_size) {
    char *line;
    line = dc_calloc(env, err, *line_size, sizeof(char));
    dc_getline(env, err, &line, line_size, stream);
    dc_str_trim(env, line);
    *line_size = dc_strlen(env, line);
    return line;
}
