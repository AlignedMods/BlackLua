#include "black_lua.hpp"

int main(int argc, char** argv) {
    BlackLua::Context ctx = BlackLua::Context::Create();
    ctx.CompileFile("tests/runtime/basic_expressions.bl", "decl");
    fmt::print("{}", ctx.DumpAST("decl"));
    fmt::print("{}", ctx.Disassemble("decl"));
    ctx.Run("decl");

    return 0;
}
