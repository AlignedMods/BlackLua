#include "black_lua.hpp"

int main(int argc, char** argv) {
    BlackLua::SetupDefaultAllocator();

    BlackLua::Context context = BlackLua::Context::Create();
    auto& vm = context.GetVM();

    BlackLua::CompiledSource* compiled = context.CompileFile("test.bl");
    std::cout << context.Disassemble(compiled);
    context.Run(compiled, "test");
    vm.Call(1);

    BLUA_FORMAT_PRINT("{}", vm.GetInt(-1));

    context.FreeSource(compiled);

    return 0;
}
