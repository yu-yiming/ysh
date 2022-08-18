#pragma once

#include <algorithm>
#include <any>
#include <array>
#include <bitset>
#include <charconv>
#include <chrono>
#include <cmath>
#include <compare>
#include <complex>
#include <concepts>
#include <coroutine>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <experimental/propagate_const>
#include <filesystem>
#include <fmt/format.h>
#include <forward_list>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <numbers>
#include <numeric>
#include <optional>
#include <queue>
#include <random>
#include <ranges>
#include <source_location>
#include <span>
#include <sstream>
#include <stack>
#include <string>
#include <string_view>
#include <sys/wait.h>
#include <sys/stat.h>
#include <thread>
#include <tuple>
#include <typeindex>
#include <type_traits>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

#ifdef __GNUG__
#include <cxxabi.h>
#endif

#define FWD(x) ::std::forward<decltype(x)>(x)
#define TYPE(x) std::decay_t<decltype(x)>

namespace stdf = ::std::filesystem;
namespace stdr = ::std::ranges;
namespace stdv = ::std::views;

namespace detail {

template<typename T, typename Variant>
struct is_contained_by;

template<typename T>
struct is_contained_by<T, std::variant<>>
        : std::false_type {};

template<typename T, typename... Tail>
struct is_contained_by<T, std::variant<T, Tail...>>
        : std::true_type {};

template<typename T, typename Head, typename... Tail>
struct is_contained_by<T, std::variant<Head, Tail...>>
        : is_contained_by<T, std::variant<Tail...>> {};

} // namespace detail

template<typename T, typename U>
concept not_of = !std::same_as<T, std::decay_t<U>>;

template<typename F, typename T>
concept returning = requires(F f) {
    { f() } -> std::convertible_to<T>;
};

template<typename T, typename Variant>
concept contained_by = detail::is_contained_by<T, Variant>::value;

template<typename T>
class generator {
public:
    struct promise_type {
        int value;

        std::suspend_always initial_suspend() {
            return {};
        }

        std::suspend_always final_suspend() {
            return {};
        }

        generator get_return_object() {
            return { handle_type::from_promise(*this) };
        }

        void unhandled_exception() {
            throw;
        }

        void return_void() {}

        std::suspend_always yield_value(int val) {
            value = val;
            return {};
        }
    };

    using handle_type = std::coroutine_handle<promise_type>;

private:
    generator(handle_type h)
        : m_handle(h) {}

    handle_type m_handle;
};

template<typename T, template<typename> class Cont>
class nesting_container_iterator;

template<typename T, template<typename> class Cont>
class nesting_container_const_iterator;

template<typename T, template<typename> class Cont>
class nesting_container {
public:
    using element_type = T;
    using reference = T&;
    using const_reference = T const&;
    using value_type = std::variant<element_type, nesting_container<T, Cont>>;
    using container_type = Cont<value_type>;
    using iterator = nesting_container_iterator<T, Cont>;
    using const_iterator = nesting_container_iterator<T, Cont>;

    friend class nesting_container_iterator<T, Cont>;

    friend class nesting_container_const_iterator<T, Cont>;

    explicit nesting_container(Cont<T> const& cont);

    template<typename InputIt>
    nesting_container(InputIt first, InputIt last)
        : m_size(std::distance(first, last)) {
        m_container.reserve(m_size);
        std::copy(first, last, std::back_inserter(m_container));
    }

    nesting_container(std::initializer_list<T> const& list)
        : nesting_container(list.begin(), list.end()) {}

    nesting_container(nesting_container const& other)
        : nesting_container(other.begin(), other.end()) {}

    nesting_container(nesting_container&& other) noexcept = default;

    T& back() {
        return const_cast<T&>(static_cast<nesting_container const&>(*this).back());
    }

    T const& back() const {
        auto* ptr = &m_container->back();
        while (std::holds_alternative<nesting_container<T, Cont>>(*ptr)) {
            ptr = &std::get<nesting_container<T, Cont>>(*ptr).back();
        }
        return *ptr;
    }

    iterator begin();

    iterator begin() const;

    iterator end();

    iterator end() const;

    /**
     * @brief Convert the nesting container into a single, flat container.
     * @example
     * @code [1, [2, 3], [4, [5, 6]]] -> [1, 2, 3, 4, 5, 6] @endcode
     */
    Cont<T> flatten() const& {
        auto result = Cont<T>();
        if constexpr (requires { result.reserve(m_size); }) {
            result.reserve(m_size);
        }
        for (auto&& elem : *this) {
            if constexpr (std::holds_alternative<element_type>(elem)) {
                result.push_back(elem);
            }
            else {
                auto cont = std::move(std::get<nesting_container>(elem)).flatten();
                result.insert(result.end(), cont.begin(), cont.end());
            }
            result.push_back(elem);
        }
        return result;
    }

