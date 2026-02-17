#include "black_lua.hpp"

int main(int argc, char** argv) {
    BlackLua::Context context = BlackLua::Context::Create();
    auto& vm = context.GetVM();

    context.CompileFile("test.bl", "test.bl");
    std::cout << context.DumpAST("test.bl");
    std::cout << context.Disassemble("test.bl");
    context.Run("test.bl");
    context.Call("main", "test.bl");
    
    fmt::println("{}", vm.GetInt(-2));

    context.FreeModule("test.bl");

    return 0;
}
