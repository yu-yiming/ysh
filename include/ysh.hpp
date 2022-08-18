#pragma once

#include "entity.hpp"
#include "strutils.hpp"

namespace ysh {

namespace stdf = std::filesystem;
namespace stdn = std::numbers;
namespace stdr = std::ranges;
namespace stdv = std::ranges::views;

/**
 * @brief The standard form of a built-in command. The convention for main function of
 * C/C++ executable should have been int (*)(int, char* []), but that's too C-like. Of
 * course, I provide a helper function @ref forward_args to convert C-style arguments
 * to command_t. For each built-in command, I also provide a xxx_main function that 
 * follows C/C++ convention.
 */
using command_t = int (*)(std::vector<ysh::string> const&);

/**
 * @brief The enum type representing the options of a command. The 64 bits of an enum_t
 * value acts as a bitset for 0-9a-zA-Z (62 distinct values).
 */
using enum_t = unsigned long long;

/**
 * @brief The input type in ysh. This is the string I deal with the most in this project
 * as it's almost always more lightweight than std::string.
 */
using input_t = ::ysh::string;

using env_t = std::unordered_map<input_t, entity_t>;

/**
 * @brief Mapping long options to short ones based on the command being called.
 */
using optmap_t = std::unordered_map<input_t, char, typename input_t::hash>;

/**
 * @brief The token type.
 */
enum token_t : unsigned char {
    YSH_COMMENT,      // e.g. # comment
    YSH_EMPTY,
    YSH_EXPRESSION,   // e.g. (1 + 2)
    YSH_NAME,         // e.g. this_is_a_name
    YSH_OPERATOR,     // e.g. <<, |
    YSH_OPTION,       // e.g. -asdf
    YSH_PACK,         // e.g. [-n 123]
    YSH_SCRIPT,       // e.g. { echo 123 }
    YSH_STRING        // e.g. "this is a string"
};

enum builtin_operator_t : unsigned char {
    YSH_NON_BUILTIN,
    YSH_ABSTR,  // ->
    YSH_ADD,    // +
    YSH_AND,    // &
    YSH_APP,    // $
    YSH_ASSIGN, // <-
    YSH_CONCAT, // ++
    YSH_CONS,   // :
    YSH_DIV,    // /
    YSH_EQ,     // =
    YSH_GE,     // >=
    YSH_GT,     // >
    YSH_LE,     // <=
    YSH_LT,     // <
    YSH_MOD,    // %
    YSH_MUL,    // *
    YSH_NE,     // !=
    YSH_OR,     // |
    YSH_POW,    // ^
    YSH_SEQ,    // ;
    YSH_SHL,    // <<
    YSH_SHR,    // >>
    YSH_SUB,    // -
    YSH_ZIP,    // ,
};

inline ysh::string const k_alpha = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
inline ysh::string const k_digits = "0123456789";
inline ysh::string const k_alnum = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_0123456789";
inline ysh::string const k_operators = "@$%^&*-+=|:<,>.?/";

extern std::string g_line;
extern bool g_is_running;
extern std::mutex g_mutex;
extern stdf::path g_current_path;
extern std::unordered_map<input_t, command_t> g_command_map;
extern env_t g_variables;
inline std::unordered_set<input_t, typename input_t::hash> g_left_associative = {
    "^", "*", "/", "%", "+", "-", "<", ">", "=", "!=", "<=", ">=", "&", "|", ",", ";"
};
inline std::unordered_set<input_t, typename input_t::hash> g_right_associative = {
    "$", ":", "<-", "->"
};
inline std::unordered_map<input_t, int, typename input_t::hash> g_precedence = {
    { "$", 100 },
    { ":", 90 },
    { "<-", 85 },
    { "^", 80 },
    { "*", 70 }, { "/", 70 }, { "%", 70 },
    { "+", 60 }, { "-", 60 },
    { "<", 50 }, { ">", 50 }, { "=", 50 }, { "!=", 50 }, { "<=", 50 }, { ">=", 50 },
    { "&", 40 }, { "|", 40 },
    { "<<", 30 }, { ">>", 30 },
    { ",", 20 },
    { "->", 10 },
    { ";", 0 }
};


/**
 * @brief Execute a single command, probably on a new process.
 * 
 * @param cmd A command name, which will be further processed in this function.
 * @param args The (ungrouped) arguments of the command, including options.
 * @return int Whether the execution is successfully carried out.
 */
int execute(input_t cmd, std::vector<input_t> const& args);

/**
 * @brief Evaluate a script expression, usually a arithmetic expression.
 * Grammar:
 *   <expression> = <term> (("+"|"-") <term>)*
 *   <term> = <factor> (("*"|"/") <factor>)*
 *   <factor> = <item> ("^" <item>)*
 *   <item> = <group> (" " <group>)*
 *   <group> = "(" <expression> ")" | <identifier> | <number> | <string>
 * 
 * @param expr A script expression (must be surrounded by a pair of parentheses).
 * @param env A temporary environment to store bound variables.
 * @return entity_t The result of the evaluation.
 */
entity_t evaluate(input_t expr, env_t& env);

/**
 * @brief Forward the C-style argument lists to C++-style ones.
 * 
 * @param argc The count of the arguments.
 * @param argv The list of arguments.
 * @return std::vector<input_t> A vector of arguments.
 */
std::vector<input_t> forward_args(int argc, char* argv[]);

auto function(input_t name) {
    static auto const fnmap = std::unordered_map<input_t, builtin_operator_t, typename input_t::hash> {
        { "+",  YSH_ADD },
        { "&",  YSH_AND },
        { "$",  YSH_APP },
        { "<-", YSH_ASSIGN },
        { ":",  YSH_CONCAT },
        { ",",  YSH_CONS },
        { "/",  YSH_DIV },
        { "=",  YSH_EQ },
        { ">=", YSH_GE },
        { ">",  YSH_GT },
        { "<=", YSH_LE },
        { "<",  YSH_LT },
        { "%",  YSH_MOD },
        { "*",  YSH_MUL },
        { "!=", YSH_NE },
        { "|",  YSH_OR },
        { "^",  YSH_POW },
        { ";",  YSH_SEQ },
        { "<<", YSH_SHL },
        { ">>", YSH_SHR },
        { "-",  YSH_SUB }
    };
    return fnmap.at(name);
}

/**
 * @brief Generate an enum_t object based on short options provided.
 * 
 * @param opts 
 * @return enum_t 
 */
enum_t generate_opts(std::vector<input_t> const& opts);

void get_hint(std::string& input);

bool get_line(std::istream& is, std::string& line);

std::istream& input_stream(input_t name);

bool is_floating_point(input_t token);

bool is_identifier(input_t token);

bool is_integer(input_t token);

bool is_operator(input_t token);

bool is_string(input_t token);

/**
 * @brief Get arguments for the command running on the current thread. Each argument is
 * bound to an option (or the command itself). When the option is not provided, the 
 * direct arguments will be returned.
 * 
 * @param opt The option name.
 * @return std::vector<input_t>& 
 */
std::vector<input_t>& local_arguments(char opt = '\0');

std::vector<input_t>& local_arguments(enum_t opt);

/**
 * @brief Turn an option (as a character) to its enum_t form. For example, 'B' => 2, 'C' => 4
 * 
 * @param opt 
 * @return enum_t 
 */
enum_t option(char opt);

/**
 * @brief Get the flag position of an option.
 * @param opt [a-zA-Z0-9]
 */
int order(char opt);

std::ostream& output_stream(input_t name);

void prepare(std::vector<input_t> const& args);

int run_separate_process(returning<int> auto&& program);

/**
 * @brief Start a shell streaming from @param is and to @param os.
 * 
 * @param is The input stream.
 * @param os The output stream.
 * @return int 
 */
int shell(std::istream& is, std::ostream& os);

/**
 * @brief Convert an infix expression to a postfix expression.
 * For example, "1 + 2 * sin 3" => "1 2 3 sin * +", and "magic 1 2 3 + 4 * 5" => "1 2 3 magic 4 + 5 *"
 * 
 * @param tokens 
 * @return std::vector<input_t> 
 */
std::vector<input_t> shunting_yard(std::vector<input_t> const& tokens);

void swap_streams(std::ios_base& s1, std::ios_base& s2);

/**
 * @brief Tokenize the given script line.
 * Rules:
 * - The tokens are usually separated by spaces. e.g. abc "123" #@a are three tokens
 * - Some tokens are surrounded by specific symbols. e.g. "123" (1 + 2) { echo abc }
 * - These special tokens don't have to be separated by spaces. e.g. echo"123" are two tokens
 * - The expression token (e.g. (1 + 2)) and the code snippet token (e.g. { echo abc }) are not
 * further tokenized in this function.
 * - Within the string token (e.g. "123") there are escape characters (starting with \ followed
 * by one more character) that should be interpreted as text. e.g. "123\"123\"123" is one token (instead of three).
 * - Packs are options bound to some arguments. It's possible that a pack contains nested expressions.
 * 
 * @param line 
 * @return std::vector<std::pair<token_t, std::string_view>> 
 */
std::vector<std::pair<token_t, input_t>> tokenize(input_t const& line);

/**
 * @brief Main entry (forwarded from the main function) of the ysh program.
 * @param argc Count of the arguments.
 * @param argv List of the arguments.
 * @return int The exit code.
 */
int ysh_main(int argc, char* argv[]);

} // namespace ysh