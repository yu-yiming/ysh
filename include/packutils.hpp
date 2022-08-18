#pragma once

#include "prelude.hpp"

namespace ysh {

namespace detail {

template<std::size_t IBegin, std::size_t IEnd, typename Pack, typename Result>
struct pack_range;

template<std::size_t... Is, std::size_t... Js>
struct pack_range<0, 0, std::index_sequence<Is...>, std::index_sequence<Js...>> {
    using type = std::index_sequence<Js...>;
};

template<std::size_t IEnd, std::size_t I, std::size_t... Is, std::size_t... Js>
struct pack_range<0, IEnd, std::index_sequence<I, Is...>, std::index_sequence<Js...>> {
    using type = typename pack_range<0, IEnd - 1, std::index_sequence<Is...>, std::index_sequence<Js..., I>>::type;
};

template<std::size_t IBegin, std::size_t IEnd, std::size_t I, std::size_t... Is, typename Result>
struct pack_range<IBegin, IEnd, std::index_sequence<I, Is...>, Result> {
    using type = typename pack_range<IBegin - 1, IEnd - 1, std::index_sequence<Is...>, std::index_sequence<>>::type;
};

}

template<typename Pack>
struct pack_tail;

template<typename Head, typename... Tail>
struct pack_tail<std::tuple<Head, Tail...>> {
    using type = std::tuple<Tail...>;
};

template<typename T, T First, T... Rest>
struct pack_tail<std::integer_sequence<T, First, Rest...>> {
    using type = std::integer_sequence<T, Rest...>;
};

template<std::size_t IBegin, std::size_t IEnd, typename Pack>
struct pack_range : detail::pack_range<IBegin, IEnd, Pack, std::index_sequence<>> {};


template<std::size_t I, typename... Ts>
constexpr auto get(Ts&&... args) {
    return std::get<I>(std::forward_as_tuple(FWD(args)...));
}

template<std::size_t... Is, typename... Ts>
constexpr auto pick(Ts&&... args) {
    return std::forward_as_tuple(get<Is>(FWD(args))...);
}

template<std::size_t... Is, typename... Ts>
constexpr auto pick(std::index_sequence<Is...>, Ts&&... args) {
    return std::forward_as_tuple(get<Is>(FWD(args))...);
}

template<std::size_t... Is, typename F, typename... Ts>
constexpr auto apply_picked(F&& f, Ts&&... args) {
    return f(std::get<Is>(FWD(args))...);
}

template<std::size_t... Is, typename F, typename... Ts>
constexpr auto apply_picked(std::index_sequence<Is...>, F&& f, Ts&&... args) {
    return f(std::get<Is>(FWD(args))...);
}

template<std::size_t IBegin, std::size_t IEnd, typename... Ts>
constexpr auto apply_range(Ts&&... args) {

}

template<typename... Ts>
constexpr auto head(Ts&&... args) {
    static_assert(sizeof...(args) > 0, "first: no arguments");
    return get<0>(FWD(args)...);
}

template<typename... Ts>
constexpr auto tail(Ts&&... args) {
    static_assert(sizeof...(args) > 0, "rest: no arguments");
    using full_indices = std::make_index_sequence<sizeof...(args)>;
    using tail_indices = typename pack_tail<full_indices>::type;
    return choose(tail_indices(), FWD(args)...);
}

template<typename F, typename... Ts>
constexpr auto apply_tail(F&& f, Ts&&... args) {
    return f(FWD(args)...);
}

template<std::size_t IBegin, std::size_t IEnd, typename IPack>
struct index_range_impl;

template<std::size_t IEnd, std::size_t... Is>
struct index_range_impl<IEnd, IEnd, std::index_sequence<Is...>> {
    using type = std::index_sequence<Is...>;
};

template<std::size_t IBegin, std::size_t IEnd, std::size_t... Is>
struct index_range_impl<IBegin, IEnd, std::index_sequence<Is...>> {
    using type = typename index_range_impl<IBegin + 1, IEnd, std::index_sequence<Is..., IBegin>>::type;
};

template<std::size_t IBegin, std::size_t IEnd>
struct index_range {
    using type = typename index_range_impl<IBegin, IEnd, std::index_sequence<>>::type;
};

template<typename... Ts, std::size_t... Is>
constexpr auto split_pack_impl(std::index_sequence<Is...>, Ts&&... args) {
    return std::make_tuple(get<Is>(FWD(args))...);
}

template<std::size_t IBegin, std::size_t IEnd, typename... Ts>
constexpr auto split_pack(Ts&& ... args) {
    using pack_indices = typename index_range<IBegin, IEnd>::type;
    return split_pack_impl(pack_indices {}, FWD(args)...);
}

template<typename F, typename... Ts, std::size_t... Is>
constexpr auto apply_pack_impl(std::index_sequence<Is...>, F&& f, Ts&&... args) {
    return f(std::get<Is>(std::make_tuple(FWD(args)...))...);
}

template<std::size_t IBegin, std::size_t IEnd, typename F, typename... Ts>
constexpr auto apply_pack(F&& f, Ts&&... args) {
    using pack_indices = typename index_range<IBegin, IEnd>::type;
    return apply_pack_impl(pack_indices {}, f, FWD(args)...);
}


} // namespace ysh