#!/usr/bin/env python3

# Copyright (c) 2014, Ruslan Baratov
# All rights reserved.

import argparse
import sugar.sugar_warnings_cmake_flags_generator
import sugar.sugar_warnings_leathers_generator
import sugar.sugar_warnings_wiki_table_generator
import sys

parser = argparse.ArgumentParser(
    description="This script will generate next files using\n"
    "internal table `main_warnings_table` as source:\n\n"
    "  1. Wiki table for `leathers` C++ project\n"
    "  2. Suppression include files for `leathers` C++ project\n"
    "  3. CMake functions for `sugar`:\n"
    "    - sugar_generate_warning_flag_by_name\n"
    "    - sugar_generate_warning_xcode_attr_by_name\n"
    "    - sugar_get_all_xcode_warning_attrs\n\n"
    "Links:\n\n"
    "  https://github.com/ruslo/leathers\n"
    "  https://github.com/ruslo/sugar\n",
    formatter_class=argparse.RawTextHelpFormatter
)

args = parser.parse_args()

"""This table entry describe compiler support of warning"""
class CompilerEntry:
  def __init__(self, option):
    self.option = option

  def wiki_entry(self, warning_name):
    if self.option == "":
      return "*no*"
    if self.option == warning_name:
      return "**same**"
    return self.option

  def cxx_entry(self, name):
    assert(self.valid())
    return self.option

  def valid(self):
    return (self.option != "")

  def bigger(self, other):
    if self.option == other.option:
      return True
    if not other.valid():
      return True
    return False

"""This table entry contains warning name
and support status for different compilers"""
class TableEntry:
  def __init__(self, warning_name):
    self.warning_name = warning_name
    self.clang = CompilerEntry("")
    self.gcc = CompilerEntry("")
    self.msvc = CompilerEntry("")
    self.xcode = CompilerEntry("")
    self.objc = False
    self.group = ""

  def c(self, entry):
    self.clang = CompilerEntry(entry)
    return self

  def g(self, entry):
    self.gcc = CompilerEntry(entry)
    return self

  def c_same(self):
    self.clang = CompilerEntry(self.warning_name)
    return self

  def g_same(self):
    self.gcc = CompilerEntry(self.warning_name)
    return self

  def m(self, option):
    self.msvc = CompilerEntry(option)
    return self

  def xc(self, option):
    self.xcode = CompilerEntry("CLANG_WARN_{}".format(option))
    return self

  def xg(self, option):
    self.xcode = CompilerEntry("GCC_WARN_{}".format(option))
    return self

  def o(self):
    self.objc = True
    return self

  def grp(self, group_name):
    self.group = group_name
    return self

  def bigger(self, other):
    if (self.warning_name == other.warning_name):
      return False
    if not self.clang.bigger(other.clang):
      return False
    if not self.gcc.bigger(other.gcc):
      return False
    if not self.msvc.bigger(other.msvc):
      return False
    if not self.xcode.bigger(other.xcode):
      return False
    if self.objc != other.objc:
      return False
    return True

def E(name):
  return TableEntry(name)

# E: Entry
# c: clang
# g: gcc
# m: msvc
# xc: xcode clang
# xg: xcode gcc
# o: objective-c

