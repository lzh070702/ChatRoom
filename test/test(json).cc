#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
using namespace std;
using json = nlohmann::json;

int main() {
    json js = {{"type", 1}};
    int str = js["type"];
    cout << str << endl;
    return 0;
}