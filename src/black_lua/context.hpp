#pragma once

#include "internal/vm.hpp"

#include <variant>
#include <vector>
#include <unordered_map>

namespace BlackLua {

    struct CompiledSource;
    class Allocator;

    using RuntimeErrorHandlerFn = void(*)(const std::string& error);
    using CompilerErrorHandlerFn = void(*)(size_t line, size_t column, const std::string& file, const std::string& error);

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

        void Call(const std::string& str, const std::string& module);

        void SetRuntimeErrorHandler(RuntimeErrorHandlerFn fn);
        void SetCompilerErrorHandler(CompilerErrorHandlerFn fn);

        void ReportCompilerError(size_t line, size_t column, const std::string& error);
        // Calls the currently set error handler with the given error message
        // If the current error handler is NULL, it will use the default handler
        void ReportRuntimeError(const std::string& error);

        Allocator* GetAllocator();
        Internal::VM& GetVM();

    private:
        std::unordered_map<std::string, CompiledSource*> m_Modules;
        Internal::VM m_VM;
        CompiledSource* m_CurrentCompiledSource = nullptr;

        RuntimeErrorHandlerFn m_RuntimeErrorHandler = nullptr;
        CompilerErrorHandlerFn m_CompilerErrorHandler = nullptr;
    };

} // namespace BlackLua
