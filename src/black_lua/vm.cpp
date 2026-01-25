#include "vm.hpp"

namespace BlackLua::Internal {

    void VM::PushBytes(size_t amount) {
        size_t alignedAmount = amount + (8 % amount); // We need to handle 8 byte alignment since some CPU's will require it
        m_Stack.reserve(m_StackPointer + alignedAmount);
        m_StackPointer += alignedAmount;

        m_StackSlots.emplace_back(m_StackPointer - alignedAmount, amount);
        m_StackSlotPointer++;
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

    StackSlot VM::GetStackSlot(int32_t slot) {
        BLUA_ASSERT(m_StackSlotPointer > m_StackSlotPointer + slot, "Out of range slot!");

        return m_StackSlots[m_StackSlotPointer + slot];
    }

} // namespace BlackLua::Internal
