#pragma once

#include "packutils.hpp"

namespace ysh {

template<typename F, std::size_t... Is>
class lambda;

template<typename F, std::size_t... Is>
class lambda {
public:
    constexpr lambda(F&& func)
        : m_func(func) {}  // This cannot be constexpr yet (in C++23).

    constexpr auto operator ()(auto&&... args) const {
        return std::any_cast<F>(m_func)(get<Is>(FWD(args)...)...);
    }

private:
    std::any m_func;
};

template<std::size_t... Is>
auto make_lambda(auto&& f) {
    return lambda<decltype(f), Is...>(FWD(f));
}

template<typename Op, typename FA, std::size_t... Is, typename FB, std::size_t... Js>
auto binary_operator(Op&& op, lambda<FA, Is...> const& a, lambda<FB, Js...> const& b) {
    return make_lambda<Is..., Js...>([&op, &a, &b](auto&&... args) {
        auto&& a_result = a(get<Is>(FWD(args)...)...);
        auto&& b_result = b(get<Js>(FWD(args)...)...);
        return op(a_result, b_result);
    });
}

template<typename Op, typename F, std::size_t... Is>
auto unary_operator(Op&& op, lambda<F, Is...> const& a) {
    return make_lambda<Is...>([&op, &a](auto&& ... args) {
        auto&& a_result = a(get<Is>(FWD(args)...)...);
        return op(a_result);
    });
}

template<typename FA, std::size_t... Is, typename FB, std::size_t... Js>
auto operator +(lambda<FA, Is...> const& a, lambda<FB, Js...> const& b) {
    return binary_operator(std::plus(), a, b);
}

template<typename FA, std::size_t... Is, typename FB, std::size_t... Js>
auto operator -(lambda<FA, Is...> const& a, lambda<FB, Js...> const& b) {
    return binary_operator(std::minus(), a, b);
}

template<typename FA, std::size_t... Is, typename FB, std::size_t... Js>
auto operator *(lambda<FA, Is...> const& a, lambda<FB, Js...> const& b) {
    return binary_operator(std::multiplies(), a, b);
}

template<typename FA, std::size_t... Is, typename FB, std::size_t... Js>
auto operator /(lambda<FA, Is...> const& a, lambda<FB, Js...> const& b) {
    return binary_operator(std::divides(), a, b);
}

template<typename FA, std::size_t... Is, typename FB, std::size_t... Js>
auto operator %(lambda<FA, Is...> const& a, lambda<FB, Js...> const& b) {
    return binary_operator(std::modulus(), a, b);
}

namespace placeholders {

template<std::size_t I = std::numeric_limits<std::size_t>::max()>
struct proto_placeholder : public lambda<proto_placeholder<I>, I> {
    static constexpr std::size_t index = I;

    constexpr proto_placeholder()
        : lambda<proto_placeholder<I>, I>(*this) {}

    [[nodiscard]]
    constexpr bool wildcard() const {
        return I == std::numeric_limits<std::size_t>::max();
    }
};

template<std::size_t I>
auto make_lambda(proto_placeholder<I> const&) {
    return lambda<proto_placeholder<I>, I>();
}

#define DECLARE_PLACEHOLDER(N, name) \
    ::ysh::placeholders::proto_placeholder<N> $ ## name;

#define DECLARE_RESERVED_PLACEHOLDER(N) \
    DECLARE_PLACEHOLDER(N, N)

DECLARE_RESERVED_PLACEHOLDER(0);
DECLARE_RESERVED_PLACEHOLDER(1);
DECLARE_RESERVED_PLACEHOLDER(2);
DECLARE_RESERVED_PLACEHOLDER(3);
DECLARE_RESERVED_PLACEHOLDER(4);
DECLARE_RESERVED_PLACEHOLDER(5);
DECLARE_RESERVED_PLACEHOLDER(6);
DECLARE_RESERVED_PLACEHOLDER(7);
DECLARE_RESERVED_PLACEHOLDER(8);
DECLARE_RESERVED_PLACEHOLDER(9);

#undef DECLARE_RESERVED_PLACEHOLDER

} // namespace placeholders

} // namespace ysh