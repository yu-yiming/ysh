#include "../include/entity.hpp"

namespace ysh::types {

entity standard_error(std::string msg) {
    return entity(error_t(std::move(msg)));
}

entity grammar_error(std::string const& err_msg) {
    return standard_error(std::string("Grammar Error: ") + err_msg);
}

/**
 * @brief The operation error. Indicate that the operation between such entities is not supported.
 * @param type The type of the primary entity (function type, or the type of the first operand).
 * @param arg_types The argument types (either the parameter types, or the type of the second operand).
 * @param op The operator name, or the name of the function.
 * @param err_msg Additional information about the error.
 * @return The error entity.
 */
entity operation_error(std::string const& type, std::vector<std::string> const& arg_types, std::string const& op, std::string const& err_msg) {
    auto sstrm = std::ostringstream();
    sstrm << "Operation Error: " << err_msg
          << "\n\twith primary object's type: " << type
          << "\n\tOperator: " << op
          << (not arg_types.empty() ? "\n\tArguments: " : "");
    for (auto i = 0uz; i < arg_types.size() - 1; ++i) {
        sstrm << arg_types[i] << ", ";
    }
    sstrm << arg_types.back();
    return standard_error(sstrm.str());
}

void throw_arithmetic_error(std::string const& err_msg) {
    throw error_t(err_msg);
}

void throw_standard_error(std::string msg) {
    throw error_t(std::move(msg));
}

void throw_grammar_error(std::string const& err_msg) {
    throw error_t(std::string("Grammar Error: ") + err_msg);
}

void throw_operation_error(std::string const& type, std::vector<std::string> const& arg_types, std::string const& op, std::string const& err_msg) {
    throw error_t(operation_error(type, arg_types, op, err_msg));
}

void tuple_t::data_deleter::operator ()(data_type* dat) const {
    delete dat;
}

tuple_t::tuple_t(tuple_t const& other) {
    *this = other;
}

template<typename InputIt>
tuple_t::tuple_t(InputIt first, InputIt last) : tuple_t() {
    auto* tupptr = this;
    while (first != last) {
        tupptr->data = data_ptr(new data_type(std::make_pair(*first++, tuple_t())));
        tupptr = &this->content().second;
    }
}

tuple_t tuple_t::concat(tuple_t const& other) const  {
    auto s = std::stack<entity>();
    auto* tupptr = this;
    while (!tupptr->empty()) {
        auto&& [head, tail] = tupptr->content();
        s.emplace(head);
        tupptr = &tail;
    }
    auto result = other;
    while (!s.empty()) {
        result.push(std::move(s.top()));
        s.pop();
    }
    return result;
}

tuple_t::content_type& tuple_t::content() const {
    return *data;
}

template<typename T>
void tuple_t::emplace(auto&&... args) {
    this->push(T(FWD(args)...));
}

void tuple_t::for_each(auto&& func) const {
    if (this->empty()) {
        return;
    }
    auto* tupptr = this;
    while (!tupptr->empty()) {
        auto&& [head, tail] = tupptr->content();
        func(head);
        tupptr = &tail;
    }
}

tuple_t& tuple_t::operator =(tuple_t const& other) {
    auto* tupptr = this;
    auto* othptr = &other;
    while (!othptr->empty()) {
        auto&& [head, tail] = othptr->content();
        tupptr->push(head);
        othptr = &tail;
    }
    return *this;
}

bool tuple_t::empty() const noexcept {
    return !data;
}

void tuple_t::push(entity const& elem) noexcept {
    auto tmp = elem;
    this->push(std::move(tmp));
}

void tuple_t::push(entity&& elem) noexcept {
    auto new_head = new data_type(std::make_pair(elem, tuple_t()));
    new_head->second.data = std::move(data);
    data = std::unique_ptr<data_type, data_deleter>(new_head);
}

list_t tuple_t::to_list() const {
    auto result = list_t();
    this->for_each([&result](auto&& item) {
        result.push_back(item);
    });
    return result;
}

entity tuple_t::value() const {
    return this->content().first;
}

void entity::value_deleter::operator ()(entity::value_type* dat) const {
    delete dat;
}

template<not_of<entity> T>
entity::entity(T&& value) {
    using enum type;
    using type = TYPE(value);

    m_value = std::unique_ptr<value_type, value_deleter>();

    if constexpr (std::is_integral_v<type>) {
        *m_value = int_t(value);
        m_type = INT;
    }
    else if constexpr (std::is_floating_point_v<type>) {
        *m_value = real_t(value);
        m_type = REAL;
    }
    else if constexpr (std::convertible_to<type, std::string>) {
        *m_value = str_t(value);
        m_type = STR;
    }
    else if constexpr (std::same_as<list_t, type>) {
        *m_value = list_t(FWD(value));
        m_type = LIST;
    }
    else if constexpr (std::same_as<tuple_t, type>) {
        *m_value = tuple_t(FWD(value));
        m_type = TUPLE;
    }
    else if constexpr (std::same_as<func_t, type>) {
        *m_value = func_t(FWD(value));
        m_type = FUNC;
    }
    else if constexpr (std::convertible_to<type, error_t>) {
        *m_value = error_t(value);
        m_type = ERROR;
    }
    else {
        *m_value = error_t("Unsupported type");
        m_type = ERROR;
    }
}

entity::entity(entity const& other)
    : m_value(new value_type(*other.m_value)), m_type(other.m_type) {}

std::string entity::name(type t) noexcept {
    switch (t) {
        case INT:   return "Int";
        case REAL:  return "Real";
        case STR:   return "Str";
        case LIST:  return "List";
        case TUPLE: return "Tuple";
        case FUNC:  return "Func";
        case ERROR: return "Error";
        default:    return "Unknown";
    }
}

template<typename T>
entity::type entity::of(T&&) noexcept {
    static auto type_map = std::unordered_map<std::type_index, entity::type> {
            { std::type_index(typeid(int_t)), INT },
            { std::type_index(typeid(real_t)), REAL },
            { std::type_index(typeid(str_t)), STR },
            { std::type_index(typeid(tuple_t)), TUPLE },
            { std::type_index(typeid(list_t)), LIST },
            { std::type_index(typeid(func_t)), FUNC },
            { std::type_index(typeid(error_t)), ERROR },
    };
    return type_map.at(std::type_index(typeid(T)));
}

entity& entity::operator =(entity const& other) {
    if (this == &other) {
        return *this;
    }
    m_type = other.m_type;
    // m_value = other.m_value;
    return *this;
}

bool operator ==(tuple_t const& lhs, tuple_t const& rhs) noexcept {
    auto* ltupptr = &lhs;
    auto* rtupptr = &rhs;
    while (!ltupptr->empty() && !rtupptr->empty()) {
        auto&& [lhead, ltail] = ltupptr->content();
        auto&& [rhead, rtail] = rtupptr->content();
        if (not (lhead == rhead)) {
            return false;
        }
        ltupptr = &ltail;
        rtupptr = &rtail;
    }
    return ltupptr->empty() && rtupptr->empty();
}

std::partial_ordering operator <=>(tuple_t const& lhs, tuple_t const& rhs) noexcept {
    auto* ltupptr = &lhs;
    auto* rtupptr = &rhs;
    while (!ltupptr->empty() && !rtupptr->empty()) {
        auto&& [lhead, ltail] = ltupptr->content();
        auto&& [rhead, rtail] = rtupptr->content();
        auto cmp = std::partial_ordering(lhead <=> rhead);
        if (cmp != std::partial_ordering::equivalent) {
            return cmp;
        }
        ltupptr = &ltail;
        rtupptr = &rtail;
    }
    if (ltupptr->empty() && rtupptr->empty()) {
        return std::partial_ordering::equivalent;
    }
    if (ltupptr->empty()) {
        return std::partial_ordering::less;
    }
    return std::partial_ordering::greater;
}

entity operator +(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](list_t const& arg_1, list_t const& arg_2) {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch"));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] + arg_2[i]);
                }
                return entity(result);
            },
            [](tuple_t const& arg_1, tuple_t const& arg_2) {
                return entity(arg_1.concat(arg_2));
            },
            [](auto&& arg_1, auto&& arg_2) {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return entity(arg_1 + arg_2);
                }
                else if constexpr (std::same_as<str_t, type_1> && std::same_as<str_t, type_2>) {
                    return entity(arg_1 + arg_2);
                }
                else if constexpr (arithmetic<type_1> && std::same_as<list_t, type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.push_back(entity(arg_1) + ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<list_t, type_1> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.push_back(ent + entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(+)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator -(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](list_t const& arg_1, list_t const& arg_2) {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] - arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return entity(arg_1 - arg_2);
                }
                else if constexpr (arithmetic<type_1> && std::same_as<type_2, list_t>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.emplace_back(entity(arg_1) - ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<type_1, list_t> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.emplace_back(ent - entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(-)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator *(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](int_t arg_1, str_t const& arg_2) -> entity {
                str_t result;
                while (arg_1-- > 0) {
                    result += arg_2;
                }
                return entity(result);
            },
            [](str_t const& arg_1, int_t arg_2) -> entity {
                str_t result;
                while (arg_2-- > 0) {
                    result += arg_1;
                }
                return entity(result);
            },
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] * arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return entity(arg_1 * arg_2);
                }
                else if constexpr (arithmetic<type_1> && std::same_as<type_2, list_t>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.emplace_back(entity(arg_1) * ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<type_1, list_t> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.emplace_back(ent * entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(*)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator /(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] / arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    if (arg_2 == 0) {
                        return entity(error_t("Division by zero."));
                    }
                    return entity(arg_1 / arg_2);
                }
                else if constexpr (arithmetic<type_1> && std::same_as<type_2, list_t>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.emplace_back(entity(arg_1) / ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<type_1, list_t> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.emplace_back(ent / entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(/)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator %(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](int_t arg_1, int_t arg_2) -> entity {
                if (arg_2 == 0) {
                    return entity(error_t("Division by zero."));
                }
                return entity(arg_1 % arg_2);
            },
            [](int_t arg_1, list_t const& arg_2) -> entity {
                auto result = list_t();
                for (auto&& ent : arg_2) {
                    result.emplace_back(entity(arg_1) % ent);
                }
                return entity(result);
            },
            [](list_t const& arg_1, int_t arg_2) -> entity {
                auto result = list_t();
                for (auto&& ent : arg_1) {
                    result.emplace_back(ent % entity(arg_2));
                }
                return entity(result);
            },
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] % arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(%)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator ^(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(std::pow(real_t(arg_1[i]), real_t(arg_2[i])));
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return entity(std::pow(arg_1, arg_2));
                }
                else if constexpr (arithmetic<type_1> && std::same_as<list_t, type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.emplace_back(entity(arg_1) ^ ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<type_1, list_t> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.emplace_back(ent ^ entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(^)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator &(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](int_t arg_1, int_t arg_2) -> entity {
                return entity(arg_1 & arg_2);
            },
            [](int_t arg_1, list_t const& arg_2) -> entity {
                auto result = list_t();
                for (auto&& ent : arg_2) {
                    result.emplace_back(entity(arg_1) & ent);
                }
                return entity(result);
            },
            [](list_t const& arg_1, int_t arg_2) -> entity {
                auto result = list_t();
                for (auto&& ent : arg_1) {
                    result.emplace_back(ent & entity(arg_2));
                }
                return entity(result);
            },
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] & arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(&)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator |(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](int_t arg_1, int_t arg_2) {
                return entity(arg_1 | arg_2);
            },
            [](int_t arg_1, list_t const& arg_2) {
                auto result = list_t();
                for (auto&& ent : arg_2) {
                    result.emplace_back(entity(arg_1) | ent);
                }
                return entity(result);
            },
            [](list_t const& arg_1, int_t arg_2) {
                auto result = list_t();
                for (const auto & ent : arg_1) {
                    result.emplace_back(ent | entity(arg_2));
                }
                return entity(result);
            },
            [](list_t const& arg_1, list_t const& arg_2) {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] | arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) {
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(|)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator <<(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](int_t arg_1, int_t arg_2) -> entity {
                return entity(arg_1 << arg_2);
            },
            [](int_t arg_1, list_t const& arg_2) -> entity {
                auto result = list_t();
                for (auto&& ent : arg_2) {
                    result.emplace_back(entity(arg_1) << ent);
                }
                return entity(result);
            },
            [](list_t const& arg_1, int_t arg_2) {
                auto result = list_t();
                for (auto&& ent : arg_1) {
                    result.emplace_back(ent << entity(arg_2));
                }
                return entity(result);
            },
            [](list_t const& arg_1, list_t const& arg_2) {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] << arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) {
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(<<)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator >>(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
        [](int_t arg_1, int_t arg_2) {
            return entity(arg_1 >> arg_2);
        },
        [](int_t arg_1, list_t const& arg_2) {
            auto result = list_t();
            for (auto&& ent : arg_2) {
                result.emplace_back(entity(arg_1) >> ent);
            }
            return entity(result);
        },
        [](list_t const& arg_1, int_t arg_2) {
            auto result = list_t();
            for (auto&& ent : arg_1) {
                result.emplace_back(ent >> entity(arg_2));
            }
            return entity(result);
        },
        [](list_t const& arg_1, list_t const& arg_2) {
            if (arg_1.size() != arg_2.size()) {
                return entity(error_t("List size mismatch."));
            }
            auto result = list_t();
            for (auto i = 0uz; i < arg_1.size(); ++i) {
                result.emplace_back(arg_1[i] >> arg_2[i]);
            }
            return entity(result);
        },
        [](auto&& arg_1, auto&& arg_2) {
            return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(>>)");
        }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator &&(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result[i] = arg_1[i] && arg_2[i];
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return entity(arg_1 && arg_2);
                }
                else if constexpr (arithmetic<type_1> && std::same_as<list_t, type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.emplace_back(entity(arg_1) && ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<type_1, list_t> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.emplace_back(ent && entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(&&)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator ||(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                if (arg_1.size() != arg_2.size()) {
                    return entity(error_t("List size mismatch."));
                }
                auto result = list_t();
                for (auto i = 0uz; i < arg_1.size(); ++i) {
                    result.emplace_back(arg_1[i] || arg_2[i]);
                }
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return entity(arg_1 || arg_2);
                }
                else if constexpr (arithmetic<type_1> && std::same_as<list_t, type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_2) {
                        result.emplace_back(entity(arg_1) || ent);
                    }
                    return entity(result);
                }
                else if constexpr (std::same_as<type_1, list_t> && arithmetic<type_2>) {
                    auto result = list_t();
                    for (auto&& ent : arg_1) {
                        result.emplace_back(ent || entity(arg_2));
                    }
                    return entity(result);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(||)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator !(entity const& arg) {
    return std::visit(overload {
            [](int_t arg_1) -> entity {
                return entity(!arg_1);
            },
            [](list_t const& arg_1) -> entity {
                auto result = list_t();
                for (auto&& ent : arg_1) {
                    result.emplace_back(!ent);
                }
                return entity(result);
            },
            [](auto&& arg_1) -> entity {
                return operation_error(entity::name_of(arg_1), { std::string("empty") }, "(!)");
            }
    }, *arg.m_value);
}

bool operator ==(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](auto&& arg_1, auto&& arg_2) -> bool {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (arithmetic<type_1> && arithmetic<type_2>) {
                    return arg_1 == arg_2;
                }
                else if constexpr (std::same_as<func_t, type_1> && std::same_as<func_t, type_2>) {
                    return false;
                }
                else if constexpr (std::same_as<error_t, type_1> && std::same_as<error_t, type_2>) {
                    return false;
                }
                else if constexpr (std::same_as<type_1, type_2>) {
                    return arg_1 == arg_2;
                }
                throw std::runtime_error("Type mismatch.");
            }
    }, *lhs.m_value, *rhs.m_value);
}

std::partial_ordering operator <=>(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
        [](list_t const& arg_1, list_t const& arg_2) {
            auto result = list_t();
            for (auto i = 0uz; i < arg_1.size(); ++i) {
                result.emplace_back(arg_1[i] <=> arg_2[i]);
            }
            return result;
        },
        [](func_t const& arg_1, func_t const& arg_2) {
            if (&arg_1 == &arg_2) {
                return std::partial_ordering::equivalent;
            }
            return std::partial_ordering::unordered;
        },
        [](error_t const& arg_1, error_t const& arg_2) {
            if (&arg_1 == &arg_2) {
                return std::partial_ordering::equivalent;
            }
            return std::partial_ordering::unordered;
        },
        [](auto&& arg_1, auto&& arg_2) -> std::partial_ordering {
            using type_1 = TYPE(arg_1);
            using type_2 = TYPE(arg_2);
            if constexpr (std::same_as<type_1, type_2> && requires { std::declval<type_1>() <=> std::declval<type_2>(); }) {
                return arg_1 <=> arg_2;
            }
            throw std::runtime_error("Type mismatch.");
        }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator_abstract(entity const& lhs, entity const& rhs) {
    auto& arg_name = std::get<str_t>(*lhs.m_value);
    auto& body = std::get<str_t>(*rhs.m_value);
    return entity(func_t([key = std::move(arg_name), value = std::move(body)](entity arg) {
        return entity(0);
    }));
}

entity operator_apply(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](auto&& arg_1, auto&& arg_2) {
                using type_1 = TYPE(arg_1);
                if constexpr (std::same_as<func_t, type_1>) {
                    return arg_1(entity(arg_2));
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "($)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator_concat(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](str_t const& arg_1, str_t const& arg_2) -> entity {
                return entity(arg_1 + arg_2);
            },
            [](list_t const& arg_1, list_t const& arg_2) -> entity {
                auto result = list_t(arg_2);
                result.insert(result.end(), arg_1.begin(), arg_1.end());
                return entity(result);
            },
            [](tuple_t const& arg_1, tuple_t const& arg_2) -> entity {
                return entity(arg_1.concat(arg_2));
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(++)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator_compare(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](auto&& arg_1, auto&& arg_2) {
                using type_1 = TYPE(arg_1);
                using type_2 = TYPE(arg_2);
                if constexpr (std::same_as<func_t, type_1> || std::same_as<func_t, type_2>) {
                    return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(<=>)");
                }
                else if constexpr (std::same_as<error_t, type_1> || std::same_as<error_t, type_2>) {
                    return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(<=>)");
                }
                else if constexpr (std::same_as<list_t, type_1> && std::same_as<list_t, type_2>) {
                    for (auto i = 0uz; i < arg_1.size() && i < arg_2.size(); ++i) {
                        auto cmp = std::partial_ordering(arg_1[i] <=> arg_2[i]);
                        if (cmp != std::partial_ordering::equivalent) {
                            return entity(cmp);
                        }
                    }
                    return entity(arg_1.size() <=> arg_2.size());
                }
                else if constexpr (std::same_as<type_1, type_2>) {
                    return entity(arg_1 <=> arg_2);
                }
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(<=>)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator_cons(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](auto&& arg_1, list_t const& arg_2) -> entity {
                auto result = list_t(arg_2);
                result.emplace_back(arg_1);
                return entity(result);
            },
            [](auto&& arg_1, auto&& arg_2) -> entity {
                return operation_error(entity::name_of(arg_1), { entity::name_of(arg_2) }, "(:)");
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity operator_zip(entity const& lhs, entity const& rhs) {
    return std::visit(overload {
            [](auto&& arg_1, auto&& arg_2) {
                using type_2 = TYPE(arg_2);
                auto result = tuple_t();
                if constexpr (!std::same_as<tuple_t, type_2>) {
                    result.emplace(arg_2);
                }
                else {
                    result = arg_2;
                }
                result.emplace(arg_1);
                return entity(result);
            }
    }, *lhs.m_value, *rhs.m_value);
}

entity::operator bool() const {
    return std::visit(overload {
            [](int_t arg) -> bool {
                return arg != 0;
            },
            [](real_t arg) -> bool {
                return arg != 0.0;
            },
            [](str_t const& arg) -> bool {
                return !arg.empty();
            },
            [](list_t const& arg) -> bool {
                return !arg.empty();
            },
            [](tuple_t const& arg) -> bool {
                return !arg.empty();
            },
            [](func_t const& arg) -> bool {
                return bool(arg);
            },
            [](auto&& arg) -> bool {
                return true;
            }
    }, *m_value);
}

entity::operator int_t() const {
    return std::visit(overload {
            [](int_t arg) -> int_t {
                return arg;
            },
            [](real_t arg) -> int_t {
                return int_t(arg);
            },
            [](str_t const& arg) -> int_t {
                int_t result;
                std::from_chars(arg.data(), arg.data() + arg.size(), result);
                return result;
            },
            [](auto&& arg) -> int_t {
                throw std::runtime_error("Invalid operation.");
            }
    }, *m_value);
}

entity::operator real_t() const {
    return std::visit(overload {
            [](int_t arg) -> real_t {
                return real_t(arg);
            },
            [](real_t arg) -> real_t {
                return arg;
            },
            [](str_t const& arg) -> real_t {
                real_t result;
                std::from_chars(arg.data(), arg.data() + arg.size(), result);
                return result;
            },
            [](auto&& arg) -> real_t {
                throw std::runtime_error("Invalid operation.");
            }
    }, *m_value);
}

entity::operator str_t() const {
    return std::visit(overload {
            [](int_t arg) -> str_t {
                return std::to_string(arg);
            },
            [](real_t arg) -> str_t {
                return std::to_string(arg);
            },
            [](str_t const& arg) -> str_t {
                return arg;
            },
            [](auto&& arg) -> str_t {
                throw std::runtime_error("Invalid operation.");
            }
    }, *m_value);
}

entity::operator list_t() const {
    return std::visit(overload {
            [](list_t const& arg) -> list_t {
                return arg;
            },
            [](tuple_t const& arg) -> list_t {
                return arg.to_list();
            },
            [](auto&& arg) -> list_t {
                list_t result;
                result.emplace_back(arg);
                return result;
            }
    }, *m_value);
}

entity::operator tuple_t() const {
    return std::visit(overload {
            [](tuple_t const& arg) -> tuple_t {
                return arg;
            },
            [](list_t const& arg) -> tuple_t {
                tuple_t result;
                for (auto const& item : arg) {
                    result.push(item);
                }
                return result;
            },
            [](auto&& arg) -> tuple_t {
                tuple_t result;
                result.emplace(arg);
                return result;
            }
    }, *m_value);
}

entity::operator func_t() const {
    return std::visit(overload {
            [](func_t const& arg) -> func_t {
                return arg;
            },
            [](auto&& arg) -> func_t {
                return func_t([arg] (entity const&) {
                    return entity(arg);
                });
            }
    }, *m_value);
}

entity::operator error_t() const {
    return std::visit(overload {
            [](error_t const& arg) {
                return arg;
            },
            [](auto&& arg) {
                return error_t("Invalid operation.");
            }
    }, *m_value);
}

entity::operator std::partial_ordering() const {
    return std::visit(overload {
            [](int_t arg) {
                return arg > 0 ? std::partial_ordering::greater : arg < 0 ? std::partial_ordering::less : std::partial_ordering::equivalent;
            },
            [this](auto&& arg) -> std::partial_ordering {
                throw_operation_error(entity::name_of(*this), {}, "(std::strong_ordering)");
            }
    }, *m_value);
}

template<not_of<entity> T>
entity::operator T() const {
    return std::visit(overload {
            [](T const& arg) -> T {
                return arg;
            },
            [](auto&& arg) -> T {
                throw_operation_error(entity::name_of(arg), {}, "(T)");
            }
    }, *m_value);
}

template<contained_by<typename entity::value_type> T>
T& entity::get() {
    if (not std::holds_alternative<T>(m_value)) {
        throw std::runtime_error("Wrong type!");
    }
    return std::get<T>(m_value);
}

template<typename T>
bool entity::is() const noexcept {
    switch (m_type) {
        case INT:   return std::same_as<T, int_t>;
        case REAL:  return std::same_as<T, real_t>;
        case STR:   return std::same_as<T, str_t>;
        case LIST:  return std::same_as<T, list_t>;
        case TUPLE: return std::same_as<T, tuple_t>;
        case FUNC:  return std::same_as<T, func_t>;
        case ERROR: return std::same_as<T, error_t>;
        default:    return false;
    }
}


}
