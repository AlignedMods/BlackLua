#include "context.hpp"
#include "internal/compiler/lexer.hpp"
#include "internal/compiler/parser.hpp"
#include "internal/compiler/type_checker.hpp"
#include "internal/compiler/emitter.hpp"
#include "internal/compiler/disassembler.hpp"
#include "internal/compiler/ast_dumper.hpp"
#include "internal/compiler/reflection/compiler_reflection.hpp"
#include "internal/stdlib/array.hpp"
#include "internal/stdlib/string.hpp"

#include <fstream>
#include <sstream>

namespace BlackLua {

    struct CompiledSource {
        CompiledSource(Context* ctx)
            : VM(ctx) {}

        std::vector<Internal::OpCode> OpCodes;
        Internal::ASTNodes ASTNodes;
        Allocator* Allocator = nullptr;
        std::string SourceCode;

        std::string Module;

        Internal::CompilerReflectionData ReflectionData;
        Internal::VM VM;
    };

    Context::Context() {}

    Context Context::Create() {
        Context ctx{};
        // ctx.m_VM.AddExtern("bl__array__init__", BlackLua::Internal::bl__array__init__);
        // ctx.m_VM.AddExtern("bl__array__destruct__", BlackLua::Internal::bl__array__destruct__);
        // ctx.m_VM.AddExtern("bl__array__copy__", BlackLua::Internal::bl__array__copy__);
        // ctx.m_VM.AddExtern("bl__array__index__", BlackLua::Internal::bl__array__index__);
        // 
        // ctx.m_VM.AddExtern("bl__string__construct__", BlackLua::Internal::bl__string__construct__);
        // ctx.m_VM.AddExtern("bl__string__construct_from_literal__", BlackLua::Internal::bl__string__construct_from_literal__);
        // 
        // ctx.m_VM.AddExtern("bl__string__copy__", BlackLua::Internal::bl__string__copy__);
        // ctx.m_VM.AddExtern("bl__string__assign__", BlackLua::Internal::bl__string__assign__);
        
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

        CompiledSource* src = new CompiledSource(this);
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
        src->ReflectionData = e.GetReflectionData();
        src->OpCodes = e.GetOpCodes();

        m_Modules[module] = src;
    }

    void Context::FreeModule(const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        delete src->Allocator;
        delete src;
        m_Modules.erase(module);
    }

    void Context::Run(const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        m_CurrentCompiledSource = src;
        m_CurrentCompiledSource->VM.RunByteCode(src->OpCodes.data(), src->OpCodes.size());
    }

    std::string Context::DumpAST(const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        Internal::ASTDumper d = Internal::ASTDumper::DumpAST(&src->ASTNodes);
        return d.GetOutput();
    }

    std::string Context::Disassemble(const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        Internal::Disassembler d = Internal::Disassembler::Disassemble(&src->OpCodes);
        return d.GetDisassembly();
    }

    void Context::PushGlobal(const std::string& str, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        BLUA_ASSERT(src->ReflectionData.Declarations.contains(str), "Trying to push an unknown global variable");
        BLUA_ASSERT(src->ReflectionData.Declarations.at(str).Type == Internal::ReflectionType::Variable, "Trying to push a non-variable global (perhaps a function)");

        src->VM.Ref(std::get<int32_t>(src->ReflectionData.Declarations.at(str).Data));
    }

    void Context::Pop(size_t count, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        for (size_t i = 0; i < count; i++) {
            src->VM.Pop();
        }
    }

    bool Context::GetBool(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetBool(index);
    }

    int8_t Context::GetChar(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetChar(index);
    }

    int16_t Context::GetShort(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetShort(index);
    }

    int32_t Context::GetInt(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetInt(index);
    }

    int64_t Context::GetLong(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetLong(index);
    }

    float Context::GetFloat(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetFloat(index);
    }

    double Context::GetDouble(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetDouble(index);
    }

    void Context::Call(const std::string& str, const std::string& module) {
        BLUA_ASSERT(m_Modules.contains(module), "Current context does not contain the requested module!");
        CompiledSource* src = m_Modules.at(module);

        BLUA_ASSERT(src->ReflectionData.Declarations.contains(str), "Trying to call an unknown function");
        BLUA_ASSERT(src->ReflectionData.Declarations.at(str).Type == Internal::ReflectionType::Function, "Trying to call an non-function");
        size_t size = GetTypeSize(src->ReflectionData.Declarations.at(str).ResolvedType);
        if (size > 0) {
            src->VM.PushBytes(size);
        }
        src->VM.Call(std::get<int32_t>(src->ReflectionData.Declarations.at(str).Data));
    }

    void Context::SetRuntimeErrorHandler(RuntimeErrorHandlerFn fn) {
        m_RuntimeErrorHandler = fn;
    }

    void Context::SetCompilerErrorHandler(CompilerErrorHandlerFn fn) {
        m_CompilerErrorHandler = fn;
    }

    CompiledSource* Context::GetCompiledSource(const std::string& module) {
        if (module.empty()) {
            BLUA_ASSERT(m_CurrentCompiledSource, "Cannot get any active module!");
            return m_CurrentCompiledSource;
        }

        BLUA_ASSERT(m_Modules.contains(module), "Current context does not contain the requested module!");
        return m_Modules.at(module);
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

        m_CurrentCompiledSource->VM.StopExecution();
    }

    Allocator* Context::GetAllocator() {
        return m_CurrentCompiledSource->Allocator;
    }

} // namespace BlackLua
