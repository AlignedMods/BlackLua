#include "vm.hpp"

namespace BlackLua::Internal {

    VM::VM() {
        m_Stack.resize(4 * 1024 * 1024); // 4MB stack by default
        m_StackSlots.resize(1024); // 1024 slots by default
    }

    void VM::PushBytes(size_t amount) {
        size_t alignedAmount = ((amount + 8 - 1) / 8) * 8; // We need to handle 8 byte alignment since some CPU's will require it
        
        BLUA_ASSERT(alignedAmount % 8 == 0, "Memory not aligned to 8 bytes correctly!");
        BLUA_ASSERT(m_StackPointer + alignedAmount < m_Stack.max_size(), "Stack overflow, allocating an insane amount of memory!");

        // Keep doubling the stack until it's big enough
        while (m_StackPointer + alignedAmount > m_Stack.size()) {
            BLUA_FORMAT_PRINT("Doubling stack size!");
            m_Stack.resize(m_Stack.size() * 2);
        }

        BLUA_FORMAT_PRINT("Pushing {} bytes, aligned to {}!", amount, alignedAmount);

        m_StackPointer += alignedAmount;

        if (m_StackSlotPointer + 1 >= m_StackSlots.size()) {
            m_StackSlots.resize(m_StackSlots.size() * 2);
        }

        m_StackSlots[m_StackSlotPointer] = {m_StackPointer - alignedAmount, amount};
        m_StackSlotPointer++;
    }

    void VM::Pop() {
        BLUA_ASSERT(m_StackPointer > 0, "Calling Pop() on an empty stack!");
        StackSlot s = GetStackSlot(-1);

        m_StackPointer = s.Index;
        m_StackSlotPointer--;
    }

    void VM::PushScope() {
        BLUA_FORMAT_PRINT("PushScope(), stack pointer: {}, slot pointer: {}", m_StackPointer, m_StackSlotPointer);

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

        BLUA_FORMAT_PRINT("PopScope(), stack pointer: {}, slot pointer: {}", m_StackPointer, m_StackSlotPointer);
    }

    void VM::Call(int32_t label, size_t paramCount, size_t returnCount) {
        // Perform a jump
        size_t pc = m_ProgramCounter;

        BLUA_ASSERT(m_Labels.contains(label), "Trying to jump to an unknown label!");
        m_ProgramCounter = m_Labels.at(label) + 1;

        PushScope();

        // for (size_t i = 0; i < paramCount; i++) {
        //     int32_t dst = -(i + 1);
        //     int32_t src = -(i + paramCount + returnCount);
        // 
        //     Copy(dst, src);
        // }

        m_CurrentScope->ReturnAddress = pc;
        m_CurrentScope->ReturnSlot = m_StackSlotPointer;

        Run();
    }

