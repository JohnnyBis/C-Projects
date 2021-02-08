#include <criterion/criterion.h>
#include <criterion/logging.h>

#include "const.h"

static char *progname = "bin/birp";

Test(basecode_tests_suite, validargs_help_test, .timeout=5) {
    char *argv[] = {progname, "-h", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int flag = 0x80000000;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt & flag, flag, "Correct bit (0x%x) not set for -h. Got: %x",
		 flag, opt);
}

Test(basecode_tests_suite, validargs_input_test, .timeout=5) {
    char *argv[] = {progname, "-i", "pgm", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = 0x21;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
}

Test(basecode_tests_suite, validargs_output_test, .timeout=5) {
    char *argv[] = {progname, "-o", "ascii", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = 0x32;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
}

Test(basecode_tests_suite, validargs_transform_test, .timeout=5) {
    char *argv[] = {progname, "-o", "birp", "-i", "birp", "-t", "128", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int ret = validargs(argc, argv);
    int exp_ret = 0;
    int opt = global_options;
    int exp_opt = 0x800222;
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
    cr_assert_eq(opt, exp_opt, "Invalid options settings.  Got: 0x%x | Expected: 0x%x",
		 opt, exp_opt);
}

Test(basecode_tests_suite, validargs_error_test, .timeout=5) {
    char *argv[] = {progname, "-i", "ascii", "-o", "birp", NULL};
    int argc = (sizeof(argv) / sizeof(char *)) - 1;
    int exp_ret = -1;
    int ret = validargs(argc, argv);
    cr_assert_eq(ret, exp_ret, "Invalid return for validargs.  Got: %d | Expected: %d",
		 ret, exp_ret);
}

Test(basecode_tests_suite, help_system_test, .timeout=5) {
    char *cmd = "bin/birp -h > /dev/null 2>&1";

    // system is a syscall defined in stdlib.h
    // it takes a shell command as a string and runs it
    // we use WEXITSTATUS to get the return code from the run
    // use 'man 3 system' to find out more
    int return_code = WEXITSTATUS(system(cmd));

    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
}

Test(basecode_tests_suite, birp_basic_test, .timeout=5) {
    char *cmd = "bin/birp -i pgm < rsrc/M.pgm > test_output/M.birp";
    char *cmp = "cmp test_output/M.birp rsrc/M.birp";

    int return_code = WEXITSTATUS(system(cmd));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program exited with %d instead of EXIT_SUCCESS",
		 return_code);
    return_code = WEXITSTATUS(system(cmp));
    cr_assert_eq(return_code, EXIT_SUCCESS,
                 "Program output did not match reference output.");
}
