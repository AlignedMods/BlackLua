#include "black_lua/black_lua.hpp"
#include "black_lua/compiler.hpp"

#include <fstream>
#include <sstream>

int main() {
    std::ifstream file("test.lua");
    std::stringstream ss;
    ss << file.rdbuf();
    std::string contents = ss.str();
    ss.flush();

    BlackLua::Lexer l = BlackLua::Lexer::Parse(contents);
    for (const auto& token : l.GetTokens()) {
        std::cout << "Token: " << BlackLua::TokenTypeToString(token.Type) << '\n';
    }

    BlackLua::Parser p = BlackLua::Parser::Parse(l.GetTokens());
    std::cout << "bye";

    return 0;
}