#include "compiler.hpp"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include <fstream>
#include <sstream>

namespace BlackLua {

    LuaContext LuaContext::Create() {
        LuaContext ctx{};
        ctx.m_State = luaL_newstate();
        luaL_openlibs(reinterpret_cast<lua_State*>(ctx.m_State));
        
        return ctx;
    }

    CompiledSource LuaContext::CompileFile(const std::string& path) {
        std::ifstream file(path);
        std::stringstream ss;
        ss << file.rdbuf();
        std::string contents = ss.str();
        ss.flush();

        return CompileString(contents);
    }

    CompiledSource LuaContext::CompileString(const std::string& source) {
        CompiledSource src{};

        BlackLua::Internal::Lexer l = BlackLua::Internal::Lexer::Parse(source);
        BlackLua::Internal::Parser p = BlackLua::Internal::Parser::Parse(l.GetTokens());

        if (p.IsValid()) {
            BlackLua::Internal::TypeChecker c = BlackLua::Internal::TypeChecker::Check(p.GetNodes());
            BlackLua::Internal::Emitter e = BlackLua::Internal::Emitter::Emit(c.GetCheckedNodes());
            src.Compiled = e.GetOutput();
        } else {
            std::cerr << "No output generated.";
            src.Compiled = "";
        }

        return src;
    }

    void LuaContext::Run(const CompiledSource& compiled, const std::string& module) {
        BLUA_ASSERT(m_State, "Context is not valid!");

        // Set the current module to be this
        lua_pushstring(reinterpret_cast<lua_State*>(m_State), module.c_str());

        if (luaL_dostring(reinterpret_cast<lua_State*>(m_State), compiled.Compiled.c_str()) != LUA_OK) {
            const char* errorMsg = lua_tostring(reinterpret_cast<lua_State*>(m_State), -1);

            std::cerr << "Lua error occured during LuaContext::Run: " << errorMsg << '\n';

            lua_pop(reinterpret_cast<lua_State*>(m_State), 1);
        }

        m_Modules[module] = luaL_ref(reinterpret_cast<lua_State*>(m_State), LUA_REGISTRYINDEX); // Save the executed source code
    }

    void LuaContext::SetActiveModule(const std::string& module) {
        lua_rawgeti(reinterpret_cast<lua_State*>(m_State), LUA_REGISTRYINDEX, m_Modules.at(module));
    }

} // namespace BlackLua