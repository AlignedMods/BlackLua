#pragma once

#include "internal/vm.hpp"

#include <variant>
#include <vector>
#include <unordered_map>

namespace BlackLua {

    struct CompiledSource;

    struct Context {
        static Context Create();

        CompiledSource* CompileFile(const std::string& path);
        CompiledSource* CompileString(const std::string& source);

        // Deallocates the compiled source
        void FreeSource(CompiledSource* source);

        // Run the compiled string in the VM
        void Run(CompiledSource* compiled, const std::string& module);
        // Dissasemble the byte emitted byte code
        // Returns a string containing the disassembled byte code
        std::string Disassemble(CompiledSource* compiled);

        Internal::VM& GetVM();

    private:
        std::unordered_map<std::string, int> m_Modules;
        Internal::VM m_VM;
    };

} // namespace BlackLua
