#include "HttpRequest.h"

#include <algorithm>
#include <ranges>

using std::array, std::pair, std::string_view, std::unordered_map;
using std::ranges::copy, std::views::split;

auto HttpRequest::parse(string_view request) -> HttpRequest {
    array<string_view, 4> result;
    unordered_map<string_view, string_view> header;

    bool isBody{false};

    constexpr string_view delimiter{"\r\n"};
    for (auto const &valueView: request | split(delimiter)) {
        const string_view value{valueView};

        if (result.begin()->empty()) copy(HttpRequest::parseLine(value), result.begin());
        else if (!value.empty() && !isBody)
            header.emplace(HttpRequest::parseHeader(value));
        else if (isBody)
            result[3] = value;

        isBody = value.empty();
    }

    return HttpRequest{result[0], result[1], result[2], result[3], std::move(header)};
}

auto HttpRequest::parseLine(string_view line) -> array<string_view, 3> {
    array<string_view, 3> result;

    for (auto point{result.begin()}; const auto &wordView: line | split(' ')) *point++ = string_view{wordView};

    result[1] = result[1].substr(1);
    result[2] = result[2].substr(5);

    return result;
}

auto HttpRequest::parseHeader(string_view header) -> pair<string_view, string_view> {
    array<string_view, 2> result;

    constexpr string_view delimiter{": "};
    for (auto point{result.begin()}; const auto &wordView: header | split(delimiter)) *point++ = string_view{wordView};

    return {result[0], result[1]};
}

HttpRequest::HttpRequest(string_view method, string_view url, string_view version, string_view body,
                         unordered_map<string_view, string_view> &&header) noexcept
    : method{method}, url{url}, version{version}, body{body}, headers{std::move(header)} {}

auto HttpRequest::getVersion() const noexcept -> string_view { return this->version; }

auto HttpRequest::getMethod() const noexcept -> string_view { return this->method; }

auto HttpRequest::getUrl() const noexcept -> string_view { return this->url; }

auto HttpRequest::getHeaderValue(string_view filed) const -> string_view {
    auto const result{this->headers.find(filed)};

    return result == this->headers.end() ? string_view{} : result->second;
}

auto HttpRequest::getBody() const noexcept -> string_view { return this->body; }
