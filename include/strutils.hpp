#pragma once

#include "prelude.hpp"

namespace ysh {

struct string : public std::string_view {
    using base_type = std::string_view;
    using material_type = std::string;
    using iterator_type = base_type::iterator;

    using base_type::base_type;

    struct hash {
        std::size_t operator ()(string const& str) const noexcept {
            return std::hash<base_type>()(str);
        }
    };

    string(base_type const& other)
        : base_type(other) {}

    string(string const& other) noexcept
        : base_type(other.data()) {}

    string& operator =(string const& other) noexcept {
        base_type::operator =(other);
        return *this;
    }

    [[nodiscard]]
    iterator_type find(char c) const noexcept {
        return this->begin() + this->base_type::find(c);
    }

    [[nodiscard]]
    iterator_type find(string const& str) const noexcept {
        return this->begin() + this->base_type::find(str);
    }

    iterator_type find(char c, iterator_type pos) const noexcept {
        return this->begin() + this->base_type::find(c, pos - this->begin());
    }

    iterator_type find(string const& str, iterator_type pos) const noexcept {
        return this->begin() + this->base_type::find(str, pos - this->begin());
    }

    [[nodiscard]]
    iterator_type find_first_not_of(char c) const noexcept {
        return this->begin() + this->base_type::find_first_not_of(c);
    }

    [[nodiscard]]
    iterator_type find_first_not_of(string const& str) const noexcept {
        return this->begin() + this->base_type::find_first_not_of(str);
    }

    iterator_type find_first_not_of(char c, iterator_type pos) const noexcept {
        return this->begin() + this->base_type::find_first_not_of(c, pos - this->begin());
    }

    iterator_type find_first_not_of(string const& str, iterator_type pos) const noexcept {
        return this->begin() + base_type::find_first_not_of(str, pos - this->begin());
    }

    [[nodiscard]]
    iterator_type find_first_of(char c) const noexcept {
        return this->begin() + this->base_type::find_first_of(c);
    }

    [[nodiscard]]
    iterator_type find_first_of(string const& str) const noexcept {
        return this->begin() + this->base_type::find_first_of(str);
    }

    iterator_type find_first_of(char c, iterator_type pos) const noexcept {
        return this->begin() + base_type::find_first_of(c, pos - this->begin());
    }

    iterator_type find_first_of(string const& str, iterator_type pos) const noexcept {
        return this->begin() + base_type::find_first_of(str, pos - this->begin());
    }

    [[nodiscard]]
    iterator_type find_last_not_of(char c) const noexcept {
        return this->begin() + this->base_type::find_last_not_of(c);
    }

    [[nodiscard]]
    iterator_type find_last_not_of(string const& str) const noexcept {
        return this->begin() + this->base_type::find_last_not_of(str);
    }

    iterator_type find_last_not_of(char c, iterator_type pos) const noexcept {
        return this->begin() + this->base_type::find_last_not_of(c, pos - this->begin());
    }

    iterator_type find_last_not_of(string const& str, iterator_type pos) const noexcept {
        return this->begin() + this->base_type::find_last_not_of(str, pos - this->begin());
    }

    [[nodiscard]]
    iterator_type find_last_of(char c) const noexcept {
        return this->begin() + this->base_type::find_last_of(c);
    }

    [[nodiscard]]
    iterator_type find_last_of(string const& str) const noexcept {
        return this->begin() + this->base_type::find_last_of(str);
    }

    iterator_type find_last_of(char c, iterator_type pos) const noexcept {
        return this->begin() + base_type::find_last_of(c, pos - this->begin());
    }

    iterator_type find_last_of(string const& str, iterator_type pos) const noexcept {
        return this->begin() + this->base_type::find_last_of(str, pos - this->begin());
    }

    static material_type join(std::vector<string> words, string const& sep) {
        material_type result;
        for (auto word : words | stdv::take(words.size() - 1)) {
            result += word;
            result += sep;
        }
        result += words.back();
        return result;
    }

    friend material_type operator +(string const& lhs, material_type const& other) {
        return std::string(lhs) + other;
    }

    friend material_type operator +(material_type const& lhs, string const& other) {
        return lhs + std::string(other);
    }

    template<std::size_t N>
    friend material_type operator +(char const (&lhs)[N], string const& other) {
        return std::string(lhs) + std::string(other);
    }

    template<std::size_t N>
    friend material_type operator +(string const& lhs, char const (&other)[N]) {
        return std::string(lhs) + std::string(other);
    }

    string remove_prefix(iterator_type last) const {
        auto result = *this;
        result.base_type::remove_prefix(last - this->begin());
        return result;
    }

    string remove_suffix(iterator_type first) const {
        auto result = *this;
        result.base_type::remove_suffix(this->end() - first);
        return result;
    }

    [[nodiscard]]
    std::vector<string> split(string const& delim) const {
        auto split_range = stdv::split(static_cast<base_type>(*this), delim);
        auto result = std::vector<string>();
        for (auto const word : split_range) {
            result.emplace_back(word.begin(), word.end());
        }
        return result;
    }

    string substr(iterator_type first) const {
        return this->substr(first, this->end());
    }

    string substr(iterator_type first, iterator_type last) const {
        return string(this->base_type::substr(first - this->begin(), last - first ));
    }

    string trim(char ch = ' ') const noexcept {
        return this->substr(this->find_first_not_of(ch), this->find_last_not_of(ch) + 1);
    }

    explicit operator std::string() const {
        return { this->begin(), this->end() };
    }

    template<arithmetic T>
    explicit operator T() const {
        T result;
        std::from_chars(this->begin(), this->end(), result);
        return result;
    }
};

template<char... Chars>
inline string operator ""_s() noexcept {
    return { Chars... };
}

} // namespace ysh