#include "compiler.hpp"

extern "C" {
    #include "lua.h"
    #include "lualib.h"
    #include "lauxlib.h"
}

#include <fstream>
#include <sstream>

namespace BlackLua {

    Context Context::Create() {
        Context ctx{};
        
        return ctx;
    }

    CompiledSource Context::CompileFile(const std::string& path) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << "!\n";
            return {};
        }

        std::stringstream ss;
        ss << file.rdbuf();
        std::string contents = ss.str();
        ss.flush();

        return CompileString(contents);
    }

    CompiledSource Context::CompileString(const std::string& source) {
        CompiledSource src{};

        BlackLua::Internal::Lexer l = BlackLua::Internal::Lexer::Parse(source);

        BlackLua::Internal::Parser p = BlackLua::Internal::Parser::Parse(l.GetTokens());

        bool valid = true;

        if (p.IsValid()) {
            p.PrintAST();
        
            BlackLua::Internal::TypeChecker c = BlackLua::Internal::TypeChecker::Check(p.GetNodes());
            if (c.IsValid()) {
                BlackLua::Internal::Emitter e = BlackLua::Internal::Emitter::Emit(c.GetCheckedNodes());
                src.OpCodes = e.GetOpCodes();
            } else {
                valid = false;
            }
        } else {
            valid = false;
        }

        if (!valid) {
            std::cerr << "No output generated.\n";
        }

        return src;
    }

    void Context::Run(const CompiledSource& compiled, const std::string& module) {
        m_VM.RunByteCode(compiled.OpCodes.data(), compiled.OpCodes.size());
    }

    Internal::VM& Context::GetVM() {
        return m_VM;
    }

} // namespace BlackLua
