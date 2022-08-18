#pragma once
#include "prelude.hpp"

namespace ysh {

template<typename T>
concept arithmetic = std::is_arithmetic_v<T>;

namespace types {

class entity;

/**
 * @brief The error value type in ysh. Basically a trivial wrapper for std::string.
 */
struct error_t : public std::exception {
    std::string msg;

    explicit error_t(std::string str)
        : msg(std::move(str)) {}

    error_t() = default;
    error_t(error_t const&) = default;
    error_t(error_t&&) noexcept = default;

    error_t& operator =(error_t const&) = default;
    error_t& operator =(error_t&&) noexcept = default;

    [[nodiscard]]
    char const* what() const noexcept override {
        return msg.c_str();
    }
};

entity arithmetic_error(std::string const& err_msg);

entity standard_error(std::string msg);

entity grammar_error(std::string const& err_msg);

entity operation_error(std::string const& type, std::vector<std::string> const& arg_types, std::string const& op, std::string const& err_msg = "");

[[noreturn]]
void throw_arithmetic_error(std::string const& err_msg);

[[noreturn]]
void throw_standard_error(std::string msg);

[[noreturn]]
void throw_grammar_error(std::string const& err_msg);

[[noreturn]]
void throw_operation_error(std::string const& type, std::vector<std::string> const& arg_types, std::string const& op, std::string const& err_msg = "");



using func_t = std::function<entity (entity)>;

using int_t = long long;

using list_t = std::vector<entity>;

using real_t = long double;

using str_t = std::string;

/**
 * @brief The tuple type in ysh.
 * type tuple = empty | (entity, tuple)
 * Since this is a recursive type, we have to use std::unique_ptr to redirect the memory.
 * @tparam content_type i.e. (entity, tuple), the type of a non-empty tuple.
 * @tparam data_type i.e. empty | (entity, tuple), the type of a general tuple, only constructible from empty or another
 * tuple.
 */
struct tuple_t {
    using content_type = std::pair<entity, tuple_t>;
    using data_type = content_type;

    struct data_deleter {
        void operator ()(data_type* data) const;
    };

    using data_ptr = std::unique_ptr<data_type, data_deleter>;

    data_ptr data;

    tuple_t() noexcept
        : data(nullptr) {}

    template<typename InputIt>
    tuple_t(InputIt first, InputIt last);

    tuple_t(tuple_t const& other);

    tuple_t(tuple_t&& other) noexcept = default;

    /**
     * @brief Concatenate two tuples.
     * (h1, t1...) + (h2, t2...) -> (h1, t1..., h2, t2...)
     * @param other
     * @return
     */
    [[nodiscard]]
    tuple_t concat(tuple_t const& other) const;

    [[nodiscard]]
    content_type& content() const;

    void for_each(auto&& func) const;

    template<typename T = entity>
    void emplace(auto&&... args);

    [[nodiscard]]
    bool empty() const noexcept;

    tuple_t& operator =(tuple_t const& other);

    tuple_t& operator =(tuple_t&& other) noexcept = default;

    friend bool operator ==(tuple_t const& lhs, tuple_t const& rhs) noexcept;

    friend std::partial_ordering operator <=>(tuple_t const& lhs, tuple_t const& rhs) noexcept;

    void push(entity const& elem) noexcept;

    void push(entity&& elem) noexcept;

    [[nodiscard]]
    list_t to_list() const;

    [[nodiscard]]
    entity value() const;
};

/**
 * @brief The entity type in ysh.
 * type entity = int | real | str | func | list | tuple | error;
 */
class entity {
public:
    using value_type = std::variant<int_t, real_t, str_t, list_t, tuple_t, func_t, error_t>;

    enum type {
        INT, REAL, STR, LIST, TUPLE, FUNC, ERROR
    };

    struct value_deleter {
        void operator ()(value_type* value) const;
    };

    using value_ptr = std::unique_ptr<value_type, value_deleter>;

    entity() = default;

    template<not_of<entity> T>
    explicit entity(T&& value);

    entity(entity const& other);

    entity(entity&& other) noexcept = default;

    ~entity() = default;

    template<contained_by<value_type> T>
    T& get();

    template<typename T>
    [[nodiscard]]
    bool is() const noexcept;

    entity& operator =(entity const& other);

    entity& operator =(entity&& other) noexcept = default;

    friend entity operator +(entity const& lhs, entity const& rhs);

    friend entity operator -(entity const& lhs, entity const& rhs);

    friend entity operator *(entity const& lhs, entity const& rhs);

    friend entity operator /(entity const& lhs, entity const& rhs);

    friend entity operator %(entity const& lhs, entity const& rhs);

    friend entity operator ^(entity const& lhs, entity const& rhs);

    friend entity operator &(entity const& lhs, entity const& rhs);

    friend entity operator |(entity const& lhs, entity const& rhs);

    friend entity operator <<(entity const& lhs, entity const& rhs);

    friend entity operator >>(entity const& lhs, entity const& rhs);

    friend entity operator &&(entity const& lhs, entity const& rhs);

    friend entity operator ||(entity const& lhs, entity const& rhs);

    friend entity operator !(entity const& arg);

    friend bool operator ==(entity const& lhs, entity const& rhs);

    friend std::partial_ordering operator <=>(entity const& lhs, entity const& rhs);

    friend entity operator_abstract(entity const& lhs, entity const& rhs);

    friend entity operator_apply(entity const& lhs, entity const& rhs);

    friend entity operator_concat(entity const& lhs, entity const& rhs);

    friend entity operator_cons(entity const& lhs, entity const& rhs);

    friend entity operator_compare(entity const& lhs, entity const& rhs);

    friend entity operator_zip(entity const& lhs, entity const& rhs);

    explicit operator bool() const;

    explicit operator int_t() const;

    explicit operator real_t() const;

    explicit operator str_t() const;

    explicit operator list_t() const;

    explicit operator tuple_t() const;

    explicit operator func_t() const;

    explicit operator error_t() const;

    explicit operator std::partial_ordering() const;

    template<not_of<entity> T>
    explicit operator T() const;

    static std::string name(type t) noexcept;

    template<typename T>
    static std::string name_of(T&& arg) noexcept {
        return name(of(arg));
    }

    template<typename T>
    static type of(T&& arg) noexcept;

private:
    value_ptr m_value;
    type m_type;
};
} // namespace types

// Bring class entity out of the types namespace.
using entity_t = types::entity;

} // namespace ysh