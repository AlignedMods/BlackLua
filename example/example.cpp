#include "black_lua/black_lua.hpp"
#include "black_lua/compiler.hpp"
#include "black_lua/vm.hpp"

int main(int argc, char** argv) {
    BlackLua::SetupDefaultAllocator();

    BlackLua::Context context = BlackLua::Context::Create();

    BlackLua::CompiledSource compiled = context.CompileFile("test.bl");
    context.Run(compiled, "test");

    BLUA_FORMAT_PRINT("{}, {}", context.GetVM().GetDouble(-1), context.GetVM().GetDouble(-2));

    return 0;
}
