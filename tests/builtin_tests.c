#include "tests.h"
#include "util.h"

Describe(builtin);

BeforeEach(builtin)
{}

AfterEach(builtin)
{}

Ensure(builtin, builtin_cd)
{
}

TestSuite *builtin_tests(void)
{
    TestSuite *suite;

    suite = create_test_suite();
    add_test_with_context(suite, builtin, builtin_cd);

    return suite;
}
