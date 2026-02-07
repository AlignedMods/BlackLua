#include "context.hpp"
#include "internal/compiler/lexer.hpp"
#include "internal/compiler/parser.hpp"
#include "internal/compiler/type_checker.hpp"
#include "internal/compiler/emitter.hpp"
#include "internal/compiler/disassembler.hpp"
#include "internal/stdlib/array.hpp"

#include <fstream>
#include <sstream>

namespace BlackLua {

    struct CompiledSource {
        std::vector<Internal::OpCode> OpCodes;
    };

    Context::Context()
        : m_VM(this) {}

    Context Context::Create() {
        Context ctx{};
        ctx.m_VM.AddExtern("bl__array__init__", BlackLua::Internal::bl__array__init__);
        ctx.m_VM.AddExtern("bl__array__destruct__", BlackLua::Internal::bl__array__destruct__);
        ctx.m_VM.AddExtern("bl__array__copy__", BlackLua::Internal::bl__array__copy__);
        ctx.m_VM.AddExtern("bl__array__index__", BlackLua::Internal::bl__array__index__);
        
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
        p.PrintAST();

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

    void Context::SetErrorHandler(ErrorHandlerFn fn) {
        m_ErrorHandler = fn;
    }

    void Context::ReportRuntimeError(const std::string& error) {
        if (m_ErrorHandler) {
            m_ErrorHandler(error);
        } else {
            BLUA_FORMAT_ERROR("A runtime error occurred!\nError message: {}", error);
        }

        m_VM.StopExecution();
    }

    Internal::VM& Context::GetVM() {
        return m_VM;
    }

} // namespace BlackLua