main_warnings_table = [
    # compatibility-c++98
    E("c++98-compat").c_same().grp("compatibility-c++98"),
    E("c++98-compat-pedantic").c_same().grp("compatibility-c++98"),

    # special-members
    E("assign-base-inaccessible").m("4626").grp("special-members"),
    E("assign-could-not-be-generated").m("4512").grp("special-members"),
    E("copy-ctor-could-not-be-generated").m("4625").grp("special-members"),
    E("dflt-ctor-base-inaccessible").m("4623").grp("special-members"),
    E("dflt-ctor-could-not-be-generated").m("4510").grp("special-members"),
    E("user-ctor-required").m("4610").grp("special-members"),

    # inline
    E("automatic-inline").m("4711").grp("inline"),
    E("force-not-inlined").m("4714").grp("inline"),
    E("not-inlined").m("4710").grp("inline"),
    E("unreferenced-inline").m("4514").grp("inline"),

    #
    E("behavior-change").m("4350"),
    E("bool-conversion").c_same(),
    E("c++11-extensions").c_same(),
    E("cast-align").c_same().g_same(),
    E("catch-semantic-changed").m("4571"),
    E("conditional-uninitialized").c_same(),
    E("constant-conditional").m("4127"),
    E("constant-conversion").c_same(),
    E("conversion").c_same().g_same().m("4244"),
    E("conversion-loss").c("conversion").g("conversion").m("4242"),
    E("conversion-sign-extended").m("4826"),
    E("covered-switch-default").c_same(),
    E("deprecated").c_same().g_same(),
    E("deprecated-declarations").c_same().g_same().m("4996"),
    E("deprecated-objc-isa-usage").c_same(),
    E("deprecated-register").c_same(),
    E("digraphs-not-supported").m("4628"),
    E("disabled-macro-expansion").c_same(),
    E("documentation").c_same(),
    E("documentation-unknown-command").c_same(),
    E("empty-body").c_same().g_same(),
    E("enum-conversion").c_same(),
    E("exit-time-destructors").c_same(),
    E("extra-semi").c_same(),
    E("format").c_same().g_same(),
    E("four-char-constants").c_same(),
    E("global-constructors").c_same(),
    E("ill-formed-comma-expr").c("unused-value").g("unused-value").m("4548"),
    E("implicit-fallthrough").c_same(),
    E("inherits-via-dominance").m("4250"),
    E("int-conversion").c_same(),
    E("invalid-offsetof").c_same().g_same(),
    E("is-defined-to-be").m("4574"),
    E("layout-changed").m("4371"),
    E("missing-braces").c_same().g_same(),
    E("missing-field-initializers").c_same().g_same(),
    E("missing-noreturn").c_same().g_same(),
    E("missing-prototypes").c_same().g_same(),
    E("name-length-exceeded").m("4503"),
    E("newline-eof").c_same(),
    E("no-such-warning").m("4619"),
    E("non-virtual-dtor").c_same().g_same().m("4265"),
    E("object-layout-change").m("4435"),
    E("old-style-cast").c_same().g_same(),
    E("overloaded-virtual").c_same().g_same(),
    E("padded").c_same().g_same().m("4820"),
    E("parentheses").c_same().g_same(),
    E("pedantic").c_same().g_same(),
    E("pointer-sign").c_same().g_same(),
    E("return-type").c_same().g_same(),
    E("shadow").c_same().g_same(),
    E("shift-sign-overflow").c_same(),
    E("shorten-64-to-32").c_same(),
    E("sign-compare").c_same().g_same().m("4389"),
    E("sign-conversion").c_same().g_same().m("4365"),
    E("signed-unsigned-compare").c("sign-compare").g("sign-compare").m("4388"),
    E("static-ctor-not-thread-safe").m("4640"),
    E("switch").c_same().g_same().m("4062"),
    E("switch-enum").c_same().g_same().m("4061"),
    E("this-used-in-init").m("4355"),
    E("undef").c_same().g_same().m("4668"),
    E("uninitialized").c_same().g_same(),
    E("unknown-pragmas").c_same().g_same(),
    E("unreachable-code").c_same().g_same().m("4702"),
    E("unreachable-code-return").c_same(),
    E("unsafe-conversion").m("4191"),
    E("unused-but-set-variable").g_same(),
    E("unused-function").c_same().g_same(),
    E("unused-label").c_same().g_same(),
    E("unused-parameter").c_same().g_same().m("4100"),
    E("unused-value").c_same().g_same().m("4555"),
    E("unused-variable").c_same().g_same(),
    E("used-but-marked-unused").c_same(),
    E("weak-vtables").c_same(),

    ### Objective-C
    E("arc-bridge-casts-disallowed-in-nonarc").c_same(),
    E("arc-repeated-use-of-weak").c_same(),
    E("deprecated-implementations").c_same(),
    E("duplicate-method-match").c_same(),
    E("explicit-ownership-type").c_same(),
    E("implicit-atomic-properties").c_same(),
    E("implicit-retain-self").c_same(),
    E("objc-missing-property-synthesis").c_same(),
    E("objc-root-class").c_same(),
    E("protocol").c_same().g_same(),
    E("receiver-is-weak").c_same(),
    E("selector").c_same().g_same(),
    E("strict-selector-match").c_same().g_same(),
    E("undeclared-selector").c_same().g_same(),
]

