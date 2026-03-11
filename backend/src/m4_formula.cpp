#include "minisheet/m4_formula.h"

#include "minisheet/m1_types.h"

#include "../third_party/tinyexpr/tinyexpr.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <vector>

using namespace std;

namespace minisheet {
namespace {

string uppercase(string value) {
  transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return value;
}

string lowercase(string value) {
  transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

string strip_formula_prefix(const string& formula) {
  string text = trim(formula);
  if (!text.empty() && text.front() == '=') {
    text.erase(text.begin());
  }
  return trim(text);
}

bool is_identifier_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

size_t skip_spaces(const string& text, size_t start) {
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    start += 1;
  }
  return start;
}

bool parse_range_ref(const string& text, CellRange& range) {
  string candidate = trim(text);
  size_t colon = candidate.find(':');
  if (colon == string::npos) {
    return false;
  }

  string first = uppercase(trim(candidate.substr(0, colon)));
  string second = uppercase(trim(candidate.substr(colon + 1)));
  if (!is_valid_cell_id(first) || !is_valid_cell_id(second)) {
    return false;
  }

  range = {parse_cell_id(first), parse_cell_id(second)};
  return true;
}

bool find_matching_paren(const string& text, size_t open_index, size_t& close_index) {
  int depth = 0;
  for (size_t index = open_index; index < text.size(); ++index) {
    if (text[index] == '(') {
      depth += 1;
    } else if (text[index] == ')') {
      depth -= 1;
      if (depth == 0) {
        close_index = index;
        return true;
      }
      if (depth < 0) {
        return false;
      }
    }
  }
  return false;
}

bool evaluate_plain_expression(const string& expression, const CellResolver& cell_resolver, double& value);

bool evaluate_function_call(const string& lower_name,
                            const string& argument_text,
                            const CellResolver& cell_resolver,
                            const RangeResolver& range_resolver,
                            double& value) {
  if (lower_name == "sum" || lower_name == "avg") {
    CellRange range;
    if (parse_range_ref(argument_text, range)) {
      return range_resolver(range, lower_name == "avg", value);
    }

    string nested = strip_formula_prefix(argument_text);
    if (nested.empty()) {
      return false;
    }

    return evaluate_plain_expression(nested, cell_resolver, value);
  }

  return false;
}

bool rewrite_expression(const string& input,
                        string& output,
                        const CellResolver& cell_resolver,
                        const RangeResolver& range_resolver) {
  output.clear();

  for (size_t index = 0; index < input.size();) {
    char current = input[index];
    if (std::isalpha(static_cast<unsigned char>(current))) {
      size_t start = index;
      while (index < input.size() && is_identifier_char(input[index])) {
        index += 1;
      }

      string identifier = input.substr(start, index - start);
      size_t lookahead = skip_spaces(input, index);
      if (lookahead < input.size() && input[lookahead] == '(') {
        size_t close_index = 0;
        if (!find_matching_paren(input, lookahead, close_index)) {
          return false;
        }

        string lower_name = lowercase(identifier);
        string args = input.substr(lookahead + 1, close_index - lookahead - 1);
        if (lower_name == "sum" || lower_name == "avg") {
          double function_value = 0.0;
          if (!evaluate_function_call(lower_name, args, cell_resolver, range_resolver, function_value)) {
            return false;
          }
          output += format_number(function_value);
        } else {
          string rewritten_args;
          if (!rewrite_expression(args, rewritten_args, cell_resolver, range_resolver)) {
            return false;
          }
          output += lower_name;
          output += "(";
          output += rewritten_args;
          output += ")";
        }
        index = close_index + 1;
      } else if (is_valid_cell_id(identifier)) {
        output += uppercase(identifier);
      } else {
        output += identifier;
      }
      continue;
    }

    output.push_back(current);
    index += 1;
  }

  return true;
}

vector<string> collect_cell_variables(const string& text) {
  vector<string> names;
  for (size_t index = 0; index < text.size();) {
    if (!std::isalpha(static_cast<unsigned char>(text[index]))) {
      index += 1;
      continue;
    }

    size_t start = index;
    while (index < text.size() && is_identifier_char(text[index])) {
      index += 1;
    }

    string identifier = text.substr(start, index - start);
    size_t lookahead = skip_spaces(text, index);
    if (lookahead < text.size() && text[lookahead] == '(') {
      continue;
    }

    if (is_valid_cell_id(identifier)) {
      string normalized = uppercase(identifier);
      if (find(names.begin(), names.end(), normalized) == names.end()) {
        names.push_back(normalized);
      }
    }
  }
  return names;
}

bool evaluate_plain_expression(const string& expression, const CellResolver& cell_resolver, double& value) {
  vector<string> names = collect_cell_variables(expression);
  vector<double> values;
  vector<te_variable> variables;
  values.reserve(names.size());
  variables.reserve(names.size());

  for (const string& name : names) {
    double numeric = 0.0;
    if (!cell_resolver(name, numeric)) {
      return false;
    }
    values.push_back(numeric);
  }

  for (size_t index = 0; index < names.size(); ++index) {
    variables.push_back({names[index].c_str(), &values[index], TE_VARIABLE, nullptr});
  }

  int error = 0;
  te_expr* expression_tree =
      te_compile(expression.c_str(), variables.empty() ? nullptr : variables.data(),
                 static_cast<int>(variables.size()), &error);
  if (expression_tree == nullptr || error != 0) {
    te_free(expression_tree);
    return false;
  }

  value = te_eval(expression_tree);
  te_free(expression_tree);
  if (!std::isfinite(value)) {
    return false;
  }
  return true;
}

}  // namespace

FormulaEvalResult evaluate_formula(const std::string& formula,
                                   const CellResolver& cell_resolver,
                                   const RangeResolver& range_resolver) {
  FormulaEvalResult result;
  string body = strip_formula_prefix(formula);
  if (body.empty()) {
    return result;
  }

  string rewritten;
  if (!rewrite_expression(body, rewritten, cell_resolver, range_resolver)) {
    return result;
  }

  double numeric = 0.0;
  if (!evaluate_plain_expression(rewritten, cell_resolver, numeric)) {
    return result;
  }

  result.ok = true;
  result.value = numeric;
  return result;
}

}  // namespace minisheet
