#include "internal/vm.hpp"
#include "allocator.hpp"

namespace BlackLua::Internal {

    template <typename T>
    T Add(T lhs, T rhs) { return lhs + rhs; }
    template <typename T>
    T Sub(T lhs, T rhs) { return lhs - rhs; }
    template <typename T>
    T Mul(T lhs, T rhs) { return lhs * rhs; }
    template <typename T>
    T Div(T lhs, T rhs) { return lhs / rhs; }
    template <std::integral T>
    T Mod(T lhs, T rhs) { return lhs % rhs; }
    template <std::floating_point T>
    T Mod(T lhs, T rhs) { return std::fmod(lhs, rhs); }

    template <typename T>
    T Cmp(T lhs, T rhs) { return lhs == rhs; }
    template <typename T>
    T Lt(T lhs, T rhs) { return lhs < rhs; }
    template <typename T>
    T Lte(T lhs, T rhs) { return lhs <= rhs; }
    template <typename T>
    T Gt(T lhs, T rhs) { return lhs > rhs; }
    template <typename T>
    T Gte(T lhs, T rhs) { return lhs >= rhs; }

    VM::VM(Context* ctx) {
        m_Stack.resize(4 * 1024 * 1024); // 4MB stack by default
        m_StackSlots.resize(1024); // 1024 slots by default

        m_Context = ctx;
    }

    void VM::PushBytes(size_t amount) {
        size_t alignedAmount = ((amount + 8 - 1) / 8) * 8; // We need to handle 8 byte alignment since some CPU's will require it
        
        BLUA_ASSERT(alignedAmount % 8 == 0, "Memory not aligned to 8 bytes correctly!");
        BLUA_ASSERT(m_StackPointer + alignedAmount < m_Stack.max_size(), "Stack overflow, allocating an insane amount of memory!");

        // Keep doubling the stack until it's big enough
        while (m_StackPointer + alignedAmount > m_Stack.size()) {
            m_Stack.resize(m_Stack.size() * 2);
        }

        m_StackPointer += alignedAmount;

        if (m_StackSlotPointer + 1 >= m_StackSlots.size()) {
            m_StackSlots.resize(m_StackSlots.size() * 2);
        }

        m_StackSlots[m_StackSlotPointer] = {&m_Stack[m_StackPointer - alignedAmount], amount};
        m_StackSlotPointer++;
    }

    void VM::Pop() {
        BLUA_ASSERT(m_StackPointer > 0, "Calling Pop() on an empty stack!");
        StackSlot s = GetStackSlot(-1);

        m_StackPointer -= s.Size;
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

    void VM::AddExtern(const std::string& signature, ExternFn fn) {
        m_ExternFuncs[signature] = fn;
    }

    void VM::Call(int32_t label) {
        // Perform a jump
        size_t pc = m_ProgramCounter;

        BLUA_ASSERT(m_Labels.contains(label), "Trying to jump to an unknown label!");
        m_ProgramCounter = m_Labels.at(label) + 1;

        PushScope();

        m_CurrentScope->ReturnAddress = pc;
        m_CurrentScope->ReturnSlot = m_StackSlotPointer;

        Run();
    }

    void VM::CallExtern(const std::string& signature) {
        BLUA_ASSERT(m_ExternFuncs.contains(signature), "Calling CallExtern() on a non-existent extern function!");

        // Do the call
        m_ExternFuncs.at(signature)(m_Context);
    }

    void VM::StoreBool(StackSlotIndex slot, bool b) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 1, "Cannot store a bool in a stack slot with a size that isn't 1!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        int8_t bb = static_cast<int8_t>(b);
        memcpy(s.Memory, &bb, 1);
    }

