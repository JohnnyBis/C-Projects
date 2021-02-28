#include "test_common.h"

#define TEST_REF_DIR "tests/rsrc"
#define TEST_OUTPUT_DIR "tests.out"

/*
 * Tests the basic program operation.
 */
Test(base_suite, quick_test) {
    char *name = "quick";
    sprintf(program_options, "rsrc/algebric.ntn");
    int err = run_using_system(name, "", "");
    assert_normal_exit(err);
    assert_outfile_matches(name, NULL);
    assert_errfile_matches(name, NULL);
}

/*
 * Tests "garbage input".  Should exit with error status, but not crash.
 */
Test(base_suite, garbage_input) {
    char *name = "garbage_input";
    sprintf(program_options, "src/main.c");
    int err = run_using_system(name, "", "valgrind");
    assert_expected_status(1, err);
}

/*
 * This test runs valgrind to check for the use of uninitialized variables.
 */
Test(base_suite, valgrind_uninitialized) {
    char *name = "valgrind_uninitialized";
    sprintf(program_options, "rsrc/algebric.ntn");
    int err = run_using_system(name, "", "valgrind --leak-check=no --undef-value-errors=yes --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
    assert_outfile_matches(name, NULL);
}

/*
 * This test runs valgrind to check for memory leaks.
 */
Test(base_suite, valgrind_leak_test) {
    char *name = "valgrind_leak";
    sprintf(program_options, "rsrc/algebric.ntn");
    int err = run_using_system(name, "", "valgrind --leak-check=full --undef-value-errors=no --error-exitcode=37");
    assert_no_valgrind_errors(err);
    assert_normal_exit(err);
    assert_outfile_matches(name, NULL);
}
