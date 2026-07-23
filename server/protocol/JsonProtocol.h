#pragma once
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

class JsonProtocol {
   public:
    static std::string encode(const json& data);
    static json decode(const std::string& data);
};