    Cont<T> flatten() && {
        auto result = Cont<T>();
        if constexpr (requires { result.reserve(m_size); }) {
            result.reserve(m_size);
        }
        for (auto&& elem : *this) {
            if constexpr (std::holds_alternative<element_type>(elem)) {
                result.push_back(std::move(elem));
            }
            else {
                auto cont = std::move(std::get<nesting_container>(elem)).flatten();
                result.insert(result.end(), std::make_move_iterator(cont.begin()), std::make_move_iterator(cont.end()));
            }
            result.push_back(std::move(elem));
        }
        return result;
    }

    /**
     * @brief Flatten the nesting container at a single position.
     * @example
     * @code [1, [2, 3], [4, [5, 6]]] -(idx = 2)-> [1, [2, 3], 4, 5, 6] @endcode
     */
    void flatten_at(std::size_t idx) {
        if (std::holds_alternative<element_type>(m_container->at(idx))) {
            return;
        }
        auto cont = std::move(std::get<nesting_container>(m_container->at(idx))).flatten();
        m_container->insert(std::advance(m_container->begin(), idx), std::make_move_iterator(cont.begin()), std::make_move_iterator(cont.end()));
        m_container->erase(std::advance(m_container->begin(), idx + cont.size()));
    }

    T& front() {
        return *this->begin();
    }

    T const& front() const {
        return *this->begin();
    }

    iterator insert(const_iterator pos, value_type const& value) {
        auto dist = std::distance(m_container->begin(), m_container->insert(pos, value));
        return std::advance(this->begin(), dist);
    }

    template<typename InputIt>
    iterator insert(const_iterator pos, InputIt first, InputIt last) {
        auto dist = std::distance(m_container->begin(), m_container->insert(pos, first, last));
        return std::advance(this->begin(), dist);
    }

    nesting_container& operator =(nesting_container const& other) {
        m_container = std::make_unique<container_type>(other.m_container->begin(), other.m_container->end());
        m_size = other.m_size;
    }

    nesting_container& operator =(nesting_container&& other) noexcept = default;

    T& operator [](std::size_t idx) {
        return const_cast<T&>(static_cast<nesting_container const&>(*this)[idx]);
    }

    T const& operator [](std::size_t idx) const {
        for (auto& elem : m_container) {
            if (std::holds_alternative<nesting_container>(elem) &&
                    std::get<nesting_container>(elem).size() > idx) {
                return std::get<nesting_container>(elem)[idx];
            }
            else if (std::holds_alternative<element_type>(elem)) [[likely]] {
                if (idx == 0) {
                    return std::get<element_type>(elem);
                }
                else {
                    --idx;
                }
            }
        }
        throw std::out_of_range("nesting_container::operator[]");
    }

    void push_back(T const& elem) {
        m_container.push_back(elem);
        ++m_size;
    }

    void push_back(T&& elem) {
        m_container.push_back(std::move(elem));
        ++m_size;
    }

    [[nodiscard]]
    std::size_t size() const noexcept {
        return m_size;
    }

private:
    using auxiliary_iterator = typename container_type::iterator;

    auxiliary_iterator aux_begin() {
        return m_container->begin();
    }

    auxiliary_iterator aux_end() {
        return m_container->end();
    }

    std::unique_ptr<container_type> m_container;
    std::size_t m_size{};
};

template<typename T, template<typename> class Cont>
class nesting_container_iterator {
public:
    /**
     * @brief The actual type of the elements in the container.
     */
    using element_type = T;

    /**
     * @brief The sum type of element_type and nesting_container.
     */
    using value_type = typename nesting_container<T, Cont>::value_type;

    /**
     * @brief The iterator over objects of element_type.
     */
    using element_iterator = typename Cont<T>::iterator;

    /**
     * @brief The iterator over objects of value_type.
     */
    using value_iterator = typename nesting_container<T, Cont>::container_type::iterator;

    struct impl_type {
        value_iterator it;
        std::stack<value_iterator> curr;
        std::stack<value_iterator> curr_end;
    };

    nesting_container_iterator(nesting_container<T, Cont>& cont, std::size_t idx);

    nesting_container_iterator(nesting_container_iterator const& other)
        : m_pimpl(std::make_unique<impl_type>(*m_pimpl)) {}

    nesting_container_iterator(nesting_container_iterator&& other) noexcept = default;

    nesting_container_iterator& operator =(nesting_container_iterator const& other) {
        m_pimpl = std::make_unique<impl_type>(*m_pimpl);
    }

    nesting_container_iterator& operator =(nesting_container_iterator&& other) noexcept = default;

    T& operator *() {
        return std::get<T>(*m_pimpl->it);
    }

    T* operator ->() {
        return &std::get<T>(*m_pimpl->it);
    }

