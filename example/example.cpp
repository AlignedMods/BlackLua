#include "black_lua/black_lua.hpp"
#include "black_lua/compiler.hpp"

int main() {
    BlackLua::SetupDefaultAllocator();

    BlackLua::LuaContext context = BlackLua::LuaContext::Create();

    BlackLua::CompiledSource compiled = context.CompileFile("test.c");
    std::cout << compiled.Compiled << '\n';
    context.Run(compiled, "test");

    return 0;
}
