#include <check.h>
#include <stdio.h>
#include "../emit.h"
#include "../compiler.h"

START_TEST (test_from_file)
{
  Program prog;
  setupCompiler(&prog);

  char * fileNames[] = {
    "examples/array.dt",
    "examples/assignables.dt",
    "examples/conds.dt",
    "examples/date_time.dt",
    "examples/fill.dt",
    "examples/fizzbuzz.dt",
    "examples/helloworld.dt",
    "examples/life.dt",
    "examples/math.dt",
    "examples/mouse.dt",
    "examples/print_digit.dt",
    "examples/pulser.dt",
    "examples/recursion.dt",
    "examples/sprite.dt",
    "examples/test.dt",
    "examples/type_determined.dt",
    "examples/vars.dt"
  };

  char * str = "test/outX.txt";

  for (int i = 0; i < 17; i++) {
    fromFile(&prog, fileNames[i]);
    str[8] = i + 48;
    toFile(&prog, str);

    parse(&prog);
    build_instruction_list(&prog, prog.root);
    ck_assert_int_eq(prog.errored, 0);

    resetCompiler(&prog);
  }
}
END_TEST

START_TEST (test_from_string)
{
  Program prog;
  setupCompiler(&prog);

  fromString(&prog, "fn main() do x :: i8 x = 1 + 1 end");

  parse(&prog);

  ck_assert_int_eq(prog.errored, 0);
}
END_TEST


Suite * lang_suite(void)
{
  Suite * s;
  TCase * test_Parser;

  s = suite_create("parser tests");

  test_Parser = tcase_create("parsing");
  tcase_set_timeout(test_Parser, 100);
  tcase_add_test(test_Parser, test_from_file);
  tcase_add_test(test_Parser, test_from_string);

  suite_add_tcase(s, test_Parser);
  return s;
}


int main(void)
{
  int number_failed;
  Suite *s;
  SRunner *sr;

  s = lang_suite();
  sr = srunner_create(s);

  srunner_run_all(sr, CK_NORMAL);
  number_failed = srunner_ntests_failed(sr);
  srunner_free(sr);
  return (number_failed == 0) ? 0 : 1;
}
