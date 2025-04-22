#pragma once
#include <utility>

namespace {

template <typename F>
class defer_impl {
   public:
    defer_impl(defer_impl &&) = delete;
    defer_impl(const defer_impl &) = delete;
    defer_impl &operator=(const defer_impl &) = delete;
    defer_impl &operator=(defer_impl &&) = delete;

    defer_impl(F &&f) : defer_function(std::forward<F>(f)) {}

    ~defer_impl() { defer_function(); }

   private:
    F defer_function;
};

}  // namespace defer

#define __DEFER__JOIN(x, y)    x##y
#define __DEFER_JOIN(x, y)     __DEFER__JOIN(x, y)
#define __DEFER_UNIQUE_NAME(x) __DEFER_JOIN(x, __LINE__)

#define defer(lambda__) [[maybe_unused]] const auto &__DEFER_UNIQUE_NAME(defer_object) = ::defer_impl([&]() lambda__)