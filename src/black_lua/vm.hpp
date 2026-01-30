#pragma once

#include "black_lua.hpp"

#include <vector>
#include <variant>
#include <unordered_map>

namespace BlackLua::Internal {

    enum class OpCodeType {
        Invalid,
        Nop,

        PushBytes,
        Pop,
        PushScope,
        PopScope,
        Store,
        Get, // Automaticaly pushes the value onto the stack
        Copy, // Copies a value into another slot

        Label,
        Jmp,
        Jt, // Perform a jump if the value at the specified slot is true (value must be a boolean)
        Jf, // Perform a jump if the value at the specified slot is false (value must be a boolean)

        Call, // Performs a jump and sets up a scope
        Ret, // Performs a jump to the current scopes return address, then pops the current scope
        RetValue, // Does the same as ret but also copies a slot into the given return slot

        NegateIntegral,
        NegateFloating,

        AddIntegral,
        SubIntegral,
        MulIntegral,
        DivIntegral,
        AddFloating,
        SubFloating,
        MulFloating,
        DivFloating,

        CmpIntegral,
        LtIntegral,
        LteIntegral,
        GtIntegral,
        GteIntegral,
        CmpFloating,
        LtFloating,
        LteFloating,
        GtFloating,
        GteFloating
    };

    struct OpCodeStore {
        int32_t SlotIndex = 0;
        void* Data = nullptr; // NOTE: The VM should not care what this contains, it should just copy it to a slot
                              // Another note: The VM should not free this memory, it should be part of the arena allocator
    };

    struct OpCodeCopy {
        int32_t DstSlot = 0;
        int32_t SrcSlot = 0;
    };

    struct OpCodeJump {
        int32_t Slot = 0;
        int32_t Label = -1;
    };

    struct OpCodeCall {
        int32_t Label = -1;
        int32_t ReturnSlot = 0;
    };

    struct OpCodeMath {
        int32_t LHSSlot = 0;
        int32_t RHSSlot = 0;
    };

    struct OpCode {
        OpCodeType Type = OpCodeType::Invalid;
        std::variant<int32_t, OpCodeStore, OpCodeCopy, OpCodeJump, OpCodeCall, OpCodeMath> Data;
        std::string DebugData; // Optional debug data the compiler can provide
    };

    struct StackSlot {
        size_t Index = 0;
        size_t Size = 0;
        bool ReadOnly = false;
        bool Written = false; // Used only for the ReadOnly flag
    }; 

    class VM {
    public:
        VM();

        // Increments the stack pointer by specified amount of bytes
        // Also creates a new stack slot, which gets set as the current stack slot
        void PushBytes(size_t amount);

        // Pops the current stack slot
        void Pop();

        // Creates a new scope
        void PushScope();
        // Removes the current scope and goes back to the previous one (if there is one)
        void PopScope();

        void Call(int32_t label, int32_t returnSlot);
        
        void StoreBool(int32_t slot, bool b);
        void StoreChar(int32_t slot, int8_t c);
        void StoreShort(int32_t slot, int16_t ch);
        void StoreInt(int32_t slot, int32_t i);
        void StoreLong(int32_t slot, int64_t l);
        void StoreFloat(int32_t slot, float f);
        void StoreDouble(int32_t slot, double d);

        // Copies the memory at one slot (srcSlot) to another slot (dstSlot)
        void Copy(int32_t dstSlot, int32_t srcSlot);

        bool GetBool(int32_t slot);
        int8_t GetChar(int32_t slot);
        int16_t GetShort(int32_t slot);
        int32_t GetInt(int32_t slot);
        int64_t GetLong(int32_t slot);
        float GetFloat(int32_t slot);
        double GetDouble(int32_t slot);

        void NegateIntegral(int32_t val);
        void NegateFloating(int32_t val);

