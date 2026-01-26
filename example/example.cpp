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

    using namespace BlackLua::Internal;

    // int x = 8;
    // while (x <= 8) { x += 2; }

    int32_t x = 8;
    int32_t constant8 = 8;
    int32_t constant2 = 2;

    OpCode byteCode[] = {
        // Global variables and constants
        { OpCodeType::PushBytes, 4 },
        { OpCodeType::Store, OpCodeStore(-1, &x) },
        { OpCodeType::PushBytes, 4 },
        { OpCodeType::Store, OpCodeStore(-1, &constant8) },
        { OpCodeType::PushBytes, 4 },
        { OpCodeType::Store, OpCodeStore(-1, &constant2) },

        // Loop
        { OpCodeType::Label, 0 },
        { OpCodeType::LteIntegral, OpCodeMath(1, 2) },
        { OpCodeType::Jf, OpCodeJump(-1, 1) }, // Jump to the end of the loop if the condition failed

        // Loop body
        { OpCodeType::AddIntegral, OpCodeMath(1, 3) },
        { OpCodeType::Copy, OpCodeCopy(1, -1) }, // Copy the result of AddIntegral to the x variable
        { OpCodeType::Pop }, // Pop the result of the addition

        { OpCodeType::Jmp, 0 }, // Jump to the start

        // After loop ends
        { OpCodeType::Label, 1 },

        { OpCodeType::Invalid }
    };

    vm.RunByteCode(byteCode, sizeof(byteCode) / sizeof(byteCode[0]));

    return 0;
}
