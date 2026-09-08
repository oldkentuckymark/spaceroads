#pragma once

// ============================================================
//  compile_time_csv.hpp  —  C++26 compile-time CSV → vector
// ============================================================
//
//  Parses a comma-separated list of numbers entirely at compile
//  time and returns a std::vector<double>.
//  Returns an empty vector on any parse error.
//
//  Usage:
//    consteval auto values = parse_csv("1.0, -2.5, 3.14e2, 0.007");

#include <vector>
#include <string_view>

namespace detail {

constexpr double pow10(int exp) noexcept {
    double r = 1.0;
    if (exp >= 0) for (int i = 0; i < exp; ++i) r *= 10.0;
    else          for (int i = 0; i > exp; --i) r /= 10.0;
    return r;
}

constexpr void skip_delimiters(std::string_view& sv) noexcept {
    while (!sv.empty()) {
        const char c = sv.front();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ',')
            sv.remove_prefix(1);
        else
            break;
    }
}

// Parses one double from the front of sv, advancing it past the number.
// Returns false on any parse error, leaving result unchanged.
constexpr bool parse_double(std::string_view& sv, double& result) noexcept {
    if (sv.empty()) return false;

    // ── sign ──────────────────────────────────────────────────────────────────
    double sign = 1.0;
    if      (sv.front() == '-') { sign = -1.0; sv.remove_prefix(1); }
    else if (sv.front() == '+') {              sv.remove_prefix(1); }

    // ── integer part ──────────────────────────────────────────────────────────
    double value = 0.0;
    bool has_digits = false;
    while (!sv.empty() && sv.front() >= '0' && sv.front() <= '9') {
        value = value * 10.0 + (sv.front() - '0');
        sv.remove_prefix(1);
        has_digits = true;
    }

    // ── fractional part ───────────────────────────────────────────────────────
    if (!sv.empty() && sv.front() == '.') {
        sv.remove_prefix(1);
        double place = 0.1;
        while (!sv.empty() && sv.front() >= '0' && sv.front() <= '9') {
            value += (sv.front() - '0') * place;
            place *= 0.1;
            sv.remove_prefix(1);
            has_digits = true;
        }
    }

    if (!has_digits) return false;  // bare sign or lone dot

    // ── exponent ──────────────────────────────────────────────────────────────
    if (!sv.empty() && (sv.front() == 'e' || sv.front() == 'E')) {
        sv.remove_prefix(1);

        int exp_sign = 1;
        if      (!sv.empty() && sv.front() == '-') { exp_sign = -1; sv.remove_prefix(1); }
        else if (!sv.empty() && sv.front() == '+') {                sv.remove_prefix(1); }

        if (sv.empty() || sv.front() < '0' || sv.front() > '9')
            return false;  // 'e' with no exponent digits

        int exp = 0;
        while (!sv.empty() && sv.front() >= '0' && sv.front() <= '9') {
            exp = exp * 10 + (sv.front() - '0');
            sv.remove_prefix(1);
        }
        value *= pow10(exp_sign * exp);
    }

    result = sign * value;
    return true;
}

} // namespace detail

consteval std::vector<double> parse_csv(std::string_view csv) {
    std::vector<double> result;

    while (true) {
        detail::skip_delimiters(csv);
        if (csv.empty()) break;

        double value{};
        if (!detail::parse_double(csv, value))
            return {};  // bad input — return empty vector

        result.push_back(value);
    }

    return result;
}
