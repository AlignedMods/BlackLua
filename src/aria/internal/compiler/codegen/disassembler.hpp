#pragma once

#include "aria/internal/vm/op_codes.hpp"

#include <vector>

namespace Aria::Internal {

    class Disassembler {
    public:
        Disassembler(const std::vector<OpCode>* opcodes);

        std::string& GetDisassembly();

    private:
        void DisassembleImpl();
        void DisassembleOpCode(const OpCode& op);

        std::string DisassembleMemRef(const MemRef& mem);

    private:
        const std::vector<OpCode>* m_OpCodes;

        std::string m_Output;
        std::string m_Indentation;
    };

} // namespace Aria::Internal
