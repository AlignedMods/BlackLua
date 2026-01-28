#include "black_lua/black_lua.hpp"
#include "black_lua/compiler.hpp"
#include "black_lua/vm.hpp"

int main(int argc, char** argv) {
    BlackLua::SetupDefaultAllocator();

    BlackLua::Context context = BlackLua::Context::Create();
    auto& vm = context.GetVM();

    BlackLua::CompiledSource compiled = context.CompileFile("test.bl");
    context.Run(compiled, "test");
    vm.Call(0);
    
    BLUA_FORMAT_PRINT("{}", context.GetVM().GetInt(-1));

    return 0;
}
