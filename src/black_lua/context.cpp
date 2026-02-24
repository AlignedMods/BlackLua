#include "context.hpp"
#include "internal/compiler/lexer.hpp"
#include "internal/compiler/parser.hpp"
#include "internal/compiler/semantic_analyzer/semantic_analyzer.hpp"
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

        Internal::VariableType* m_CurrentStackTop = nullptr;
    };

    Context::Context() {}

    Context Context::Create() {
        Context ctx{};
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

        Internal::Lexer l({ src->SourceCode.c_str(), src->SourceCode.size() });

        Internal::Parser p(l.GetTokens(), this);
        valid = p.IsValid();
        if (!valid) { delete src->Allocator; return; }
        src->ASTNodes = *p.GetNodes();

        Internal::SemanticAnalyzer c(&src->ASTNodes, this);
        if (!valid) { BLUA_ASSERT(false, "f"); delete src->Allocator; return; }

        Internal::Emitter e(p.GetNodes(), this);
        src->ReflectionData = e.GetReflectionData();
        src->OpCodes = e.GetOpCodes();

        src->VM.AddExtern("bl__array__init__", BlackLua::Internal::bl__array__init__);
        src->VM.AddExtern("bl__array__destruct__", BlackLua::Internal::bl__array__destruct__);
        src->VM.AddExtern("bl__array__copy__", BlackLua::Internal::bl__array__copy__);

        src->VM.AddExtern("bl__array__append__", BlackLua::Internal::bl__array__append__);
        src->VM.AddExtern("bl__array__index__", BlackLua::Internal::bl__array__index__);

        src->VM.AddExtern("bl__string__construct__", BlackLua::Internal::bl__string__construct__);
        src->VM.AddExtern("bl__string__construct_from_literal__", BlackLua::Internal::bl__string__construct_from_literal__);

        src->VM.AddExtern("bl__string__copy__", BlackLua::Internal::bl__string__copy__);
        src->VM.AddExtern("bl__string__assign__", BlackLua::Internal::bl__string__assign__);

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

        Internal::ASTDumper d(&src->ASTNodes);
        return d.GetOutput();
    }

    std::string Context::Disassemble(const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        Internal::Disassembler d(&src->OpCodes);
        return d.GetDisassembly();
    }

    void Context::PushBool(bool b, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(b), Internal::CreateVarType(this, Internal::PrimitiveType::Bool));
        src->VM.StoreBool(-1, b);
    }

    void Context::PushChar(int8_t c, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(c), Internal::CreateVarType(this, Internal::PrimitiveType::Char));
        src->VM.StoreChar(-1, c);
    }

    void Context::PushShort(int16_t s, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(s), Internal::CreateVarType(this, Internal::PrimitiveType::Short));
        src->VM.StoreShort(-1, s);
    }

    void Context::PushInt(int32_t i, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(i), Internal::CreateVarType(this, Internal::PrimitiveType::Int));
        src->VM.StoreInt(-1, i);
    }

    void Context::PushLong(int64_t l, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(l), Internal::CreateVarType(this, Internal::PrimitiveType::Long));
        src->VM.StoreLong(-1, l);
    }

    void Context::PushFloat(float f, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(f), Internal::CreateVarType(this, Internal::PrimitiveType::Float));
        src->VM.StoreFloat(-1, f);
    }

    void Context::PushDouble(double d, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(d), Internal::CreateVarType(this, Internal::PrimitiveType::Double));
        src->VM.StoreDouble(-1, d);
    }

    void Context::PushPointer(void* p, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.PushBytes(sizeof(p), nullptr);
        src->VM.StorePointer(-1, p);
    }

    void Context::StoreBool(size_t index, bool b, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreBool(index, b);
    }

    void Context::StoreChar(size_t index, int8_t c, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreChar(index, c);
    }

    void Context::StoreShort(size_t index, int16_t s, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreShort(index, s);
    }

    void Context::StoreInt(size_t index, int32_t i, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreInt(index, i);
    }

    void Context::StoreLong(size_t index, int64_t l, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreLong(index, l);
    }

    void Context::StoreFloat(size_t index, float f, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreFloat(index, f);
    }

    void Context::StoreDouble(size_t index, double d, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StoreDouble(index, d);
    }

    void Context::StorePointer(size_t index, void* p, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.StorePointer(index, p);
    }

    void Context::PushGlobal(const std::string& str, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        BLUA_ASSERT(src->ReflectionData.Declarations.contains(str), "Trying to push an unknown global variable");
        BLUA_ASSERT(src->ReflectionData.Declarations.at(str).Type == Internal::ReflectionType::Variable, "Trying to push a non-variable global (perhaps a function)");

        src->VM.Ref(std::get<int32_t>(src->ReflectionData.Declarations.at(str).Data), src->ReflectionData.Declarations.at(str).ResolvedType);
    }

    void Context::Pop(size_t count, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        for (size_t i = 0; i < count; i++) {
            src->VM.Pop();
        }
    }

    void Context::PushField(int32_t index, const std::string& name, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);

        Internal::StackSlot slot = src->VM.GetStackSlot(index);
        
        BLUA_ASSERT(slot.ResolvedType->Type == Internal::PrimitiveType::Structure, "Accessing a field can only happen with a struct");
        
        for (const auto& field : std::get<Internal::StructDeclaration>(slot.ResolvedType->Data).Fields) {
            if (Internal::StringView(name.data(), name.size()) == field.Identifier) {
                src->VM.Ref({index, field.Offset, GetTypeSize(field.ResolvedType)}, field.ResolvedType);
            }
        }

        ReportRuntimeError(fmt::format("Could find field {} in {}", name, Internal::VariableTypeToString(slot.ResolvedType)));
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

    void* Context::GetPointer(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        return src->VM.GetPointer(index);
    }

    StackSlot Context::GetStackSlot(int32_t index, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        Internal::StackSlot slot = src->VM.GetStackSlot(index);
        return { slot.Memory, slot.Size };
    }

    void Context::AddExternalFunction(const std::string& name, ExternFn fn, const std::string& module) {
        CompiledSource* src = GetCompiledSource(module);
        src->VM.AddExtern(name, fn);
    }

    void Context::Call(const std::string& str, const std::string& module) {
        BLUA_ASSERT(m_Modules.contains(module), "Current context does not contain the requested module!");
        CompiledSource* src = m_Modules.at(module);

        BLUA_ASSERT(src->ReflectionData.Declarations.contains(str), "Trying to call an unknown function");
        BLUA_ASSERT(src->ReflectionData.Declarations.at(str).Type == Internal::ReflectionType::Function, "Trying to call an non-function");
        size_t size = GetTypeSize(src->ReflectionData.Declarations.at(str).ResolvedType);
        if (size > 0) {
            src->VM.PushBytes(size, src->ReflectionData.Declarations.at(str).ResolvedType);
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

    void Context::ReportCompilerError(size_t line, size_t column, size_t startLine, size_t startColumn, size_t endLine, size_t endColumn, const std::string& error) {
        if (m_CompilerErrorHandler) {
            m_CompilerErrorHandler(line, column, startLine, startColumn, endLine, endColumn, m_CurrentCompiledSource->Module, error);
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
