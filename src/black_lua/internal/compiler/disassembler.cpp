#include "internal/compiler/disassembler.hpp"

#include <string>
#include <stdio.h>

namespace BlackLua::Internal {

    Disassembler Disassembler::Disassemble(const std::vector<OpCode>* opcodes) {
        Disassembler d;
        d.m_OpCodes = opcodes;
        d.DisassembleImpl();
        return d;
    }

    std::string& Disassembler::GetDisassembly() {
        return m_Output;
    }

    void Disassembler::DisassembleImpl() {
        for (const auto& op : *m_OpCodes) {
            DisassembleOpCode(op);
        }
    }

    void Disassembler::DisassembleOpCode(const OpCode& op) {
        #define CASE_MATH(__op__, __str__) case OpCodeType::__op__: { \
            OpCodeMath m = std::get<OpCodeMath>(op.Data); \
            m_Output += m_Indentation; \
            m_Output += __str__; \
            DisassembleStackSlotIndex(m.LHSSlot); m_Output += ' '; DisassembleStackSlotIndex(m.RHSSlot); \
            m_Output += '\n'; \
            break; \
        }

        #define CASE_CAST(__op__, __str__) case OpCodeType::__op__: { \
            OpCodeCast c = std::get<OpCodeCast>(op.Data); \
            m_Output += fmt::format("{}{}", m_Indentation, __str__); \
            DisassembleStackSlotIndex(c.Value); \
            m_Output += fmt::format(" {}\n", c.Size); \
            break; \
        }

        switch (op.Type) {
            case OpCodeType::Nop: m_Output += "nop\n"; break;

            case OpCodeType::PushBytes: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += m_Indentation;
                m_Output += "push ";
                m_Output += std::to_string(i.Slot);

                if (!op.DebugData.empty()) {
                    m_Output += "    ; ";
                    m_Output += op.DebugData;
                }

                m_Output += '\n';

                break;
            }

            case OpCodeType::Pop: m_Output += m_Indentation; m_Output += "pop\n"; break;
            case OpCodeType::PushScope: m_Output += m_Indentation; m_Output += "push scope\n"; break;
            case OpCodeType::PopScope: m_Output += m_Indentation; m_Output += "pop scope\n"; break;

            case OpCodeType::Store: {
                OpCodeStore s = std::get<OpCodeStore>(op.Data);

                m_Output += m_Indentation;
                m_Output += "store ";
                DisassembleStackSlotIndex(s.SlotIndex);

                m_Output += " <0x";
                const uint8_t* bytes = reinterpret_cast<const uint8_t*>(s.Data);

                for (size_t i = 0; i < s.DataSize; i++) {
                    m_Output += fmt::format("{:02x}", bytes[i]);
                }

                m_Output += ">\n";

                break;
            }

            case OpCodeType::Get: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += m_Indentation;
                m_Output += "get ";
                DisassembleStackSlotIndex(i);
                m_Output += '\n';

                break;
            }

            case OpCodeType::Copy: {
                OpCodeCopy c = std::get<OpCodeCopy>(op.Data);

                m_Output += m_Indentation;
                m_Output += "copy dst ";
                DisassembleStackSlotIndex(c.DstSlot);
                m_Output += " src ";
                DisassembleStackSlotIndex(c.SrcSlot);
                m_Output += '\n';

                break;
            }

            case OpCodeType::Dup: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += m_Indentation;
                m_Output += "dup ";
                DisassembleStackSlotIndex(i);
                m_Output += '\n';

                break;
            }

            case OpCodeType::Offset: {
                OpCodeOffset o = std::get<OpCodeOffset>(op.Data);

                m_Output += m_Indentation;
                m_Output += "offset ";
                DisassembleStackSlotIndex(o.Slot);
                DisassembleStackSlotIndex(o.Offset);
                m_Output += std::to_string(o.Size);
                m_Output += '\n';

                break;
            }

            case OpCodeType::Label: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += '\n';
                m_Output += std::to_string(i.Slot);
                m_Output += ":\n";

                m_Indentation.clear();
                m_Indentation.append(4, ' ');

                break;
            }

            case OpCodeType::Jmp: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += m_Indentation;
                m_Output += "jmp ";
                m_Output += std::to_string(i.Slot);
                m_Output += '\n';

                break;
            }

            case OpCodeType::Jt: {
                OpCodeJump j = std::get<OpCodeJump>(op.Data);

                m_Output += m_Indentation;
                m_Output += "jt ";
                DisassembleStackSlotIndex(j.Slot);
                m_Output += " ";
                m_Output += std::to_string(j.Label);

                break;
            }

            case OpCodeType::Jf: {
                OpCodeJump j = std::get<OpCodeJump>(op.Data);

                m_Output += m_Indentation;
                m_Output += "jf ";
                DisassembleStackSlotIndex(j.Slot);
                m_Output += " ";
                m_Output += std::to_string(j.Label);

                break;
            }

            case OpCodeType::Call: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += m_Indentation;
                m_Output += "call ";
                m_Output += std::to_string(i.Slot);
                m_Output += '\n';

                break;
            }

            case OpCodeType::CallExtern: {
                std::string s = std::get<std::string>(op.Data);

                m_Output += m_Indentation;
                m_Output += "call extern ";
                m_Output += s;
                m_Output += '\n';

                break;
            }

            case OpCodeType::Ret: m_Output += m_Indentation; m_Output += "ret\n"; break;

            case OpCodeType::RetValue: {
                StackSlotIndex i = std::get<StackSlotIndex>(op.Data);

                m_Output += m_Indentation;
                m_Output += "ret value ";
                DisassembleStackSlotIndex(i);
                m_Output += '\n';

                break;
            }

            CASE_MATH(AddIntegral, "add int ");
            CASE_MATH(SubIntegral, "sub int ");
            CASE_MATH(MulIntegral, "mul int ");
            CASE_MATH(DivIntegral, "div int ");

            CASE_MATH(CmpIntegral, "cmp int ");
            CASE_MATH(LtIntegral, "lt int ");
            CASE_MATH(LteIntegral, "lte int ");
            CASE_MATH(GtIntegral, "gt int ");
            CASE_MATH(GteIntegral, "gte int ");

            CASE_MATH(AddFloating, "add float ");
            CASE_MATH(SubFloating, "sub float ");
            CASE_MATH(MulFloating, "mul float ");
            CASE_MATH(DivFloating, "div float ");

            CASE_MATH(CmpFloating, "cmp float ");
            CASE_MATH(LtFloating, "lt float ");
            CASE_MATH(LteFloating, "lte float ");
            CASE_MATH(GtFloating, "gt float ");
            CASE_MATH(GteFloating, "gte float ");

            CASE_CAST(CastIntegralToIntegral, "cast itoi ");
            CASE_CAST(CastIntegralToFloating, "cast itof ");
            CASE_CAST(CastFloatingToIntegral, "cast ftoi ");
            CASE_CAST(CastFloatingToFloating, "cast ftof ");
        }

        #undef CASE_MATH
        #undef CASE_CAST
    }

    void Disassembler::DisassembleStackSlotIndex(const StackSlotIndex& i) {
        if (i.Offset == 0 && i.Size == 0) {
            m_Output += "%(";
            m_Output += std::to_string(i.Slot);
            m_Output += ")";
        } else {
            m_Output += "(";
            m_Output += std::to_string(i.Slot);
            m_Output += ", ";
            m_Output += std::to_string(i.Offset);
            m_Output += ", ";
            m_Output += std::to_string(i.Size);
            m_Output += ")";
        }
    }

} // namespace BlackLua::Internal