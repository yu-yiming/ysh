#include "../include/ysh.hpp"
#include "../include/lambda.hpp"

namespace ysh {

/**
 * @brief Coroutine boilerplate. 
 * 
 */
class token_generator {
public:
    struct promise_type {
        using handle_type = std::coroutine_handle<promise_type>;

        std::pair<token_t, input_t> value;

        token_generator get_return_object() {
            return { handle_type::from_promise(*this) };
        }
        std::suspend_always initial_suspend() {
            return {};
        }
        std::suspend_always final_suspend() noexcept {
            return {};
        }
        void return_void() {
            // Do nothing.
        }
        [[noreturn]]
        void unhandled_exception() {
            throw;
        }
        std::suspend_always yield_value(input_t str) {
            using enum token_t;
            auto& [type, input] = value;
            input = str;
            if (str.empty()) {
                type = YSH_EMPTY;
                return {};
            }
            switch (str[0]) {
            case '#':
                type = YSH_COMMENT; break;
            case '(':
                type = YSH_EXPRESSION; break;
            case '-':
                type = YSH_OPTION; break;
            case '[':
                type = YSH_PACK; break;
            case '{':
                type = YSH_SCRIPT; break;
            case '"':
                type = YSH_STRING; break;
            case '<':
            case '>':
            case '|':
            case '&':
                type = YSH_OPERATOR; break;
            default:
                type = YSH_NAME; break;
            }
            return {};
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

    class iterator_type {
    public:
        explicit iterator_type(handle_type handle)
            : m_handle(handle) {}

        void operator ++() {
            m_handle.resume();
        }

        std::pair<token_t, input_t> operator *() {
            return m_handle.promise().value;
        }

        bool operator ==(std::default_sentinel_t) const {
            return !m_handle || m_handle.done();
        }
    private:
        handle_type m_handle;
    };

    token_generator(handle_type h)
        : m_handle(h) {}

    token_generator(token_generator const&) = delete;

    token_generator(token_generator&& other) noexcept
        : m_handle(other.m_handle) {
        other.m_handle = nullptr;
    }

    ~token_generator() {
        if (m_handle) {
            m_handle.destroy();
        }
    }

    iterator_type begin() {
        if (m_handle) {
            m_handle.resume();
            if (m_handle.done()) {
                m_handle = nullptr;
            }
        }
        return iterator_type(m_handle);
    }

    void destroy() { 
        m_handle.destroy();
    }

    std::default_sentinel_t end() {
        return {};
    }

    std::pair<token_t, input_t> next() {
        this->resume();
        return m_handle.promise().value;
    }

    token_generator& operator =(token_generator const&) = delete;

    token_generator& operator =(token_generator&& other) noexcept {
        if (this == &other) {
            return *this;
        }
        if (m_handle) {
            m_handle.destroy();
        }
        m_handle = other.m_handle;
        other.m_handle = nullptr;
        return *this;
    }

    void resume() {
        m_handle.resume();
    }

private:
    handle_type m_handle;
};


entity_t evaluate(input_t expr, env_t& env) {
    auto const first = [](auto&& first, auto&&... rest) {
        return first;
    };
    auto const second = [](auto&& first, auto&& second, auto&&... rest) {
        return second;
    };

    auto const app = [](auto&& f, entity_t const& arg_1, entity_t const& arg_2) -> decltype(f(arg_1, arg_2)) {
        using function_type = entity_t (*)(decltype(arg_1), decltype(arg_2));
        if constexpr (std::is_same_v<TYPE(f), input_t>) {
            switch (function(f)) {
                case YSH_ABSTR:  return operator_abstract(arg_1, arg_2);
                case YSH_ADD:    return arg_1 + arg_2;
                case YSH_AND:    return arg_1 & arg_2;
                case YSH_APP:    return operator_apply(arg_1, arg_2);
                case YSH_ASSIGN: return const_cast<entity_t&>(arg_1) = arg_2;
                case YSH_CONCAT: return operator_concat(arg_1, arg_2);
                case YSH_DIV:    return arg_1 / arg_2;
                case YSH_EQ:     return arg_1 == arg_2;
                case YSH_GE:     return entity_t(arg_1 >= arg_2);
                case YSH_GT:     return arg_1 > arg_2;
                case YSH_LE:     return arg_1 <= arg_2;
                case YSH_LT:     return arg_1 < arg_2;
                case YSH_MOD:    return arg_1 % arg_2;
                case YSH_MUL:    return arg_1 * arg_2;
                case YSH_NE:     return arg_1 != arg_2;
                case YSH_OR:     return arg_1 | arg_2;
                case YSH_POW:    return arg_1 ^ arg_2;
                case YSH_SEQ:    return arg_2;
                case YSH_SHL:    return arg_1 << arg_2;
                case YSH_SHR:    return arg_1 >> arg_2;
                case YSH_SUB:    return arg_1 - arg_2;
            }
        }
        else {

        }
    };

    std::string spaced_expr;
    // Add spaces around operators. e.g. "a+b<<3" -> "a + b << 3"
    for (auto ch : expr) {
        if (ispunct(ch)) {
            spaced_expr += ' ';
            spaced_expr += ch;
            spaced_expr += ' ';
        } 
        else {
            spaced_expr += ch;
        }
    }
    // Split the expression into tokens.
    auto tokens = spaced_expr | stdv::split(' ') 
                              | stdv::filter([](auto const& token) {
                                    return !token.empty();
                                })
                              | stdv::transform([](auto const& token) {
                                    return input_t(token.begin(), token.end());
                              });
    
    auto postfix_tokens = shunting_yard({ tokens.begin(), tokens.end() });
    auto operands = std::stack<entity_t>();

    for (auto token : postfix_tokens) {
        if (is_identifier(token)) {
            operands.push(g_variables[token]);
        }
        else if (is_integer(token)) {
            int value;
            std::from_chars(token.data(), token.data() + token.size(), value);
            operands.emplace(value);
        }
        else if (is_floating_point(token)) {
            double value;
            std::from_chars(token.data(), token.data() + token.size(), value);
            operands.emplace(value);
        }
        else if (is_operator(token)) {
            auto rhs = operands.top();
            operands.pop();
            auto lhs = operands.top();
            operands.pop();
            operands.push(app(token, lhs, rhs));
        }
    }
}

inline std::vector<input_t> forward_args(int argc, char* argv[]) {
    auto result = std::vector<input_t>();
    for (auto i = 0; i < argc; ++i) {
        result[i] = input_t(argv[i], strnlen(argv[i], 256));
    }
    return result;
}

inline enum_t generate_opts(std::vector<input_t> const& opts) {
    auto result = enum_t();
    for (auto const& opt : opts) {
        for (auto ch : opt) {
            result |= option(ch);
        }
    }
    return result;
}

inline bool get_line(std::istream& is, std::string& line) {
    char ch;
    while (is >> ch) {
        if (ch == '\n') {
            return true;
        }
        else if (ch == '\t') {
            get_hint(line);
        }
        line += ch;
    }
    return false;       // EOF
}

std::istream& input_stream(input_t name) {
    if (name == "stdin") {
        return std::cin;
    }
    std::ifstream& file = *new std::ifstream(std::string(name));
    if (!file.is_open()) {
        throw std::runtime_error("failed to open file");
    }
    return file;
}

bool is_floating_point(input_t token) {
    auto dot_or_e_1 = token.find_first_not_of(k_digits);
    auto dot_or_e_2 = token.find_last_not_of(k_digits);
    if (dot_or_e_1 == dot_or_e_2) {
        return *dot_or_e_1 == '.' || *dot_or_e_1 == 'e';
    }
    return *dot_or_e_1 == '.' && *dot_or_e_2 == 'e' && token.find_first_not_of(k_digits, dot_or_e_1 + 1) == dot_or_e_2;
}

bool is_identifier(input_t token) {
    if (isdigit(token[0])) {
        return false;
    }
    return token.find_first_not_of(k_alnum) == token.end();
}

bool is_integer(input_t token) {
    return token.find_first_not_of(k_digits) == token.end();
}

bool is_operator(input_t token) {
    return token.find_first_not_of(k_operators) == token.end();
}

bool is_string(input_t token) {
    if (token[0] != '"') {
        return false;
    }
    auto quote = token.find('"', token.begin() + 1);
    while (quote != token.end() - 1) {
        if (quote[-1] != '\\') {
            return false;
        }
        quote = token.find('"', quote + 1);
    }
    return quote[-1] != '\\';
}

std::vector<input_t>& local_arguments(char opt) {
    static thread_local std::vector<std::vector<input_t>> arguments(64);
    if (opt == '\0') {
        return arguments.back();
    }
    return arguments[order(opt)];
}

std::vector<input_t>& local_arguments(enum_t opt) {
    auto power = std::size_t(opt);
    auto ord = 0;
    while (power > 0) {
        power >>= 1;
        ++ord;
    }
    if (ord >= order('a') && ord <= order('z')) {

    }
}

inline enum_t option(char opt) {
    return 1ull << order(opt);
}

inline int order(char opt) {
    if (opt >= '0' && opt <= '9') {
        return opt - '0' + 1;
    }
    else if (opt >= 'A' && opt <= 'Z') {
        return opt - 'A' + 11;
    }
    else if (opt >= 'a' && opt <= 'z') {
        return opt - 'a' + 27;
    }
    return opt;
}

static std::unordered_map<input_t, char, typename input_t::hash> optmap = {
    { "continue", 'c' },
    { "help", 'h' },
    { "output-stream", 'o' },
    { "separate-process", 'p' }
};

std::ostream& output_stream(input_t name) {
    if (name == "stdout") {
        return std::cout;
    }
    else if (name == "stderr") {
        return std::cerr;
    }
    
    std::ofstream& result = *new std::ofstream(std::string(name));
    if (!result.is_open()) {
        throw std::runtime_error("failed to open output stream");
    }
    return result;
}

/**
 * @brief A coroutine-based tokenizer.
 * 
 * @param line 
 * @return token_generator 
 */
static token_generator parse(input_t line) {
    auto begin = line.begin();
    auto end = line.end();
    auto it = begin;

    static auto compound = std::unordered_map<char, char> {
        { '(', ')' },
        { '{', '}' },
        { '[', ']' },
        { '\'', '\'' },
        { '"', '"' },
        { '`', '`' }
    };

    auto left = '\0';
    auto s = std::stack<char>();
    auto term = '\0';

    while (it != end) {
        auto ch = *it;
        // Yield a token as spaces are the delimiters.
        switch (ch) {
        case ' ':
            co_yield { begin, it };
            begin = ++it;
            continue;
        case '(': case '[': case '{': case '\'': case '"': case '`':
            s.push(ch);
            break;
        case ')': case ']': case '}':
            if (s.empty()) {
                throw std::runtime_error("unbalanced parentheses");
            }
            if (compound[s.top()] == ch) {
                auto top = s.top();
                s.pop();
                if (s.empty()) {
                    co_yield { begin, ++it };
                    begin = it;
                    continue;
                }
            }
            break;
        case '\\':
            ++it;
            if (it == end) {
                throw std::runtime_error("unexpected end of line");
            }
            auto escape = *it;
            switch (escape) {
                
            }
        default: break;
        }
    }

}

enum_t prepare(std::vector<input_t> const& args, optmap_t const& optmap) {
    enum_t result;

    auto const add_long_opt = [&optmap](auto&& opt_name) {
        auto long_opt = opt_name.substr(opt_name.begin() + 2);
        if (not optmap.contains(long_opt)) {
            throw std::runtime_error("unknown option: " + long_opt);
        }
        return option(optmap.at(long_opt));
    };

    auto const add_short_opt = [&optmap](auto&& opt) {
        if (not optmap.contains(opt)) {
            throw std::runtime_error("unknown option: " + opt);
        }
        return option(optmap.at(opt));
    };

    for (auto arg : args) {
        arg = arg.trim();
        // packs: [opt, arg1, arg2, ...]
        if (arg.starts_with('[')) {
            auto content = arg.substr(arg.begin() + 1, arg.end() - 1);
            auto parts = content.split(" ");
            auto opt_name = parts[0];
            auto opts = std::vector<enum_t>();

            if (opt_name.starts_with("--")) {
                opts.push_back(add_long_opt(opt_name));
                result |= opts.back();
            }
            else if (opt_name.starts_with('-')) {
                opts.push_back(add_short_opt(opt_name));
                result |= opts.back();
            }

            for (auto opt : opts) {
                auto& opt_args = local_arguments(opt);
                opt_args.clear();
                
                for (auto part : parts | stdv::drop(1)) {
                    opt_args.push_back(part);
                }
            }
        }
        else if (arg.starts_with("--")) {
            result |= add_long_opt(arg);
        }
        else if (arg.starts_with('-')) {
            result |= add_short_opt(arg);
        }
        else {
            local_arguments().push_back(arg);
        }
    }
    return result;
}

int run_separate_process(returning<int> auto&& program) {
    pid_t pid = 0;
    pid = fork();
    if (pid == 0) {
        exit(program());
    }
    else {
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            std::cerr << "Error: waitpid() failed.\n";
            return EXIT_FAILURE;
        }
        if (WIFEXITED(status)) {
            int returned = WEXITSTATUS(status);
            std::cerr << "Shell exited with status " << returned << "\n";
            return returned;
        }
        else if (WIFSIGNALED(status)) {
            int signal = WTERMSIG(status);
            std::cerr << "Shell was terminated by signal " << signal << "\n";
            return EXIT_FAILURE;
        }
        else if (WIFSTOPPED(status)) {
            int signal = WSTOPSIG(status);
            std::cerr << "Shell was stopped by signal " << signal << "\n";
            return EXIT_FAILURE;
        }
        else {
            std::cerr << "Shell exited with unknown status.\n";
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
}

int shell(std::istream& is, std::ostream& os) {
    std::string line, partial_line;
    bool continued = true;
    while (true) {
        bool new_line = true;
        while (new_line && (continued = get_line(is, partial_line))) {
            line += partial_line;
            new_line = (line.size() - line.find_last_not_of('\\')) % 2 == 0;
            if (new_line) {
                line.pop_back();
            }
        }
        if (not continued) {
            break;
        }

        auto tokens = tokenize({ line.begin(), line.end() });
        line.clear();
    }
    if (is.bad()) {
        std::cerr << "Error: input stream is bad.\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

std::vector<input_t> shunting_yard(std::vector<input_t> const& tokens) {
    auto result = std::vector<input_t>();
    auto s = std::stack<input_t>();
    auto size = tokens.size();

    for (auto i = 0uz; i < size; ++i) {
        auto token = tokens[i];
        if (is_operator(token) && (g_left_associative.contains(token) || g_right_associative.contains(token))) {
            while (!s.empty() && is_operator(s.top())) {
                if ((g_left_associative.contains(token) && g_precedence[token] <= g_precedence[s.top()]) ||
                    (g_right_associative.contains(token) && g_precedence[token] < g_precedence[s.top()])) {
                    result.push_back(s.top());
                    s.pop();
                    continue;
                }
                break;
            }
            s.push(token);
        }
        else if (token == "(") {
            s.push(token);
        }
        else if (token == ")") {
            while (!s.empty() && s.top() != "(") {
                result.push_back(s.top());
                s.pop();
            }
            s.pop();
        }
        else {
            while (i + 1 < size && is_operator(tokens[i + 1])) {
                result.push_back(tokens[++i]);
            }
            result.push_back(token);
        }
    }
    while (!s.empty()) {
        result.push_back(s.top());
        s.pop();
    }
    return result;
}

void swap_streams(std::ios_base& s1, std::ios_base& s2) {

}

std::vector<std::pair<token_t, input_t>> tokenize(input_t const& line) {
    auto result = std::vector<std::pair<token_t, input_t>>();
    auto gen = parse(line);
    for (auto const& value : gen) {
        result.push_back(value);
    }
    return result;
}

int ysh(enum_t opts) {
    bool show_help = opts | option('h');
    bool start_shell = opts | option('c');
    bool separate_process = opts | option('p');

    auto& ostrm = opts | option('o') ? output_stream(local_arguments('o')[0]) : std::cout;
    auto& istrm = local_arguments().empty() ? std::cin : input_stream(local_arguments()[0]);

    if (show_help) {
        ostrm << "usage: ysh [-i input-stream] [-o ouput-stream] [-chp]\n"
              << "\t-c --continue\n"
              << "\t\tContinue the shell after the input stream is exhausted.\n"
              << "\t\tIgnored if the input stream is stdin.\n"
              << "\t-h --help\n"
              << "\t\tShow this help message on startup.\n"
              << "\t-i --input-stream\n"
              << "\t\tSpecify the input stream.\n"
              << "\t-o --output-stream\n"
              << "\t\tSpecify the output stream (can be altered later).\n"
              << "\t-p --separate-process\n"
              << "\t\tRun the shell in a separate process.\n";
    }

    int retval;
    if (separate_process) {
        retval = run_separate_process([&istrm, &ostrm] {
            return shell(istrm, ostrm);
        });
    }
    else {
        retval = shell(istrm, ostrm);
    }
    if (retval != EXIT_SUCCESS) {
        return retval;
    }

    if (start_shell && &istrm != &std::cin) {
        // Redirect to stdin.
        local_arguments().clear();
        return ysh(0);
    }
    return EXIT_SUCCESS;
}

inline int ysh_main(int argc, char* argv[]) {
    return ysh(prepare(forward_args(argc, argv), optmap));
}

} // namespace ysh