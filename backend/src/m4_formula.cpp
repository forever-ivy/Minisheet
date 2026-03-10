#include "minisheet/m4_formula.h"

#include "minisheet/m1_types.h"

#include "../third_party/tinyexpr/tinyexpr.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace minisheet {
namespace {

std::string uppercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::toupper(ch));
  });
  return value;
}

std::string lowercase(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
    return static_cast<char>(std::tolower(ch));
  });
  return value;
}

std::string strip_formula_prefix(const std::string& formula) {
  std::string text = trim(formula);
  if (!text.empty() && text.front() == '=') {
    text.erase(text.begin());
  }
  return trim(text);
}

bool is_identifier_char(char ch) {
  return std::isalnum(static_cast<unsigned char>(ch)) || ch == '_';
}

std::size_t skip_spaces(const std::string& text, std::size_t start) {
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    start += 1;
  }
  return start;
}

std::optional<CellRange> parse_range_ref(const std::string& text) {
  std::string candidate = trim(text);
  std::size_t colon = candidate.find(':');
  if (colon == std::string::npos) {
    return std::nullopt;
  }

  std::string first = uppercase(trim(candidate.substr(0, colon)));
  std::string second = uppercase(trim(candidate.substr(colon + 1)));
  if (!is_valid_cell_id(first) || !is_valid_cell_id(second)) {
    return std::nullopt;
  }
  return CellRange {parse_cell_id(first), parse_cell_id(second)};
}

