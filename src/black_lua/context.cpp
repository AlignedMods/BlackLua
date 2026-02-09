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

        m_CurrentFile = path;
        CompiledSource* c = CompileString(contents);
        m_CurrentFile.clear();
        return c;
    }

    CompiledSource* Context::CompileString(const std::string& source) {
        bool valid = true;

        Internal::Lexer l = Internal::Lexer::Parse(source);

        Internal::Parser p = Internal::Parser::Parse(l.GetTokens(), this);
        valid = p.IsValid();
        if (!valid) { return nullptr; } 
        p.PrintAST();

        Internal::TypeChecker c = Internal::TypeChecker::Check(p.GetNodes(), this);
        valid = c.IsValid();
        if (!valid) { return nullptr; }

        Internal::Emitter e = Internal::Emitter::Emit(p.GetNodes());

        CompiledSource* src = new CompiledSource();
        src->OpCodes = e.GetOpCodes();

        return src;
    }

    void Context::FreeSource(CompiledSource* source) {
        delete source;
    }

    void Context::Run(CompiledSource* compiled, const std::string& module) {
        if (compiled) {
            m_VM.RunByteCode(compiled->OpCodes.data(), compiled->OpCodes.size());
        }
    }

    std::string Context::Disassemble(CompiledSource* compiled) {
        if (compiled) {
            Internal::Disassembler d = Internal::Disassembler::Disassemble(&compiled->OpCodes);
            return d.GetDisassembly();
        }
        
        return {};
    }

    void Context::SetRuntimeErrorHandler(RuntimeErrorHandlerFn fn) {
        m_RuntimeErrorHandler = fn;
    }

    void Context::SetCompilerErrorHandler(CompilerErrorHandlerFn fn) {
        m_CompilerErrorHandler = fn;
    }

    void Context::ReportCompilerError(size_t line, size_t column, const std::string& error) {
        if (m_CompilerErrorHandler) {
            m_CompilerErrorHandler(line, column, m_CurrentFile, error);
        } else {
            fmt::print(fg(fmt::color::gray), "{}:{}:{}, ", m_CurrentFile, line, column);
            fmt::print(fg(fmt::color::pale_violet_red), "fatal error: ");
            fmt::print("{}\n", error);
        }
    }

    void Context::ReportRuntimeError(const std::string& error) {
        if (m_RuntimeErrorHandler) {
            m_RuntimeErrorHandler(error);
        } else {
            fmt::print(stderr, "A runtime error occurred!\nError message: {}", error);
        }

        m_VM.StopExecution();
    }

    Internal::VM& Context::GetVM() {
        return m_VM;
    }

} // namespace BlackLua
