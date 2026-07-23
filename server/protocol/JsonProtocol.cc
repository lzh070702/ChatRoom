#include "JsonProtocol.h"

std::string JsonProtocol::encode(const json& js) {
    return js.dump();
}

json JsonProtocol::decode(const std::string& data) {
    return json::parse(data /*, nullptr, true, true*/);
}