#pragma once

#include "internal/vm.hpp"

#include <variant>
#include <vector>
#include <unordered_map>

namespace BlackLua {

    struct CompiledSource;
    using RuntimeErrorHandlerFn = void(*)(const std::string& error);
    using CompilerErrorHandlerFn = void(*)(size_t line, size_t column, const std::string& file, const std::string& error);

    struct Context {
        Context();
        static Context Create();

        CompiledSource* CompileFile(const std::string& path);
        CompiledSource* CompileString(const std::string& source);

        // Deallocates the compiled source
        void FreeSource(CompiledSource* source);

        // Run the compiled string in the VM
        // Dissasemble the byte emitted byte code
        void Run(CompiledSource* compiled, const std::string& module);
        // Returns a string containing the disassembled byte code
        std::string Disassemble(CompiledSource* compiled);

        void SetRuntimeErrorHandler(RuntimeErrorHandlerFn fn);
        void SetCompilerErrorHandler(CompilerErrorHandlerFn fn);

        void ReportCompilerError(size_t line, size_t column, const std::string& error);
        // Calls the currently set error handler with the given error message
        // If the current error handler is NULL, it will use the default handler
        void ReportRuntimeError(const std::string& error);

        Internal::VM& GetVM();

    private:
        std::unordered_map<std::string, int> m_Modules;
        Internal::VM m_VM;
        std::string m_CurrentFile; // Current file being compiled

        RuntimeErrorHandlerFn m_RuntimeErrorHandler = nullptr;
        CompilerErrorHandlerFn m_CompilerErrorHandler = nullptr;
    };

} // namespace BlackLua
