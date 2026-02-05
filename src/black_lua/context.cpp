#include "context.hpp"
#include "internal/compiler/lexer.hpp"
#include "internal/compiler/parser.hpp"
#include "internal/compiler/type_checker.hpp"
#include "internal/compiler/emitter.hpp"
#include "internal/compiler/disassembler.hpp"

#include <fstream>
#include <sstream>

namespace BlackLua {

    struct CompiledSource {
        std::vector<Internal::OpCode> OpCodes;
    };

    Context Context::Create() {
        Context ctx{};
        
        return ctx;
    }

    CompiledSource* Context::CompileFile(const std::string& path) {
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

    CompiledSource* Context::CompileString(const std::string& source) {
        bool valid = true;

        BlackLua::Internal::Lexer l = BlackLua::Internal::Lexer::Parse(source);

        BlackLua::Internal::Parser p = BlackLua::Internal::Parser::Parse(l.GetTokens());
        valid = p.IsValid();
        if (!valid) { return nullptr; }

        BlackLua::Internal::TypeChecker c = BlackLua::Internal::TypeChecker::Check(p.GetNodes());
        valid = c.IsValid();
        if (!valid) { return nullptr; }

        BlackLua::Internal::Emitter e = BlackLua::Internal::Emitter::Emit(p.GetNodes());

        CompiledSource* src = new CompiledSource();
        src->OpCodes = e.GetOpCodes();

        return src;
    }

    void Context::FreeSource(CompiledSource* source) {
        delete source;
    }

    void Context::Run(CompiledSource* compiled, const std::string& module) {
        m_VM.RunByteCode(compiled->OpCodes.data(), compiled->OpCodes.size());
    }

    std::string Context::Disassemble(CompiledSource* compiled) {
        BlackLua::Internal::Disassembler d = BlackLua::Internal::Disassembler::Disassemble(&compiled->OpCodes);
        return d.GetDisassembly();
    }

    Internal::VM& Context::GetVM() {
        return m_VM;
    }

} // namespace BlackLua
