#include "vm.hpp"

namespace BlackLua::Internal {

    VM::VM() {
        m_Stack.resize(4 * 1024 * 1024); // 4MB stack by default
    }

    void VM::PushBytes(size_t amount) {
        size_t alignedAmount = amount + (amount % 8); // We need to handle 8 byte alignment since some CPU's will require it
        
        BLUA_ASSERT(m_StackPointer + alignedAmount < m_Stack.max_size(), "Stack overflow, allocating an insane amount of memory!");

        // Keep doubling the stack until it's big enough
        while (m_StackPointer + alignedAmount > m_Stack.size()) {
            BLUA_FORMAT_PRINT("Doubling stack size!");
            m_Stack.resize(m_Stack.size() * 2);
        }

        m_StackPointer += alignedAmount;

        m_StackSlots.emplace_back(m_StackPointer - alignedAmount, amount);
        m_StackSlotPointer++;
    }

    void VM::Pop() {
        BLUA_ASSERT(m_StackPointer > 0, "Calling Pop() on an empty stack!");
        StackSlot s = GetStackSlot(-1);

        m_StackPointer = s.Index;
        m_StackSlotPointer--;
    }

    void VM::PushScope() {
        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Previous = m_CurrentScope;
        newScope->Offset = m_StackPointer;
        newScope->SlotOffset = m_StackSlotPointer;

        m_CurrentScope = newScope;
    }

    void VM::PopScope() {
        BLUA_ASSERT(m_CurrentScope, "Calling PopScope() with no active scope!");

        m_StackPointer = m_CurrentScope->Offset;
        m_StackSlotPointer = m_CurrentScope->SlotOffset;
        m_CurrentScope = m_CurrentScope->Previous;
    }