    /**
     * @brief Increments the iterator; move to the next element.
     * @return
     */
    nesting_container_iterator& operator ++() {
        auto& impl = *m_pimpl;

        while (not impl.curr.empty()) {
            ++impl.it;
            // Great if it's not the end of the current nested container.
            if (impl.it != impl.curr_end.top()) {
                while (is_container(impl.it)) {
                    auto& curr_cont = get_container(impl.it);
                    impl.curr.push(impl.it);
                    impl.curr_end.push(curr_cont.aux_end());
                    impl.it = curr_cont.aux_begin();
                }
                return *this;
            }
            impl.curr_end.pop();
            impl.it = std::move(impl.curr.top());
            impl.curr.pop();
        }
        return *this;
    }

    nesting_container_iterator operator ++(int) & {
        auto result = *this;
        ++*this;
        return result;
    }

    nesting_container_iterator& operator --() {
        auto& impl = *m_pimpl;

        while (not impl.curr.empty() && impl.it == get_container(impl.curr.top()).aux_begin()) {
            impl.curr_end.pop();
            impl.curr.pop();
            impl.it = impl.curr_end.top();
            --impl.it;
            while (is_container(impl.it)) {
                auto& curr_cont = get_container(impl.it);
                impl.curr.push(impl.it);
                impl.curr_end.push(curr_cont.aux_end());
                impl.it = curr_cont.aux_end();
                --impl.it;
            }
        }
    }

    nesting_container_iterator operator --(int) & {
        auto result = *this;
        --*this;
        return result;
    }

    friend bool operator ==(nesting_container_iterator const& lhs, nesting_container_iterator const& rhs) {
        return lhs.m_it == rhs.m_it;
    }

private:
    static nesting_container<T, Cont>& get_container(value_iterator const& it) {
        return std::get<nesting_container<T, Cont>>(*it);
    }

    static bool is_container(value_iterator const& it) {
        return std::holds_alternative<value_type>(*it);
    }

    std::unique_ptr<impl_type> m_pimpl;
};


template<typename... Ts>
struct overload : Ts... {
    using Ts::operator ()...;
};

template<typename... Ts>
overload(Ts...) -> overload<Ts...>;

template<typename Tag, typename Tag::type MemPtr>
struct private_getter {
    friend typename Tag::type get(Tag) {
        return MemPtr;
    }
};

/**
 * @brief A private member getter declarator. This is legal, but spooky C++. The mechanism to enable this template
 * trick is that access specifiers are ignored on explicit instantiation of templates.
 * <br><br>
 * <b>Usage</b>:
 * @code
 *  struct foo {
 *      foo(int val) : x(val) {}
 *  private:
 *      int x;
 *  };
 *  DECLARE_GETTER(foo, int, x);    // This line must be used in a namespace scope.
 *
 *  auto f = foo(42);
 *  auto f_x = get_x(42);           // get_x is generated by this macro.
 * @endcode
 */
#define DECLARE_GETTER(class_name, member_type, member_name)                    \
    struct class_name ## _ ## member_name ## _tag {                             \
        using type = member_type class_name::*;                                 \
        friend type get(class_name ## _ ## member_name ## _tag);                \
    };                                                                          \
    template struct private_getter<class_name ## _ ## member_name ## _tag, &class_name::member_name>; \
    namespace tags {                                                            \
        class_name ## _ ## member_name ## _tag class_name ## member_name;       \
    }                                                                           \
    member_type& get ## _ ## member_name (class_name& obj) {                    \
        return obj.*get(tags:: class_name ## member_name);                      \
    }                                                                           \
    member_type const& get ## _ ## member_name (class_name const& obj) {        \
        return obj.*get(tags:: class_name ## member_name);                      \
    }

/**
 * @brief A simple wrapper class for std::unique_ptr.
 * The top-level const will be propagated to the underlying pointer.
 * @tparam T The data type of the underlying pointer.
 * @tparam Del The customized deleter type (defaulted to std::default_delete).
 */
template<typename T, typename Del = std::default_delete<T>>
class prop_ptr {
public:
    explicit prop_ptr(T* ptr) noexcept
        : m_ptr(ptr) {}

    prop_ptr(prop_ptr const&) = delete;

    prop_ptr(prop_ptr&& other) noexcept
        : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    prop_ptr& operator =(prop_ptr const&) = delete;

    prop_ptr& operator =(prop_ptr&& other) noexcept {
        m_ptr = other.m_ptr;
        other.m_ptr = nullptr;
        return *this;
    }

    T& operator *() {
        return *m_ptr;
    }

    T const& operator *() const {
        return *m_ptr;
    }
private:
    std::experimental::propagate_const<std::unique_ptr<T, Del>> m_ptr;
};

template<typename T, typename Del, typename... Args>
prop_ptr<T, Del> make_prop(Args&&... args) {
    return prop_ptr<T, Del>(new T(FWD(args)...));
}
