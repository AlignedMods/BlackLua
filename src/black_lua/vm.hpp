#pragma once

#include "black_lua.hpp"

#include <vector>

namespace BlackLua::Internal {

    struct StackSlot {
        size_t Index = 0;
        size_t Size = 0;
    }; 

    class VM {
    public:
        // Increments the stack pointer by specified amount of bytes
        // Also creates a new stack slot, which gets set as the current stack slot
        void PushBytes(size_t amount);
        
        // Stores a 32 bit integer in a stack slot
        // NOTE: This function does NOT allocate stack space!
        void StoreBool(int32_t slot, bool b);
        void StoreChar(int32_t slot, int8_t c);
        void StoreShort(int32_t slot, int16_t ch);
        void StoreInt(int32_t slot, int32_t i);
        void StoreLong(int32_t slot, int64_t l);
        void StoreFloat(int32_t slot, float f);
        void StoreDouble(int32_t slot, double d);

        bool GetBool(int32_t slot);
        int8_t GetChar(int32_t slot);
        int16_t GetShort(int32_t slot);
        int32_t GetInt(int32_t slot);
        int64_t GetLong(int32_t slot);
        float GetFloat(int32_t slot);
        double GetDouble(int32_t slot);

        StackSlot GetStackSlot(int32_t slot);

    private:
        std::vector<uint8_t> m_Stack;
        size_t m_StackPointer = 0;
        std::vector<StackSlot> m_StackSlots;
        int32_t m_StackSlotPointer = 0;
    };

} // namespace BlackLua::Internal
