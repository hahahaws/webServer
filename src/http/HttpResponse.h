#pragma once

#include <span>
#include <string_view>
#include <vector>

struct HttpResponse {
    constexpr HttpResponse() noexcept = default;

    auto setVersion(std::string_view newVersion) -> void;

    auto setStatusCode(std::string_view newStatusCode) -> void;

    auto addHeader(std::string_view header) -> void;

    auto setBody(std::span<const std::byte> newBody) -> void;

    [[nodiscard]] auto combine() -> std::vector<std::byte>;

private:
    std::vector<std::byte> version, statusCode, headers, body;
};