    void VM::StoreBool(int32_t slot, bool b) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 1, "Cannot store a bool in a stack slot with a size that isn't 1!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        int8_t bb = static_cast<int8_t>(b);
        memcpy(&m_Stack[s.Index], &bb, 1);
    }

    void VM::StoreChar(int32_t slot, int8_t c) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 1, "Cannot store a char in a stack slot with a size that isn't 1!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(&m_Stack[s.Index], &c, 1);
    }

    void VM::StoreShort(int32_t slot, int16_t sh) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 2, "Cannot store a short in a stack slot with a size that isn't 2!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(&m_Stack[s.Index], &sh, 2);
    }

    void VM::StoreInt(int32_t slot, int32_t i) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 4, "Cannot store an int in a stack slot with a size that isn't 4!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(&m_Stack[s.Index], &i, 4);
    }

    void VM::StoreLong(int32_t slot, int64_t l) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 8, "Cannot store a long in a stack slot with a size that isn't 8!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(&m_Stack[s.Index], &l, 8);
    }

    void VM::StoreFloat(int32_t slot, float f) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 4, "Cannot store a float in a stack slot with a size that isn't 4!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(&m_Stack[s.Index], &f, 4);
    }

    void VM::StoreDouble(int32_t slot, double d) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 8, "Cannot store a double in a stack slot with a size that isn't 8!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(&m_Stack[s.Index], &d, 8);
    }

    void VM::Copy(int32_t dstSlot, int32_t srcSlot) {
        StackSlot dst = GetStackSlot(dstSlot);
        StackSlot src = GetStackSlot(srcSlot);

        BLUA_ASSERT(dst.Size == src.Size, "Invalid Copy() call, sizes of both slots must be the same!");
        BLUA_ASSERT(!dst.ReadOnly, "Trying to copy data into a read only slot!");

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

    void VM::NegateIntegral(int32_t val) {
        StackSlot value = GetStackSlot(val);

        switch (value.Size) {
            case 1: {
                NegateGeneric<int8_t>(val);
                break;
            }

            case 2: {
                NegateGeneric<int16_t>(val);
                break;
            }

            case 4: {
                NegateGeneric<int32_t>(val);
                break;
            }

            case 8: {
                NegateGeneric<int64_t>(val);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::NegateFloating(int32_t val) {
        StackSlot value = GetStackSlot(val);

        switch (value.Size) {
            case 4: {
                NegateGeneric<float>(val);
                break;
            }

            case 8: {
                NegateGeneric<double>(val);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
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

    void VM::CmpIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid CmpIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                CmpGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                CmpGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                CmpGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                CmpGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::LtIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid LtIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                LtGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                LtGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                LtGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                LtGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::LteIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid LteIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                LteGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                LteGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                LteGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                LteGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::GtIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid GtIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                GtGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                GtGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                GtGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                GtGeneric<int64_t>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::GteIntegral(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid GteIntegral() call, sizes of both sides must be the same!");

        switch (lhsSlot.Size) {
            case 1: {
                GteGeneric<int8_t>(lhs, rhs);
                break;
            }

            case 2: {
                GteGeneric<int16_t>(lhs, rhs);
                break;
            }

            case 4: {
                GteGeneric<int32_t>(lhs, rhs);
                break;
            }

            case 8: {
                GteGeneric<int64_t>(lhs, rhs);
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

    void VM::CmpFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid CmpFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                CmpGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                CmpGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::LtFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid LtFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                LtGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                LtGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::LteFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid LteFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                LteGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                LteGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::GtFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid GtFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                GtGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                GtGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::GteFloating(int32_t lhs, int32_t rhs) {
        StackSlot lhsSlot = GetStackSlot(lhs);
        StackSlot rhsSlot = GetStackSlot(rhs);

        BLUA_ASSERT(lhsSlot.Size == rhsSlot.Size, "Invalid GteFloating() call, sizes of both operands must be the same!");

        switch (lhsSlot.Size) {
            case 4: {
                GteGeneric<float>(lhs, rhs);
                break;
            }

            case 8: {
                GteGeneric<double>(lhs, rhs);
                break;
            }

            default: BLUA_ASSERT(false, "Unsupported sizes of operands!");
        }
    }

    void VM::RunByteCode(const OpCode* data, size_t count) {
        m_Program = data;
        m_ProgramSize = count;

        RegisterLables();

        Run();
    }

    void VM::Run() {
        BLUA_FORMAT_PRINT("Running code!");

        #define CASE_MATH(type) case OpCodeType::type: {     \
            OpCodeMath math = std::get<OpCodeMath>(op.Data); \
            type(math.LHSSlot, math.RHSSlot);                \
            break;                                           \
        }

        for (; m_ProgramCounter < m_ProgramSize; m_ProgramCounter++) {
            const OpCode& op = m_Program[m_ProgramCounter];

            if (m_BreakPoints.contains(m_ProgramCounter)) {
                __debugbreak();
            }

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
                    StackSlot& s = GetStackSlot(store.SlotIndex);

                    BLUA_ASSERT(!s.ReadOnly, "Trying to store data in a read only slot!");

                    memcpy(&m_Stack[s.Index], store.Data, s.Size);
                    s.ReadOnly = store.SetReadOnly;

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

                    BLUA_ASSERT(dst.Size == src.Size, "Sizes of both slots must be the same when copying!");
                    BLUA_ASSERT(!dst.ReadOnly, "Trying to copy data into a read only slot!");

                    memcpy(&m_Stack[dst.Index], &m_Stack[src.Index], src.Size);

                    break;
                }

                case OpCodeType::Dup: {
                    int32_t slot = std::get<int32_t>(op.Data);
                    StackSlot src = GetStackSlot(slot);

                    PushBytes(src.Size);
                    StackSlot dst = GetStackSlot(-1);
                    
                    memcpy(&m_Stack[dst.Index], &m_Stack[src.Index], src.Size);

                    break;
                }

                // When we encounter a label, instantly stop execution
                case OpCodeType::Label: m_ProgramCounter = m_ProgramSize - 1; break;

                case OpCodeType::Jmp: {
                    int32_t labelIdentifier = std::get<int32_t>(op.Data);

                    BLUA_ASSERT(m_Labels.contains(labelIdentifier), "Trying to jump to an unknown label!");
                    m_ProgramCounter = m_Labels.at(labelIdentifier);

                    break;
                }

                case OpCodeType::Jt: {
                    OpCodeJump jump = std::get<OpCodeJump>(op.Data);

                    if (GetBool(jump.Slot) == true) {
                        BLUA_ASSERT(m_Labels.contains(jump.Label), "Trying to jump to an unknown label!");
                        m_ProgramCounter = m_Labels.at(jump.Label);
                    }

                    break;
                }

                case OpCodeType::Jf: {
                    OpCodeJump jump = std::get<OpCodeJump>(op.Data);

                    if (GetBool(jump.Slot) == false) {
                        BLUA_ASSERT(m_Labels.contains(jump.Label), "Trying to jump to an unknown label!");
                        m_ProgramCounter = m_Labels.at(jump.Label);
                    }

                    break;
                }

                case OpCodeType::Call: {
                    OpCodeCall call = std::get<OpCodeCall>(op.Data);
                    Call(call.Label, call.ParamCount, call.ReturnCount);

                    break;
                }

                case OpCodeType::Ret: {
                    BLUA_ASSERT(m_CurrentScope, "Trying to return out of no scope!");

                    // Keep popping scopes until we find the function scope
                    while (m_CurrentScope->ReturnAddress == SIZE_MAX) {
                        BLUA_ASSERT(m_CurrentScope->Previous, "Trying return out of non function scope!");

                        PopScope();
                    }

                    m_ProgramCounter = m_CurrentScope->ReturnAddress;
                    PopScope();

                    break;
                }

                case OpCodeType::RetValue: {
                    int32_t slot = std::get<int32_t>(op.Data);

                    BLUA_ASSERT(m_CurrentScope, "Trying to return out of no scope!");

                    Copy(m_CurrentScope->ReturnSlot, slot);

                    // Keep popping scopes until we find the function scope
                    while (m_CurrentScope->ReturnAddress == SIZE_MAX) {
                        BLUA_ASSERT(m_CurrentScope->Previous, "Trying return out of non function scope!");

                        PopScope();
                    }

                    m_ProgramCounter = m_CurrentScope->ReturnAddress;
                    PopScope();

                    break;
                }

                case OpCodeType::NegateIntegral: {
                    int32_t slot = std::get<int32_t>(op.Data);

                    NegateIntegral(slot);
                    break;
                }

                case OpCodeType::NegateFloating: {
                    int32_t slot = std::get<int32_t>(op.Data);

                    NegateFloating(slot);
                    break;
                }

                CASE_MATH(AddIntegral);
                CASE_MATH(SubIntegral);
                CASE_MATH(MulIntegral);
                CASE_MATH(DivIntegral);

                CASE_MATH(CmpIntegral);
                CASE_MATH(LtIntegral);
                CASE_MATH(LteIntegral);
                CASE_MATH(GtIntegral);
                CASE_MATH(GteIntegral);

                CASE_MATH(AddFloating);
                CASE_MATH(SubFloating);
                CASE_MATH(MulFloating);
                CASE_MATH(DivFloating);

                CASE_MATH(CmpFloating);
                CASE_MATH(LtFloating);
                CASE_MATH(LteFloating);
                CASE_MATH(GtFloating);
                CASE_MATH(GteFloating);
            }
        }

        #undef CASE_MATH
    }

    StackSlot& VM::GetStackSlot(int32_t slot) {
        if (slot < 0) {
             BLUA_ASSERT(m_StackSlotPointer + slot >= 0, "Out of range slot!");

             return m_StackSlots[m_StackSlotPointer + slot];
        } else if (slot > 0) {
            BLUA_ASSERT((slot - 1) < m_StackSlotPointer, "Out of range slot!");
            return m_StackSlots[slot - 1];
        } else {
            BLUA_ASSERT(false, "Slot cannot be 0!");
        }
    }

    void VM::AddBreakPoint(int32_t pc) {
        m_BreakPoints[pc] = true;
    }

    void VM::RegisterLables() {
        for (; m_ProgramCounter < m_ProgramSize; m_ProgramCounter++) {
            const OpCode& op = m_Program[m_ProgramCounter];

            if (op.Type == OpCodeType::Label) {
                int32_t labelIdentifier = std::get<int32_t>(op.Data);

                m_Labels[labelIdentifier] = m_ProgramCounter;
                m_LabelCount++;
            }
        }

        m_ProgramCounter = 0; // Reset the program counter so the normal execution happens from the start
    }

} // namespace BlackLua::Internal