        void AddIntegral(int32_t lhs, int32_t rhs);
        void SubIntegral(int32_t lhs, int32_t rhs);
        void MulIntegral(int32_t lhs, int32_t rhs);
        void DivIntegral(int32_t lhs, int32_t rhs);

        void CmpIntegral(int32_t lhs, int32_t rhs);
        void LtIntegral(int32_t lhs, int32_t rhs);
        void LteIntegral(int32_t lhs, int32_t rhs);
        void GtIntegral(int32_t lhs, int32_t rhs);
        void GteIntegral(int32_t lhs, int32_t rhs);

        void AddFloating(int32_t lhs, int32_t rhs);
        void SubFloating(int32_t lhs, int32_t rhs);
        void MulFloating(int32_t lhs, int32_t rhs);
        void DivFloating(int32_t lhs, int32_t rhs);

        void CmpFloating(int32_t lhs, int32_t rhs);
        void LtFloating(int32_t lhs, int32_t rhs);
        void LteFloating(int32_t lhs, int32_t rhs);
        void GtFloating(int32_t lhs, int32_t rhs);
        void GteFloating(int32_t lhs, int32_t rhs);

        // Run an array of op codes in the VM, executing each operations one at a time
        void RunByteCode(const OpCode* data, size_t count);
        void Run();

        // NOTE: The "slot" parameter can be either negative or positive
        // If it's negative, it accesses from the top of stack backwards,
        // AKA: return stack[top of stack + slot]
        // If it's positive though, it accesses from the start of the stack,
        // AKA: return stack[slot]
        StackSlot GetStackSlot(int32_t slot);

    private:
        void RegisterLables();

        // Helpers to avoid a LOT of repetition (NOTE: They may look quite ugly because they need to handle a lot of possible combinations)
        template <typename T>
        void AddGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            T result = l + r;

            PushBytes(__LHS.Size);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void SubGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            T result = l - r;

            PushBytes(__LHS.Size);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void MulGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            T result = l * r;

            PushBytes(__LHS.Size);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void DivGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            T result = l / r;

            PushBytes(__LHS.Size);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void CmpGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            bool result = (l == r);

            PushBytes(1);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void LtGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            bool result = (l < r);

            PushBytes(1);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void LteGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            bool result = (l <= r);

            PushBytes(1);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void GtGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            bool result = (l > r);

            PushBytes(1);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void GteGeneric(int32_t lhs, int32_t rhs) {
            StackSlot __LHS = GetStackSlot(lhs);
            StackSlot __RHS = GetStackSlot(rhs);
            T l{};
            memcpy(&l, &m_Stack[__LHS.Index], __LHS.Size);
            T r{};
            memcpy(&r, &m_Stack[__RHS.Index], __RHS.Size);

            bool result = (l >= r);

            PushBytes(1);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &result, newSlot.Size);
        }

        template <typename T>
        void NegateGeneric(int32_t value) {
            StackSlot __value = GetStackSlot(value);
            T v{};
            memcpy(&v, &m_Stack[__value.Index], __value.Size);

            v = -v;

            PushBytes(__value.Size);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &v, newSlot.Size);
        }

    private:
        std::vector<uint8_t> m_Stack;
        size_t m_StackPointer = 0;
        std::vector<StackSlot> m_StackSlots;
        int32_t m_StackSlotPointer = 0;

        struct Scope {
            Scope* Previous = nullptr;
            size_t Offset = 0;
            size_t SlotOffset = 0;
            size_t ReturnAddress = SIZE_MAX;
            int32_t ReturnSlot = 0;
        };
        Scope* m_CurrentScope = nullptr;
        size_t m_CurrentReturnAdress = SIZE_MAX;

        const OpCode* m_Program = nullptr;
        size_t m_ProgramSize = 0;
        size_t m_ProgramCounter = 0;

        std::unordered_map<int32_t, size_t> m_Labels;
        size_t m_LabelCount = 0;
    };

} // namespace BlackLua::Internal
