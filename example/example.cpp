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
    vm.PushBytes(8);
    vm.PushBytes(4);
    vm.PushBytes(8);
    vm.PushBytes(4);
    vm.PushBytes(2);
    vm.PushBytes(1);
    vm.PushBytes(1);

    vm.StoreInt(-8, 7);
    vm.StoreLong(-7, 758345999944924ll);
    vm.StoreFloat(-6, 6769.69f);
    vm.StoreDouble(-5, 42324398432972397432984.069696969);
    vm.StoreInt(-4, 9);
    vm.StoreShort(-3, 7788);
    vm.StoreChar(-2, '+');
    vm.StoreBool(-1, false);

    BLUA_FORMAT_PRINT("-1: {}, -2: {}, -3: {}, -4: {}, -5: {}, -6: {}, -7: {}, -8: {}", vm.GetBool(-1), vm.GetChar(-2), vm.GetShort(-3), vm.GetInt(-4), vm.GetDouble(-5), vm.GetFloat(-6), vm.GetLong(-7), vm.GetInt(-8));

    return 0;
}
