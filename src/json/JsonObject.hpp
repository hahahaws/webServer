#pragma once

#include <string>
#include <unordered_map>

class JsonValue;

class JsonObject {
public:
    explicit JsonObject(std::string_view json = std::string_view{});

    [[nodiscard]] auto toString() const -> std::string;

    auto add(std::string_view key, JsonValue &&value) -> void;

    [[nodiscard]] auto operator[](const std::string &key) const -> const JsonValue &;

    [[nodiscard]] auto operator[](const std::string &key) -> JsonValue &;

    auto remove(const std::string &key) -> void;

    [[nodiscard]] auto stringSize() const -> unsigned long;

private:
    std::unordered_map<std::string, JsonValue> values;
};
