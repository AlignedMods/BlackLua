#include "black_lua.hpp"

int main(int argc, char** argv) {
    BlackLua::Context context = BlackLua::Context::Create();

    context.CompileFile("tests/runtime/basic_expressions.bl", "test.bl");
    std::cout << context.DumpAST("test.bl");
    std::cout << context.Disassemble("test.bl");
    context.Run("test.bl");
    // context.Call("main", "test.bl");

    context.FreeModule("test.bl");

    return 0;
}
