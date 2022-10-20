#include "nudl/analysis/testing/analysis_test.h"

namespace nudl {
namespace analysis {

TEST_F(AnalysisTest, SimpleLambdas) {
  // Fully bound types:
  CheckCode("lambda_test", "lambda_simple", R"(
// Types fully specified:
def ProcessNames(names: Array<String>) : UInt =>
  sum(map(names, (s : String) : UInt => len(s)))
)");
  // Types inferred from lambda up.
  CheckCode("lambda_test", "lambda_infer", R"(
// Lambda bound on call:
def ProcessNames(names: Array<String>) : UInt =>
  sum(map(names, s => len(s)))
)");
  // Types inferred two functions up.
  CheckCode("lambda_test", "lambda_double_infer", R"(
import cdm

// `names` and lambda bound on call:
def ProcessNames(names) =>
  sum(map(names, s => len(s)))

// Binds the ProcessNames3 fully upon call:
def UseProcessNames(name: cdm.HumanName) =>
  ProcessNames(name.prefix)
)");
  CheckCode("lambda_test", "lambda_builtin", R"(
// Using a standard function:
def ProcessNames(names: Array<String>) : UInt =>
  sum(map(names, len))
)");
  CheckCode("lambda_test", "lambda_external", R"(
// Using external function:
def ProcessNames(names: Array<cdm.HumanName>) =>
  sum(map(names, s => len(cdm.GetFamilyName(s))))
)");
  CheckCode("lambda_test", "lambda_fluent", R"(
// Showing fluent calls:
def ProcessNames(names: Array<String>) =>
  names.map(s => len(s)).sum()
)");
  CheckCode("lambda_test", "lambda_default_value", R"(
def FilterName(names: Array<String>, extra: String) => {
  filtered = names.filter((name, arg=extra) => { len(name) > len(arg) });
  return not filtered.empty();
})");
  CheckCode("lambda_test", "lambda_default_value_with_return", R"(
def ProcessNames(names: Array<String>, min_len: UInt) => {
  names.map((s, m=min_len) => len(s) - m).sum()
})");
}

TEST_F(AnalysisTest, LambdaReturns) {
  // Function returning a lambda:
  CheckCode("lambda_test", "return_lambda", R"(
def get_fun() => s : Int => s + 10
)");
  CheckError("abstract_return", R"(
def get_fun() => { s => s + 10 }
)",
             "Please add non-abstract type specifications");
}

TEST_F(AnalysisTest, LambdaVars) {
  CheckCode("lambda_test", "call_lambda", R"(
def add(x: Int) => {
  adder = (s: Int) : Int => s + 10;
  adder(x)
})");
  CheckError("abstract_lambda_var", R"(
def add(x: Int) => {
  adder = (s => s + 10);
  adder(x)
})",
             "Please add non-abstract type specifications");
}

}  // namespace analysis
}  // namespace nudl