    void VM::StoreBool(int32_t slot, bool b) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 1, "Cannot store a bool in a stack slot with a size that isn't 1!");

        int8_t bb = static_cast<int8_t>(b);
        memcpy(&m_Stack[s.Index], &bb, 1);
    }

    void VM::StoreChar(int32_t slot, int8_t c) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 1, "Cannot store a char in a stack slot with a size that isn't 1!");

        memcpy(&m_Stack[s.Index], &c, 1);
    }

    void VM::StoreShort(int32_t slot, int16_t sh) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 2, "Cannot store a short in a stack slot with a size that isn't 2!");

        memcpy(&m_Stack[s.Index], &sh, 2);
    }

    void VM::StoreInt(int32_t slot, int32_t i) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 4, "Cannot store an int in a stack slot with a size that isn't 4!");

        memcpy(&m_Stack[s.Index], &i, 4);
    }

    void VM::StoreLong(int32_t slot, int64_t l) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 8, "Cannot store a long in a stack slot with a size that isn't 8!");

        memcpy(&m_Stack[s.Index], &l, 8);
    }

    void VM::StoreFloat(int32_t slot, float f) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 4, "Cannot store a float in a stack slot with a size that isn't 4!");

        memcpy(&m_Stack[s.Index], &f, 4);
    }

    void VM::StoreDouble(int32_t slot, double d) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 8, "Cannot store a double in a stack slot with a size that isn't 8!");

        memcpy(&m_Stack[s.Index], &d, 8);
    }

    void VM::Copy(int32_t dstSlot, int32_t srcSlot) {
        StackSlot dst = GetStackSlot(dstSlot);
        StackSlot src = GetStackSlot(srcSlot);

        BLUA_ASSERT(dst.Size == src.Size, "Invalid Copy() call, sizes of both slots must be the same!");

        memcpy(&m_Stack[dst.Index], &m_Stack[src.Index], src.Size);
    }

    bool VM::GetBool(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 1, "Invalid GetBool() call!");

        bool b = false;
        memcpy(&b, &m_Stack[s.Index], 1);
        return b;
    }

    int8_t VM::GetChar(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 1, "Invalid GetChar() call!");

        int8_t c = 0;
        memcpy(&c, &m_Stack[s.Index], 1);
        return c;
    }

    int16_t VM::GetShort(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 2, "Invalid GetShort() call!");

        int16_t sh = 0;
        memcpy(&sh, &m_Stack[s.Index], 2);
        return sh;
    }

    int32_t VM::GetInt(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 4, "Invalid GetInt() call!");

        int32_t i = 0;
        memcpy(&i, &m_Stack[s.Index], 4);
        return i;
    }

    int64_t VM::GetLong(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 8, "Invalid GetLong() call!");

        int64_t l = 0;
        memcpy(&l, &m_Stack[s.Index], 8);
        return l;
    }

    float VM::GetFloat(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 4, "Invalid GetFloat() call!");

        float f = 0;
        memcpy(&f, &m_Stack[s.Index], 4);
        return f;
    }

    double VM::GetDouble(int32_t slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 8, "Invalid GetDouble() call!");

        double d = 0;
        memcpy(&d, &m_Stack[s.Index], 8);
        return d;
    }

    void VM::AddIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid AddIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                AddGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                AddGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                AddGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                AddGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::SubIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid SubIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                SubGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                SubGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                SubGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                SubGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::MulIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid MulIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                MulGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                MulGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                MulGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                MulGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::DivIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid DivIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                DivGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                DivGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                DivGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                DivGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::AddFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid AddFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                AddGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                AddGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::SubFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid SubFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                SubGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                SubGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::MulFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid MulFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                MulGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                MulGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::DivFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid DivFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                DivGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                DivGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::RunByteCode(OpCode* data, size_t count) {
        m_Program = data;
        m_ProgramSize = count;

        #define CASE_MATH(type) case OpCodeType::type: {     \
            OpCodeMath math = std::get<OpCodeMath>(op.Data); \
            type(math.LHSSlot, math.RHSSlot);                \
            break;                                           \
        }

        for (size_t i = 0; i < m_ProgramSize; i++) {
            OpCode& op = m_Program[i];

            switch (op.Type) {
                case OpCodeType::Invalid: { BLUA_ASSERT(false, "Unreachable!"); break; }
                case OpCodeType::Nop: { continue; }

                case OpCodeType::PushBytes: {
                    size_t byteCount = static_cast<size_t>(std::get<int32_t>(op.Data));

                    PushBytes(byteCount);
                    break;
                }

                case OpCodeType::Pop: {
                    Pop();
                    break;
                }

                case OpCodeType::PushScope: {
                    PushScope();
                    break;
                }

                case OpCodeType::PopScope: {
                    PopScope();
                    break;
                }

                case OpCodeType::Store: {
                    OpCodeStore store = std::get<OpCodeStore>(op.Data);
                    StackSlot s = GetStackSlot(store.SlotIndex);
                    memcpy(&m_Stack[s.Index], store.Data, s.Size);

                    break;
                }

                case OpCodeType::Get: {
                    int32_t slot = std::get<int32_t>(op.Data);
                    StackSlot s = GetStackSlot(slot);
                    PushBytes(s.Size);

                    memcpy(&m_Stack[GetStackSlot(-1).Index], &m_Stack[s.Index], s.Size);

                    break;
                }

                case OpCodeType::Copy: {
                    OpCodeCopy copy = std::get<OpCodeCopy>(op.Data);
                    StackSlot dst = GetStackSlot(copy.DstSlot);
                    StackSlot src = GetStackSlot(copy.SrcSlot);

                    memcpy(&m_Stack[dst.Index], &m_Stack[src.Index], src.Size);

                    break;
                }

                CASE_MATH(AddIntegral);
                CASE_MATH(SubIntegral);
                CASE_MATH(MulIntegral);
                CASE_MATH(DivIntegral);
                CASE_MATH(AddFloating);
                CASE_MATH(SubFloating);
                CASE_MATH(MulFloating);
                CASE_MATH(DivFloating);
            }
        }

        #undef CASE_MATH
    }

    StackSlot VM::GetStackSlot(int32_t slot) {
        if (slot < 0) {
            BLUA_ASSERT(m_StackSlotPointer > m_StackSlotPointer + slot, "Out of range slot!");

             return m_StackSlots[m_StackSlotPointer + slot];
        } else if (slot > 0) {
            BLUA_ASSERT(slot < m_StackSlotPointer, "Out of range slot!");
            return m_StackSlots[slot - 1];
        } else {
            BLUA_ASSERT(false, "Slot cannot be 0!");
        }
    }

} // namespace BlackLua::Internal
