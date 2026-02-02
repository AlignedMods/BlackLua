#include "black_lua/black_lua.hpp"
#include "black_lua/compiler.hpp"
#include "black_lua/vm.hpp"

void bl_Add(BlackLua::Internal::VM* vm) {
    int32_t lhs = vm->GetInt(-3);
    int32_t rhs = vm->GetInt(-2);

    vm->StoreInt(-1, lhs + rhs);
}

void bl_CastIntToFloat(BlackLua::Internal::VM* vm) {
    int32_t x = vm->GetInt(-2);

    vm->StoreFloat(-1, static_cast<float>(x));
}

int main(int argc, char** argv) {
    BlackLua::SetupDefaultAllocator();

    BlackLua::Context context = BlackLua::Context::Create();
    auto& vm = context.GetVM();

    vm.AddExtern("Add", bl_Add);
    vm.AddExtern("CastIntToFloat", bl_CastIntToFloat);

    BlackLua::CompiledSource compiled = context.CompileFile("test.bl");
    context.Run(compiled, "test");
    vm.Call(0);

    BLUA_FORMAT_PRINT("{}", vm.GetInt(-1));

    return 0;
}