bool find_matching_paren(const std::string& text, std::size_t open_index, std::size_t& close_index) {
  int depth = 0;
  for (std::size_t index = open_index; index < text.size(); ++index) {
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

std::optional<double> evaluate_plain_expression(const std::string& expression,
                                                const CellResolver& cell_resolver);

std::optional<double> evaluate_function_call(const std::string& lower_name,
                                             const std::string& argument_text,
                                             const CellResolver& cell_resolver,
                                             const RangeResolver& range_resolver) {
  if (lower_name == "sum" || lower_name == "avg") {
    if (auto range = parse_range_ref(argument_text); range.has_value()) {
      return range_resolver(*range, lower_name == "avg");
    }

    std::string nested = strip_formula_prefix(argument_text);
    if (nested.empty()) {
      return std::nullopt;
    }

    auto value = evaluate_plain_expression(nested, cell_resolver);
    if (!value.has_value()) {
      return std::nullopt;
    }
    return value.value();
  }

  return std::nullopt;
}

bool rewrite_expression(const std::string& input,
                        std::string& output,
                        const CellResolver& cell_resolver,
                        const RangeResolver& range_resolver) {
  output.clear();

  for (std::size_t index = 0; index < input.size();) {
    char current = input[index];
    if (std::isalpha(static_cast<unsigned char>(current))) {
      std::size_t start = index;
      while (index < input.size() && is_identifier_char(input[index])) {
        index += 1;
      }

      std::string identifier = input.substr(start, index - start);
      std::size_t lookahead = skip_spaces(input, index);
      if (lookahead < input.size() && input[lookahead] == '(') {
        std::size_t close_index = 0;
        if (!find_matching_paren(input, lookahead, close_index)) {
          return false;
        }

        std::string lower_name = lowercase(identifier);
        std::string args = input.substr(lookahead + 1, close_index - lookahead - 1);
        if (lower_name == "sum" || lower_name == "avg") {
          auto value = evaluate_function_call(lower_name, args, cell_resolver, range_resolver);
          if (!value.has_value()) {
            return false;
          }
          output += format_number(value.value());
        } else {
          std::string rewritten_args;
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

std::unordered_set<std::string> collect_cell_variables(const std::string& text) {
  std::unordered_set<std::string> result;
  for (std::size_t index = 0; index < text.size();) {
    if (!std::isalpha(static_cast<unsigned char>(text[index]))) {
      index += 1;
      continue;
    }

    std::size_t start = index;
    while (index < text.size() && is_identifier_char(text[index])) {
      index += 1;
    }

    std::string identifier = text.substr(start, index - start);
    std::size_t lookahead = skip_spaces(text, index);
    if (lookahead < text.size() && text[lookahead] == '(') {
      continue;
    }

    if (is_valid_cell_id(identifier)) {
      result.insert(uppercase(identifier));
    }
  }
  return result;
}

std::optional<double> evaluate_plain_expression(const std::string& expression,
                                                const CellResolver& cell_resolver) {
  std::unordered_set<std::string> names = collect_cell_variables(expression);
  std::vector<std::string> ordered_names(names.begin(), names.end());
  std::vector<double> values;
  std::vector<te_variable> variables;
  values.reserve(ordered_names.size());
  variables.reserve(ordered_names.size());

  for (const std::string& name : ordered_names) {
    std::optional<double> value = cell_resolver(name);
    if (!value.has_value()) {
      return std::nullopt;
    }
    values.push_back(value.value());
  }

  for (std::size_t index = 0; index < ordered_names.size(); ++index) {
    variables.push_back({ordered_names[index].c_str(), &values[index], TE_VARIABLE, nullptr});
  }

  int error = 0;
  te_expr* expression_tree =
      te_compile(expression.c_str(), variables.empty() ? nullptr : variables.data(),
                 static_cast<int>(variables.size()), &error);
  if (expression_tree == nullptr || error != 0) {
    te_free(expression_tree);
    return std::nullopt;
  }

  double value = te_eval(expression_tree);
  te_free(expression_tree);
  if (!std::isfinite(value)) {
    return std::nullopt;
  }
  return value;
}

}  // namespace

std::unordered_set<std::string> extract_formula_references(const std::string& formula) {
  std::unordered_set<std::string> references;
  std::string body = strip_formula_prefix(formula);

  for (std::size_t index = 0; index < body.size();) {
    if (!std::isalpha(static_cast<unsigned char>(body[index]))) {
      index += 1;
      continue;
    }

    std::size_t start = index;
    while (index < body.size() && std::isalpha(static_cast<unsigned char>(body[index]))) {
      index += 1;
    }
    std::size_t digit_start = index;
    while (index < body.size() && std::isdigit(static_cast<unsigned char>(body[index]))) {
      index += 1;
    }

    if (digit_start == index) {
      continue;
    }

    std::string first = uppercase(body.substr(start, index - start));
    if (!is_valid_cell_id(first)) {
      continue;
    }

    std::size_t colon_pos = skip_spaces(body, index);
    if (colon_pos < body.size() && body[colon_pos] == ':') {
      std::size_t second_start = skip_spaces(body, colon_pos + 1);
      std::size_t second_index = second_start;
      while (second_index < body.size() && std::isalpha(static_cast<unsigned char>(body[second_index]))) {
        second_index += 1;
      }
      std::size_t second_digit_start = second_index;
      while (second_index < body.size() && std::isdigit(static_cast<unsigned char>(body[second_index]))) {
        second_index += 1;
      }

      std::string second = uppercase(body.substr(second_start, second_index - second_start));
      if (second_digit_start != second_index && is_valid_cell_id(second)) {
        CellCoord start_coord = parse_cell_id(first);
        CellCoord end_coord = parse_cell_id(second);
        for (int row = std::min(start_coord.row, end_coord.row); row <= std::max(start_coord.row, end_coord.row);
             ++row) {
          for (int column = std::min(start_coord.column, end_coord.column);
               column <= std::max(start_coord.column, end_coord.column); ++column) {
            references.insert(to_cell_id({row, column}));
          }
        }
        index = second_index;
        continue;
      }
    }

    references.insert(first);
  }

  return references;
}

FormulaEvalResult evaluate_formula(const std::string& formula,
                                   const CellResolver& cell_resolver,
                                   const RangeResolver& range_resolver) {
  FormulaEvalResult result;
  result.references = extract_formula_references(formula);

  std::string body = strip_formula_prefix(formula);
  if (body.empty()) {
    return result;
  }

  std::string rewritten;
  if (!rewrite_expression(body, rewritten, cell_resolver, range_resolver)) {
    return result;
  }

  std::optional<double> numeric = evaluate_plain_expression(rewritten, cell_resolver);
  if (!numeric.has_value()) {
    return result;
  }

  result.ok = true;
  result.value = numeric.value();
  return result;
}

}  // namespace minisheet
