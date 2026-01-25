#include "black_lua/black_lua.hpp"
#include "black_lua/compiler.hpp"
#include "black_lua/vm.hpp"

int main(int argc, char** argv) {
    BlackLua::SetupDefaultAllocator();

    BlackLua::LuaContext context = BlackLua::LuaContext::Create();

    BlackLua::CompiledSource compiled = context.CompileFile("test.bl");
    std::cout << compiled.Compiled << '\n';
    context.Run(compiled, "test");

    BlackLua::Internal::VM vm;
    vm.PushBytes(4);
    vm.PushBytes(4);
    
    vm.StoreInt(-1, 4);
    vm.Copy(-1, -2);

    BLUA_FORMAT_PRINT("Stack: {}, {}", vm.GetInt(-1), vm.GetInt(-2));

    return 0;
}