### Xcode attributes is nothing less but another name of clang option:
xcode_table = [
    E("bool-conversion").c_same().xc("BOOL_CONVERSION"),
    E("c++11-extensions").c_same().xc("CXX0X_EXTENSIONS"),
    E("constant-conversion").c_same().xc("CONSTANT_CONVERSION"),
    E("conversion").c_same().xc("SUSPICIOUS_IMPLICIT_CONVERSION"),
    E("deprecated-declarations").c_same().xg("ABOUT_DEPRECATED_FUNCTIONS"),
    E("deprecated-objc-isa-usage").c_same().xc("DIRECT_OBJC_ISA_USAGE"),
    E("documentation").c_same().xc("DOCUMENTATION_COMMENTS"),
    E("empty-body").c_same().xc("EMPTY_BODY"),
    E("enum-conversion").c_same().xc("ENUM_CONVERSION"),
    E("exit-time-destructors").c_same().xc("_EXIT_TIME_DESTRUCTORS"),
    E("format").c_same().xg("TYPECHECK_CALLS_TO_PRINTF"),
    E("four-char-constants").c_same().xg("FOUR_CHARACTER_CONSTANTS"),
    E("int-conversion").c_same().xc("INT_CONVERSION"),
    E("invalid-offsetof").c_same().xg("ABOUT_INVALID_OFFSETOF_MACRO"),
    E("missing-braces").c_same().xg("INITIALIZER_NOT_FULLY_BRACKETED"),
    E("missing-field-initializers").c_same().
        xg("ABOUT_MISSING_FIELD_INITIALIZERS"),
    E("missing-prototypes").c_same().xg("ABOUT_MISSING_PROTOTYPES"),
    E("newline-eof").c_same().xg("ABOUT_MISSING_NEWLINE"),
    E("non-virtual-dtor").c_same().xg("NON_VIRTUAL_DESTRUCTOR"),
    E("overloaded-virtual").c_same().xg("HIDDEN_VIRTUAL_FUNCTIONS"),
    E("parentheses").c_same().xg("MISSING_PARENTHESES"),
    E("pointer-sign").c_same().xg("ABOUT_POINTER_SIGNEDNESS"),
    E("return-type").c_same().xg("ABOUT_RETURN_TYPE"),
    E("shadow").c_same().xg("SHADOW"),
    E("shorten-64-to-32").c_same().xg("64_TO_32_BIT_CONVERSION"),
    E("sign-compare").c_same().xg("SIGN_COMPARE"),
    E("sign-conversion").c_same().xc("IMPLICIT_SIGN_CONVERSION"),
    E("switch").c_same().xg("CHECK_SWITCH_STATEMENTS"),
    E("uninitialized").c_same().xg("UNINITIALIZED_AUTOS"),
    E("unknown-pragmas").c_same().xg("UNKNOWN_PRAGMAS"),
    E("unused-function").c_same().xg("UNUSED_FUNCTION"),
    E("unused-label").c_same().xg("UNUSED_LABEL"),
    E("unused-parameter").c_same().xg("UNUSED_PARAMETER"),
    E("unused-value").c_same().xg("UNUSED_VALUE"),
    E("unused-variable").c_same().xg("UNUSED_VARIABLE"),

    ### Objective-C
    E("arc-bridge-casts-disallowed-in-nonarc").c_same().
        xc("_ARC_BRIDGE_CAST_NONARC").o(),
    E("arc-repeated-use-of-weak").c_same().xc("OBJC_REPEATED_USE_OF_WEAK").o(),
    E("deprecated-implementations").c_same().
        xc("DEPRECATED_OBJC_IMPLEMENTATIONS").o(),
    E("duplicate-method-match").c_same().xc("_DUPLICATE_METHOD_MATCH").o(),
    E("explicit-ownership-type").c_same().
        xc("OBJC_EXPLICIT_OWNERSHIP_TYPE").o(),
    E("implicit-atomic-properties").c_same().
        xc("OBJC_IMPLICIT_ATOMIC_PROPERTIES").o(),
    E("implicit-retain-self").c_same().xc("OBJC_IMPLICIT_RETAIN_SELF").o(),
    E("objc-missing-property-synthesis").c_same().
        xc("OBJC_MISSING_PROPERTY_SYNTHESIS").o(),
    E("objc-root-class").c_same().xc("OBJC_ROOT_CLASS").o(),
    E("protocol").c_same().xg("ALLOW_INCOMPLETE_PROTOCOL").o(),
    E("receiver-is-weak").c_same().xc("OBJC_RECEIVER_WEAK").o(),
    E("selector").c_same().xg("MULTIPLE_DEFINITION_TYPES_FOR_SELECTOR").o(),
    E("strict-selector-match").c_same().xg("STRICT_SELECTOR_MATCH").o(),
    E("undeclared-selector").c_same().xg("UNDECLARED_SELECTOR").o(),
]

### Apply xcode table
for xcode_entry in xcode_table:
  found = False
  for main_entry in main_warnings_table:
    if main_entry.clang.option == xcode_entry.clang.option:
      main_entry.xcode = xcode_entry.xcode
      main_entry.objc = xcode_entry.objc
      found = True
  if not found:
    sys.exit("Xcode entry `{}` not found".format(xcode_entry.warning_name))

### Verify that there is no entries that contain another entry
for some in main_warnings_table:
  for other in main_warnings_table:
    if some.bigger(other):
      print(
          "Entry `{}` contains `{}`".format(
              some.warning_name, other.warning_name
          )
      )
      sys.exit(1)

sugar.sugar_warnings_wiki_table_generator.generate(main_warnings_table)
sugar.sugar_warnings_leathers_generator.generate(main_warnings_table)
sugar.sugar_warnings_cmake_flags_generator.generate(main_warnings_table)
