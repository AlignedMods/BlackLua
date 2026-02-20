#pragma once

#include "internal/compiler/variable_type.hpp"

#include <variant>
#include <vector>
#include <unordered_map>

namespace BlackLua::Internal {
    class Parser;
    class TypeChecker;
    class Emitter;
    class VM;

    class StringBuilder;
    struct NodeList;
}

namespace BlackLua {

    struct CompiledSource;
    class Allocator;

    using RuntimeErrorHandlerFn = void(*)(const std::string& error);
    using CompilerErrorHandlerFn = void(*)(size_t line, size_t column, const std::string& file, const std::string& error);

    using ExternFn = void(*)(Context* ctx);

    struct Context {
        Context();
        static Context Create();

        void CompileFile(const std::string& path, const std::string& module);
        void CompileString(const std::string& source, const std::string& module);

        // Deallocates the given module
        void FreeModule(const std::string& module);

        // Run the compiled string in the VM
        // Dissasemble the byte emitted byte code
        void Run(const std::string& module);

        std::string DumpAST(const std::string& module);
        // Returns a string containing the disassembled byte code
        std::string Disassemble(const std::string& module);

        void PushBool(bool b    , const std::string& module = {});
        void PushChar(int8_t c  , const std::string& module = {});
        void PushShort(int16_t s, const std::string& module = {});
        void PushInt(int32_t i  , const std::string& module = {});
        void PushLong(int64_t l , const std::string& module = {});
        void PushFloat(float f  , const std::string& module = {});
        void PushDouble(double f, const std::string& module = {});

        void PushGlobal(const std::string& str, const std::string& module = {});
        void Pop(size_t count, const std::string& module = {});

        bool    GetBool(int32_t index,   const std::string& module = {});
        int8_t  GetChar(int32_t index,   const std::string& module = {});
        int16_t GetShort(int32_t index,  const std::string& module = {});
        int32_t GetInt(int32_t index,    const std::string& module = {});
        int64_t GetLong(int32_t index,   const std::string& module = {});
        float   GetFloat(int32_t index,  const std::string& module = {});
        double  GetDouble(int32_t index, const std::string& module = {});

        void AddExternalFunction(const std::string& name, ExternFn fn, const std::string& module);
        void Call(const std::string& str, const std::string& module);

        void SetRuntimeErrorHandler(RuntimeErrorHandlerFn fn);
        void SetCompilerErrorHandler(CompilerErrorHandlerFn fn);

    private:
        CompiledSource* GetCompiledSource(const std::string& module);

        void ReportCompilerError(size_t line, size_t column, const std::string& error);
        void ReportRuntimeError(const std::string& error);

        Allocator* GetAllocator();

        friend class Internal::Parser;
        friend class Internal::TypeChecker;
        friend class Internal::Emitter;
        friend class Internal::VM;

        friend class Internal::StringBuilder;
        friend struct Internal::NodeList;
        friend Internal::VariableType* Internal::CreateVarType(Context* ctx, Internal::PrimitiveType type, decltype(Internal::VariableType::Data) data);

    private:
        std::unordered_map<std::string, CompiledSource*> m_Modules;
        CompiledSource* m_CurrentCompiledSource = nullptr;

        RuntimeErrorHandlerFn m_RuntimeErrorHandler = nullptr;
        CompilerErrorHandlerFn m_CompilerErrorHandler = nullptr;
    };

} // namespace BlackLua