    void VM::StoreChar(StackSlotIndex slot, int8_t c) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 1, "Cannot store a char in a stack slot with a size that isn't 1!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &c, 1);
    }

    void VM::StoreShort(StackSlotIndex slot, int16_t sh) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 2, "Cannot store a short in a stack slot with a size that isn't 2!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &sh, 2);
    }

    void VM::StoreInt(StackSlotIndex slot, int32_t i) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 4, "Cannot store an int in a stack slot with a size that isn't 4!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &i, 4);
    }

    void VM::StoreLong(StackSlotIndex slot, int64_t l) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 8, "Cannot store a long in a stack slot with a size that isn't 8!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &l, 8);
    }

    void VM::StoreFloat(StackSlotIndex slot, float f) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 4, "Cannot store a float in a stack slot with a size that isn't 4!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &f, 4);
    }

    void VM::StoreDouble(StackSlotIndex slot, double d) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == 8, "Cannot store a double in a stack slot with a size that isn't 8!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &d, 8);
    }

    void VM::StorePointer(StackSlotIndex slot, void* p) {
        StackSlot s = GetStackSlot(slot);
        
        BLUA_ASSERT(s.Size == sizeof(void*), "Cannot store a double in a stack slot with a size that isn't sizeof(void*)!");
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");

        memcpy(s.Memory, &p, sizeof(void*));
    }

    void VM::SetMemory(StackSlotIndex slot, void* data) {
        StackSlot s = GetStackSlot(slot);
        BLUA_ASSERT(!s.ReadOnly, "Trying to store data into a read only slot!");
        m_StackSlots[GetStackSlotIndex(slot.Slot)].Memory = data;
    }

    void* VM::GetMemory(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);
        return s.Memory;
    }

    void VM::Copy(StackSlotIndex dstSlot, StackSlotIndex srcSlot) {
        StackSlot dst = GetStackSlot(dstSlot);
        StackSlot src = GetStackSlot(srcSlot);

        BLUA_ASSERT(dst.Size == src.Size, "Invalid Copy() call, sizes of both slots must be the same!");
        BLUA_ASSERT(!dst.ReadOnly, "Trying to copy data into a read only slot!");

        memcpy(dst.Memory, src.Memory, src.Size);
    }

    bool VM::GetBool(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 1, "Invalid GetBool() call!");

        bool b = false;
        memcpy(&b, s.Memory, 1);
        return b;
    }

    int8_t VM::GetChar(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 1, "Invalid GetChar() call!");

        int8_t c = 0;
        memcpy(&c, s.Memory, 1);
        return c;
    }

    int16_t VM::GetShort(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 2, "Invalid GetShort() call!");

        int16_t sh = 0;
        memcpy(&sh, s.Memory, 2);
        return sh;
    }

    int32_t VM::GetInt(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 4, "Invalid GetInt() call!");

        int32_t i = 0;
        memcpy(&i, s.Memory, 4);
        return i;
    }

    int64_t VM::GetLong(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 8, "Invalid GetLong() call!");

        int64_t l = 0;
        memcpy(&l, s.Memory, 8);
        return l;
    }

    float VM::GetFloat(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 4, "Invalid GetFloat() call!");

        float f = 0;
        memcpy(&f, s.Memory, 4);
        return f;
    }

    double VM::GetDouble(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == 8, "Invalid GetDouble() call!");

        double d = 0;
        memcpy(&d, s.Memory, 8);
        return d;
    }

    void* VM::GetPointer(StackSlotIndex slot) {
        StackSlot s = GetStackSlot(slot);

        BLUA_ASSERT(s.Size == sizeof(void*), "Invalid GetPointer() call!");

        void* p = nullptr;
        memcpy(&p, s.Memory, sizeof(void*));
        return p;
    }

    void VM::RunByteCode(const OpCode* data, size_t count) {
        m_Program = data;
        m_ProgramSize = count;

        RegisterLables();

        Run();
    }

    void VM::Run() {
        #define CASE_MATH(_enum, _builtinType, langName, _builtinOp) case OpCodeType::_enum: { \
            OpCodeMath m = std::get<OpCodeMath>(op.Data); \
            _builtinType lhs = Get##langName(m.LHSSlot); \
            _builtinType rhs = Get##langName(m.RHSSlot); \
            _builtinType result = _builtinOp(lhs, rhs); \
            PushBytes(sizeof(_builtinType)); \
            StackSlot s = GetStackSlot(-1); \
            memcpy(s.Memory, &result, sizeof(_builtinType)); \
            break; \
        }

        #define CASE_MATH_GROUP(_mathop, op) \
            CASE_MATH(_mathop##I8,  int8_t,   Char,   op) \
            CASE_MATH(_mathop##I16, int16_t,  Short,  op) \
            CASE_MATH(_mathop##I32, int32_t,  Int,    op) \
            CASE_MATH(_mathop##I64, int64_t,  Long,   op) \
            CASE_MATH(_mathop##U8,  uint8_t,  Char,   op) \
            CASE_MATH(_mathop##U16, uint16_t, Short,  op) \
            CASE_MATH(_mathop##U32, uint32_t, Int,    op) \
            CASE_MATH(_mathop##U64, uint64_t, Long,   op) \
            CASE_MATH(_mathop##F32, float,    Float,  op) \
            CASE_MATH(_mathop##F64, double,   Double, op)

        #define CASE_CAST(_enum, sourceType, destType) case OpCodeType::_enum: { \
            StackSlotIndex v = std::get<StackSlotIndex>(op.Data); \
            StackSlot s = GetStackSlot(v); \
            sourceType t{}; \
            memcpy(&t, s.Memory, sizeof(sourceType)); \
            destType d = static_cast<destType>(t); \
            PushBytes(sizeof(destType)); \
            StackSlot __a = GetStackSlot(-1); \
            memcpy(__a.Memory, &d, sizeof(destType)); \
            break; \
        }

        #define CASE_CAST_GROUP(_cast, _builtinType) \
            CASE_CAST(Cast##_cast##ToI8,  _builtinType, int8_t) \
            CASE_CAST(Cast##_cast##ToI16, _builtinType, int16_t) \
            CASE_CAST(Cast##_cast##ToI32, _builtinType, int32_t) \
            CASE_CAST(Cast##_cast##ToI64, _builtinType, int64_t) \
            CASE_CAST(Cast##_cast##ToU8,  _builtinType, uint8_t) \
            CASE_CAST(Cast##_cast##ToU16, _builtinType, uint16_t) \
            CASE_CAST(Cast##_cast##ToU32, _builtinType, uint32_t) \
            CASE_CAST(Cast##_cast##ToU64, _builtinType, uint64_t) \
            CASE_CAST(Cast##_cast##ToF32, _builtinType, float) \
            CASE_CAST(Cast##_cast##ToF64, _builtinType, double)

        for (; m_ProgramCounter < m_ProgramSize; m_ProgramCounter++) {
            const OpCode& op = m_Program[m_ProgramCounter];

            if (m_BreakPoints.contains(m_ProgramCounter)) {
                __debugbreak();
            }

            switch (op.Type) {
                case OpCodeType::Invalid: { BLUA_ASSERT(false, "Unreachable!"); break; }
                case OpCodeType::Nop: { continue; }

                case OpCodeType::PushBytes: {
                    size_t byteCount = static_cast<size_t>(std::get<StackSlotIndex>(op.Data).Slot);

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

                    BLUA_ASSERT(!s.ReadOnly, "Trying to store data in a read only slot!");

                    memcpy(s.Memory, store.Data, s.Size);

                    m_StackSlots[GetStackSlotIndex(store.SlotIndex.Slot)].ReadOnly = store.SetReadOnly;

                    break;
                }

                case OpCodeType::Get: {
                    StackSlotIndex slot = std::get<StackSlotIndex>(op.Data);
                    StackSlot s = GetStackSlot(slot);
                    PushBytes(s.Size);

                    memcpy(GetStackSlot(-1).Memory, s.Memory, s.Size);

                    break;
                }

                case OpCodeType::Copy: {
                    OpCodeCopy copy = std::get<OpCodeCopy>(op.Data);
                    StackSlot dst = GetStackSlot(copy.DstSlot);
                    StackSlot src = GetStackSlot(copy.SrcSlot);

                    BLUA_ASSERT(dst.Size == src.Size, "Sizes of both slots must be the same when copying!");
                    BLUA_ASSERT(!dst.ReadOnly, "Trying to copy data into a read only slot!");

                    memcpy(dst.Memory, src.Memory, src.Size);

                    break;
                }

                case OpCodeType::Dup: {
                    StackSlotIndex slot = std::get<StackSlotIndex>(op.Data);
                    StackSlot src = GetStackSlot(slot);

                    PushBytes(src.Size);
                    StackSlot dst = GetStackSlot(-1);
                    
                    memcpy(dst.Memory, src.Memory, src.Size);

                    break;
                }

                case OpCodeType::Offset: {
                    OpCodeOffset off = std::get<OpCodeOffset>(op.Data);
                    int32_t offset = GetInt(off.Offset);
                    StackSlot slot = GetStackSlot(off.Slot);

                    BLUA_ASSERT(offset < slot.Size, "Offset out of bounds!");

                    StackSlot s;
                    s.Memory = reinterpret_cast<uint8_t*>(slot.Memory) + offset;
                    s.ReadOnly = slot.ReadOnly;
                    s.Size = off.Size;
                    m_StackSlots[m_StackSlotPointer] = s;
                    m_StackSlotPointer++;

                    break;
                }

                // When we encounter a label, instantly stop execution
                case OpCodeType::Label: m_ProgramCounter = m_ProgramSize - 1; break;

                case OpCodeType::Jmp: {
                    int32_t labelIdentifier = std::get<StackSlotIndex>(op.Data).Slot;

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
                    int32_t label = std::get<StackSlotIndex>(op.Data).Slot;
                    Call(label);

                    break;
                }

                case OpCodeType::CallExtern: {
                    std::string sig = std::get<std::string>(op.Data);
                    CallExtern(sig);

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
                    StackSlotIndex slot = std::get<StackSlotIndex>(op.Data);

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

                CASE_MATH_GROUP(Add, Add)
                CASE_MATH_GROUP(Sub, Sub)
                CASE_MATH_GROUP(Mul, Mul)
                CASE_MATH_GROUP(Div, Div)
                CASE_MATH_GROUP(Mod, Mod)

                CASE_MATH_GROUP(Cmp, Cmp)
                CASE_MATH_GROUP(Lt, Lt)
                CASE_MATH_GROUP(Lte, Lte)
                CASE_MATH_GROUP(Gt, Gt)
                CASE_MATH_GROUP(Gte, Gte)

                CASE_CAST_GROUP(I8, int8_t)
                CASE_CAST_GROUP(I16, int16_t)
                CASE_CAST_GROUP(I32, int32_t)
                CASE_CAST_GROUP(I64, int64_t)
                CASE_CAST_GROUP(U8, uint8_t)
                CASE_CAST_GROUP(U16, uint16_t)
                CASE_CAST_GROUP(U32, uint32_t)
                CASE_CAST_GROUP(U64, uint64_t)
                CASE_CAST_GROUP(F32, float)
                CASE_CAST_GROUP(F64, double)
            }
        }

        #undef CASE_MATH
        #undef CASE_MATH_GROUP
        #undef CASE_CAST
        #undef CASE_CAST_GROUP
    }

    StackSlot VM::GetStackSlot(StackSlotIndex slot) {
        StackSlot s = m_StackSlots[GetStackSlotIndex(slot.Slot)];
        s.Memory = reinterpret_cast<uint8_t*>(s.Memory) + slot.Offset;
        
        if (slot.Size != 0) {
            s.Size = slot.Size;
        }

        return s;
    }

    size_t VM::GetStackSlotIndex(int32_t slot) {
        if (slot < 0) {
            BLUA_ASSERT(m_StackSlotPointer + slot >= 0, "Out of range slot!");

            return m_StackSlotPointer + slot;
        } else if (slot > 0) {
            BLUA_ASSERT((slot - 1) < m_StackSlotPointer, "Out of range slot!");
            return slot - 1;
        } else {
            BLUA_ASSERT(false, "Slot cannot be 0!");
        }

        return 0;
    }

    void VM::AddBreakPoint(int32_t pc) {
        m_BreakPoints[pc] = true;
    }

    void VM::StopExecution() {
        m_ProgramCounter = m_ProgramSize;
    }

    void VM::RegisterLables() {
        for (; m_ProgramCounter < m_ProgramSize; m_ProgramCounter++) {
            const OpCode& op = m_Program[m_ProgramCounter];

            if (op.Type == OpCodeType::Label) {
                int32_t labelIdentifier = std::get<StackSlotIndex>(op.Data).Slot;

                m_Labels[labelIdentifier] = m_ProgramCounter;
                m_LabelCount++;
            }
        }

        m_ProgramCounter = 0; // Reset the program counter so the normal execution happens from the start
    }

} // namespace BlackLua::Internal
