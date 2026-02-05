#pragma once

#include "core.hpp"

#include <vector>
#include <variant>
#include <unordered_map>

namespace BlackLua::Internal {

    class VM;
    using ExternFn = void(*)(VM* vm);

    struct StackSlotIndex {
        StackSlotIndex() = default;
        StackSlotIndex(int32_t slot) : Slot(slot) {}
        StackSlotIndex(int32_t slot, size_t offset, size_t size) : Slot(slot), Offset(offset), Size(size) {}

        StackSlotIndex operator-(int32_t lhs) { return StackSlotIndex(Slot - lhs, Offset, Size); }
        bool operator==(int32_t lhs) { return Slot == lhs; }

        int32_t Slot = 0;
        size_t Offset = 0;
        size_t Size = 0;
    };

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
        Dup, // Creates a new stack slot and copies a value into it
        Offset, // Adds a certain offset (specified as a slot) to a specified slot

        Label,
        Jmp,
        Jt, // Perform a jump if the value at the specified slot is true (value must be a boolean)
        Jf, // Perform a jump if the value at the specified slot is false (value must be a boolean)

        Call, // Performs a jump and sets up a scope
        CallExtern, // Calls a native C/C++ function
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
        GteFloating,

        CastIntegralToIntegral,
        CastIntegralToFloating,
        CastFloatingToIntegral,
        CastFloatingToFloating,
    };

    struct OpCodeStore {
        StackSlotIndex SlotIndex{};
        size_t DataSize = 0;
        void* Data = nullptr; // NOTE: The VM should not care what this contains, it should just copy it to a slot
                              // Another note: The VM should not free this memory, it should be part of the arena allocator

        bool SetReadOnly = false;
    };

    struct OpCodeCopy {
        StackSlotIndex DstSlot{};
        StackSlotIndex SrcSlot{};
    };

    struct OpCodeOffset {
        StackSlotIndex Offset{};
        StackSlotIndex Slot{};
        size_t Size = 0;
    };

    struct OpCodeJump {
        StackSlotIndex Slot{};
        int32_t Label = -1;
    };

    struct OpCodeMath {
        StackSlotIndex LHSSlot{};
        StackSlotIndex RHSSlot{};
    };

    struct OpCodeCast {
        StackSlotIndex Value{};
        size_t Size = 0;
    };

    struct OpCode {
        OpCodeType Type = OpCodeType::Invalid;
        std::variant<StackSlotIndex, std::string, OpCodeStore, OpCodeCopy, OpCodeOffset, OpCodeJump, OpCodeMath, OpCodeCast> Data;
        std::string DebugData; // Optional debug data the compiler can provide
    };

    struct StackSlot {
        size_t Index = 0;
        size_t Size = 0;
        bool ReadOnly = false;
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

        void AddExtern(const std::string& signature, ExternFn fn);

        void Call(int32_t label);
        void CallExtern(const std::string& signature);
        
        void StoreBool(StackSlotIndex slot, bool b);
        void StoreChar(StackSlotIndex slot, int8_t c);
        void StoreShort(StackSlotIndex slot, int16_t ch);
        void StoreInt(StackSlotIndex slot, int32_t i);
        void StoreLong(StackSlotIndex slot, int64_t l);
        void StoreFloat(StackSlotIndex slot, float f);
        void StoreDouble(StackSlotIndex slot, double d);

        // Copies the memory at one slot (srcSlot) to another slot (dstSlot)
        void Copy(StackSlotIndex dstSlot, StackSlotIndex srcSlot);

        bool GetBool(StackSlotIndex slot);
        int8_t GetChar(StackSlotIndex slot);
        int16_t GetShort(StackSlotIndex slot);
        int32_t GetInt(StackSlotIndex slot);
        int64_t GetLong(StackSlotIndex slot);
        float GetFloat(StackSlotIndex slot);
        double GetDouble(StackSlotIndex slot);

        void NegateIntegral(StackSlotIndex val);
        void NegateFloating(StackSlotIndex val);

        void AddIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void SubIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void MulIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void DivIntegral(StackSlotIndex lhs, StackSlotIndex rhs);

        void CmpIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void LtIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void LteIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void GtIntegral(StackSlotIndex lhs, StackSlotIndex rhs);
        void GteIntegral(StackSlotIndex lhs, StackSlotIndex rhs);

        void AddFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void SubFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void MulFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void DivFloating(StackSlotIndex lhs, StackSlotIndex rhs);

        void CmpFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void LtFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void LteFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void GtFloating(StackSlotIndex lhs, StackSlotIndex rhs);
        void GteFloating(StackSlotIndex lhs, StackSlotIndex rhs);

        void CastIntegralToIntegral(StackSlotIndex val, size_t size);
        void CastIntegralToFloating(StackSlotIndex val, size_t size);
        void CastFloatingToIntegral(StackSlotIndex val, size_t size);
        void CastFloatingToFloating(StackSlotIndex val, size_t size);

        // Run an array of op codes in the VM, executing each operations one at a time
        void RunByteCode(const OpCode* data, size_t count);
        void Run();

        // NOTE: The "slot" parameter can be either negative or positive
        // If it's negative, it accesses from the top of stack backwards,
        // AKA: return stack[top of stack + slot]
        // If it's positive though, it accesses from the start of the stack,
        // AKA: return stack[slot]
        StackSlot GetStackSlot(StackSlotIndex slot);
        size_t GetStackSlotIndex(int32_t slot);

        // Sets a breakpoint up for when the program counter hits a specified value
        void AddBreakPoint(int32_t pc);

    private:
        void RegisterLables();

        // Helpers to avoid a LOT of repetition (NOTE: They may look quite ugly because they need to handle a lot of possible combinations)
        template <typename T>
        void AddGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void SubGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void MulGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void DivGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void CmpGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void LtGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void LteGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void GtGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void GteGeneric(StackSlotIndex lhs, StackSlotIndex rhs) {
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
        void NegateGeneric(StackSlotIndex value) {
            StackSlot __value = GetStackSlot(value);
            T v{};
            memcpy(&v, &m_Stack[__value.Index], __value.Size);

            v = -v;

            PushBytes(__value.Size);
            StackSlot newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[newSlot.Index], &v, newSlot.Size);
        }

        template <typename T, typename Y>
        void CastGeneric(StackSlotIndex value) {
            StackSlot __value = GetStackSlot(value);
            T v{};
            memcpy(&v, &m_Stack[__value.Index], __value.Size);

            Y t1 = static_cast<Y>(v);

            PushBytes(sizeof(Y));
            StackSlot __newSlot = GetStackSlot(-1);

            memcpy(&m_Stack[__newSlot.Index], &t1, __newSlot.Size);
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

        std::unordered_map<std::string, ExternFn> m_ExternFuncs;

        std::unordered_map<int32_t, bool> m_BreakPoints;
    };

} // namespace BlackLua::Internal
