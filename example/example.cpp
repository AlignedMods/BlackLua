#include "black_lua.hpp"

int main(int argc, char** argv) {
    BlackLua::Context context = BlackLua::Context::Create();
    auto& vm = context.GetVM();

    context.CompileFile("test.bl", "test.bl");
    std::cout << context.DumpAST("test.bl");
    std::cout << context.Disassemble("test.bl");
    context.Run("test.bl");
    vm.Call(1);
    
    fmt::print("{}", vm.GetInt(-1));
    
    context.FreeModule("test.bl");

    return 0;
}
