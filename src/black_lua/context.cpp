#include "context.hpp"
#include "internal/compiler/lexer.hpp"
#include "internal/compiler/parser.hpp"
#include "internal/compiler/type_checker.hpp"
#include "internal/compiler/emitter.hpp"
#include "internal/compiler/disassembler.hpp"
#include "internal/compiler/ast_dumper.hpp"
#include "internal/stdlib/array.hpp"

#include <fstream>
#include <sstream>

namespace BlackLua {

    struct CompiledSource {
        std::vector<Internal::OpCode> OpCodes;
        Internal::ASTNodes ASTNodes;
        Allocator* Allocator = nullptr;
        std::string SourceCode;

        std::string Module;
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

    void Context::CompileFile(const std::string& path, const std::string& module) {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open file: " << path << "!\n";
            return;
        }

        std::stringstream ss;
        ss << file.rdbuf();
        std::string contents = ss.str();
        ss.flush();

        CompileString(contents, module);
    }

    void Context::CompileString(const std::string& source, const std::string& module) {
        bool valid = true;

        CompiledSource* src = new CompiledSource();
        src->Allocator = new Allocator(10 * 1024 * 1024);
        src->SourceCode = source;
        src->Module = module;
        m_CurrentCompiledSource = src;

        Internal::Lexer l = Internal::Lexer::Lex({ src->SourceCode.c_str(), src->SourceCode.size() });

        Internal::Parser p = Internal::Parser::Parse(l.GetTokens(), this);
        valid = p.IsValid();
        if (!valid) {delete src->Allocator; return; }
        src->ASTNodes = *p.GetNodes();

        Internal::TypeChecker c = Internal::TypeChecker::Check(&src->ASTNodes, this);
        valid = c.IsValid();
        if (!valid) { delete src->Allocator; return; }

        Internal::Emitter e = Internal::Emitter::Emit(p.GetNodes(), this);
        src->OpCodes = e.GetOpCodes();

        m_Modules[module] = src;
    }

    void Context::FreeModule(const std::string& module) {
        if (!m_Modules.contains(module)) {
            fmt::print("Module {} does not exist in the current context!", module);
            return;
        }

        CompiledSource* src = m_Modules.at(module);
        delete src->Allocator;
        delete src;
        m_Modules.erase(module);
    }

    void Context::Run(const std::string& module) {
        if (!m_Modules.contains(module)) {
            fmt::print("Module {} does not exist in the current context!", module);
            return;
        }

        CompiledSource* src = m_Modules.at(module);
        m_CurrentCompiledSource = src;
        m_VM.RunByteCode(src->OpCodes.data(), src->OpCodes.size());
    }

    std::string Context::DumpAST(const std::string& module) {
        if (!m_Modules.contains(module)) {
            fmt::print("Module {} does not exist in the current context!", module);
            return {};
        }

        CompiledSource* src = m_Modules.at(module);
        Internal::ASTDumper d = Internal::ASTDumper::DumpAST(&src->ASTNodes);
        return d.GetOutput();
    }

    std::string Context::Disassemble(const std::string& module) {
        if (!m_Modules.contains(module)) {
            fmt::print("Module {} does not exist in the current context!", module);
            return {};
        }

        CompiledSource* src = m_Modules.at(module);
        Internal::Disassembler d = Internal::Disassembler::Disassemble(&src->OpCodes);
        return d.GetDisassembly();
    }

    void Context::SetRuntimeErrorHandler(RuntimeErrorHandlerFn fn) {
        m_RuntimeErrorHandler = fn;
    }

    void Context::SetCompilerErrorHandler(CompilerErrorHandlerFn fn) {
        m_CompilerErrorHandler = fn;
    }

    void Context::ReportCompilerError(size_t line, size_t column, const std::string& error) {
        if (m_CompilerErrorHandler) {
            m_CompilerErrorHandler(line, column, m_CurrentCompiledSource->Module, error);
        } else {
            fmt::print(fg(fmt::color::gray), "{}:{}:{}, ", m_CurrentCompiledSource->Module, line, column);
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

    Allocator* Context::GetAllocator() {
        return m_CurrentCompiledSource->Allocator;
    }

    Internal::VM& Context::GetVM() {
        return m_VM;
    }

} // namespace BlackLua
