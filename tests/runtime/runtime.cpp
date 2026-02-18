#include "context.hpp"

#include "catch2.hpp"

TEST_CASE("Runtime Variable Declaration") {
    BlackLua::Context ctx = BlackLua::Context::Create();
    ctx.CompileFile("tests/runtime/variable_declaration.bl", "Runtime Variable Declaration");
    ctx.Run("Runtime Variable Declaration");
    
    ctx.PushGlobal("f");
    REQUIRE(ctx.GetBool(-1) == false);
    ctx.PushGlobal("t");
    REQUIRE(ctx.GetBool(-1) == true);
    ctx.PushGlobal("i");
    REQUIRE(ctx.GetInt(-1) == 99);
}

TEST_CASE("Runtime Basic Expressions") {
    BlackLua::Context ctx = BlackLua::Context::Create();
    ctx.CompileFile("tests/runtime/basic_expressions.bl", "Runtime Basic Expressions");
    ctx.Run("Runtime Basic Expressions");
    
    ctx.PushGlobal("a");
    REQUIRE(ctx.GetInt(-1) == 10);
    ctx.PushGlobal("b");
    REQUIRE(ctx.GetInt(-1) == -3);
    ctx.PushGlobal("c");
    REQUIRE(ctx.GetInt(-1) == 10);
}

TEST_CASE("Runtime Functions") {
    BlackLua::Context ctx = BlackLua::Context::Create();
    ctx.CompileFile("tests/runtime/functions.bl", "Runtime Functions");
    ctx.Run("Runtime Functions");
    
    ctx.Call("main", "Runtime Functions");
    
    ctx.PushGlobal("result");
    REQUIRE(ctx.GetInt(-1) == 24);
}

TEST_CASE("Runtime Control Flow") {
    BlackLua::Context ctx = BlackLua::Context::Create();
    ctx.CompileFile("tests/runtime/control_flow.bl", "Runtime Control Flow");
    fmt::print("{}", ctx.Disassemble("Runtime Control Flow"));
    ctx.Run("Runtime Control Flow");
    
    ctx.Call("While", "Runtime Control Flow");
    REQUIRE(ctx.GetInt(-1) == 9);
    ctx.Pop(1);

    ctx.Call("If", "Runtime Control Flow");
    REQUIRE(ctx.GetBool(-1) == false);
    ctx.Pop(1);
}