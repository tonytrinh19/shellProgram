#include "tests.h"
#include "util.h"
#include "command.h"
#include "state.h"
#include <dc_posix/dc_stdlib.h>
#include <dc_posix/dc_string.h>

/**
 * Checks whether the state has been reset.
 * @param err the error.
 * @param state the state to display.
 */
static void check_state_reset(const struct dc_error *err, const struct state *state);

Describe(util);

static struct dc_posix_env env;
static struct dc_error err;

BeforeEach(util)
{
    dc_posix_env_init(&env, NULL);
    dc_error_init(&err, NULL);
}

AfterEach(util)
{
}

Ensure(util, get_prompt)
{
    char *prompt;

    prompt = dc_getenv(&env, "PS1");

    if(prompt != NULL)
    {
        dc_setenv(&env, &err, "PS1", NULL, true);
    }

    prompt = get_prompt(&env, &err);
    assert_that(prompt, is_equal_to_string("$ "));

    dc_setenv(&env, &err, "PS1", "ABC", true);
    prompt = get_prompt(&env, &err);
    assert_that(prompt, is_equal_to_string("ABC"));
}

Ensure(util, get_path)
{
    static const char *paths[] =
            {
                "",
                ".",
                "abc",
                "abc:def",
                "/usr/bin:.",
                ".:/usr/bin",
                ":",
                "/usr/bin:/bin:/usr/local/bin",
                NULL,
            };
    char *path;

    dc_unsetenv(&env, &err, "PATH");
    path = get_path(&env, &err);
    assert_that(path, is_null);

    for(int i = 0; paths[i]; i++)
    {
        dc_setenv(&env, &err, "PATH", paths[i], true);
        path = get_path(&env, &err);
        assert_that(path, is_equal_to_string(paths[i]));
        assert_that(path, is_not_equal_to(paths[i]));
    }
}

Ensure(util, parse_path)
{

}

Ensure(util, state_to_string)
{
    struct state state;
    char *str;

    state.in_redirect_regex = NULL;
    state.out_redirect_regex = NULL;
    state.err_redirect_regex = NULL;
    state.path = NULL;
    state.prompt = NULL;
    state.max_line_length = 0;
    state.current_line = NULL;
    state.current_line_length = 0;
    state.command = NULL;

    state.fatal_error = false;
    str = state_to_string(&env, &err, &state);
    assert_that(str, is_equal_to_string("current_line = NULL, fatal_error = 0"));
    free(str);

    state.fatal_error = true;
    str = state_to_string(&env, &err, &state);
    assert_that(str, is_equal_to_string("current_line = NULL, fatal_error = 1"));
    free(str);

    state.current_line = "";
    state.fatal_error = false;
    str = state_to_string(&env, &err, &state);
    assert_that(str, is_equal_to_string("current_line = \"\", fatal_error = 0"));
    free(str);

    state.current_line = "hello";
    state.fatal_error = false;
    str = state_to_string(&env, &err, &state);
    assert_that(str, is_equal_to_string("current_line = \"hello\", fatal_error = 0"));
    free(str);

    state.current_line = "world";
    state.fatal_error = true;
    str = state_to_string(&env, &err, &state);
    assert_that(str, is_equal_to_string("current_line = \"world\", fatal_error = 1"));
    free(str);
}

Ensure(util, do_reset_state)
{
    struct state state;
    state.in_redirect_regex = NULL;
    state.out_redirect_regex = NULL;
    state.err_redirect_regex = NULL;
    state.path = NULL;
    state.prompt = NULL;
    state.max_line_length = 0;

    state.current_line = NULL;
    state.current_line_length = 0;
    state.command = NULL;
    state.fatal_error = false;

    do_reset_state(&env, &err, &state);
    check_state_reset(&err, &state);

    state.current_line = dc_strdup(&env, &err, "ls");
    state.current_line_length = dc_strlen(&env, state.current_line);
    do_reset_state(&env, &err, &state);
    check_state_reset(&err, &state);

    state.current_line = dc_strdup(&env, &err, "ls");
    state.current_line_length = dc_strlen(&env, state.current_line);
    do_reset_state(&env, &err, &state);
    check_state_reset(&err, &state);

    state.current_line = dc_strdup(&env, &err, "ls");
    state.current_line_length = dc_strlen(&env, state.current_line);
    // When resetting state.command, has to deallocate every single items inside of struct command.
    state.command = dc_calloc(&env, &err, 1, sizeof(struct command));
    do_reset_state(&env, &err, &state);
    check_state_reset(&err, &state);

    DC_ERROR_RAISE_ERRNO(&err, E2BIG);
    do_reset_state(&env, &err, &state);
    check_state_reset(&err, &state);
}

static void check_state_reset(const struct dc_error *err, const struct state *state)
{
    assert_that(state->current_line, is_null);
    assert_that(state->current_line_length, is_equal_to(0));
    assert_that(state->command, is_null);
    assert_that(err->message, is_null);
    assert_that(err->file_name, is_null);
    assert_that(err->function_name, is_null);
    assert_that(err->line_number, is_equal_to(0));
    assert_that(err->type, is_equal_to(0));
    assert_that(err->reporter, is_null);
    assert_that(err->err_code, is_equal_to(0));
}

TestSuite *util_tests(void)
{
    TestSuite *suite;

    suite = create_test_suite();
    add_test_with_context(suite, util, get_prompt);
    add_test_with_context(suite, util, get_path);
    add_test_with_context(suite, util, parse_path);
    add_test_with_context(suite, util, state_to_string);
    add_test_with_context(suite, util, do_reset_state);

    return suite;
}
