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

    int32_t myInt = 69420;
    BlackLua::Internal::OpCode byteCode[] = {
        { BlackLua::Internal::OpCodeType::PushBytes, 4 },
        { BlackLua::Internal::OpCodeType::Store, BlackLua::Internal::OpCodeStore(-1, &myInt) },
        { BlackLua::Internal::OpCodeType::AddIntegral, BlackLua::Internal::OpCodeMath(-1, -1) },
        { BlackLua::Internal::OpCodeType::Get, -1 },
        { BlackLua::Internal::OpCodeType::Get, -1 },
    };

    vm.RunByteCode(byteCode, sizeof(byteCode) / sizeof(byteCode[0]));

    BLUA_FORMAT_PRINT("Stack: {}, {}, {}", vm.GetInt(-1), vm.GetInt(-2), vm.GetInt(-3));

    return 0;
}
