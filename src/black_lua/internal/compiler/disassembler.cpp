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
        #define CASE_MATH(__op__, _op_str_, __str__) case OpCodeType::__op__: { \
            OpCodeMath m = std::get<OpCodeMath>(op.Data); \
            m_Output += fmt::format("{}{} {} ", m_Indentation, _op_str_, __str__); \
            DisassembleStackSlotIndex(m.LHSSlot); m_Output += ' '; DisassembleStackSlotIndex(m.RHSSlot); \
            m_Output += '\n'; \
            break; \
        }

        #define CASE_MATH_GROUP(_mathop, _str_) \
            CASE_MATH(_mathop##I8,  _str_, "i8") \
            CASE_MATH(_mathop##I16, _str_, "i16") \
            CASE_MATH(_mathop##I32, _str_, "i32") \
            CASE_MATH(_mathop##I64, _str_, "i64") \
            CASE_MATH(_mathop##U8,  _str_, "u8") \
            CASE_MATH(_mathop##U16, _str_, "u16") \
            CASE_MATH(_mathop##U32, _str_, "u32") \
            CASE_MATH(_mathop##U64, _str_, "u64") \
            CASE_MATH(_mathop##F32, _str_, "f32") \
            CASE_MATH(_mathop##F64, _str_, "f64")

        #define CASE_CAST(__op__, _op_str_, __str__) case OpCodeType::__op__: { \
            StackSlotIndex i = std::get<StackSlotIndex>(op.Data); \
            m_Output += fmt::format("{}cast {} {} ", m_Indentation, _op_str_, __str__); \
            DisassembleStackSlotIndex(i); \
            m_Output += '\n'; \
            break; \
        }

        #define CASE_CAST_GROUP(_cast, _str_) \
            CASE_CAST(Cast##_cast##ToI8,  _str_, "i8") \
            CASE_CAST(Cast##_cast##ToI16, _str_, "i16") \
            CASE_CAST(Cast##_cast##ToI32, _str_, "i32") \
            CASE_CAST(Cast##_cast##ToI64, _str_, "i64") \
            CASE_CAST(Cast##_cast##ToU8,  _str_, "u8") \
            CASE_CAST(Cast##_cast##ToU16, _str_, "u16") \
            CASE_CAST(Cast##_cast##ToU32, _str_, "u32") \
            CASE_CAST(Cast##_cast##ToU64, _str_, "u64") \
            CASE_CAST(Cast##_cast##ToF32, _str_, "f32") \
            CASE_CAST(Cast##_cast##ToF64, _str_, "f64")

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

            CASE_MATH_GROUP(Add, "add")
            CASE_MATH_GROUP(Sub, "sub")
            CASE_MATH_GROUP(Mul, "mul")
            CASE_MATH_GROUP(Div, "div")

            CASE_MATH_GROUP(Cmp, "cmp")
            CASE_MATH_GROUP(Lt, "lt")
            CASE_MATH_GROUP(Lte, "lte")
            CASE_MATH_GROUP(Gt, "gt")
            CASE_MATH_GROUP(Gte, "gte")

            CASE_CAST_GROUP(I8, "i8");
            CASE_CAST_GROUP(I16, "i16");
            CASE_CAST_GROUP(I32, "i32");
            CASE_CAST_GROUP(I64, "i64");
            CASE_CAST_GROUP(U8, "u8");
            CASE_CAST_GROUP(U16, "u16");
            CASE_CAST_GROUP(U32, "u32");
            CASE_CAST_GROUP(U64, "u64");
            CASE_CAST_GROUP(F32, "f32");
            CASE_CAST_GROUP(F64, "f64");
        }

        #undef CASE_MATH
        #undef CASE_MATH_GROUP
        #undef CASE_CAST
        #undef CASE_CAST_GROUP
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