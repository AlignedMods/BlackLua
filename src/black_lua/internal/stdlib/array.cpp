#include "internal/stdlib/array.hpp"

namespace BlackLua::Internal {

    struct Array {
        void* Data = nullptr;
        int32_t MemberSize = 0;

        int32_t Size = 0;
        int32_t Capacity = 0;
    };

    void bl__array__init__(Context* ctx) {
        // Array* arr = new Array();
        // StackSlot size = ctx->GetVM().GetStackSlot(-1);
        // 
        // arr->Size = 0;
        // arr->Capacity = 1;
        // arr->MemberSize = size.Size;
        // arr->Data = new uint8_t[size.Size];
        // 
        // ctx->GetVM().StorePointer(-2, arr);
        BLUA_ASSERT(false, "TODO");
    }

    void bl__array__copy__(Context* ctx) {
        // Array* dst = reinterpret_cast<Array*>(ctx->GetVM().GetPointer(-2));
        // Array* src = reinterpret_cast<Array*>(ctx->GetVM().GetPointer(-1));
        // 
        // dst->Size = src->Size;
        // dst->MemberSize = src->MemberSize;
        // dst->Data = new uint8_t[dst->Size * dst->MemberSize];
        // memcpy(dst->Data, src->Data, dst->Size * dst->MemberSize);

        BLUA_ASSERT(false, "TODO");
    }

    void bl__array__destruct__(Context* ctx) {
        // Array* arr = reinterpret_cast<Array*>(ctx->GetVM().GetPointer(-1));
        // 
        // delete[] arr->Data;
        // arr->Size = 0;
        // arr->Capacity = 0;
        // 
        // delete arr;
        // ctx->GetVM().StorePointer(-1, nullptr);
        BLUA_ASSERT(false, "TODO");
    }

    void bl__array__index__(Context* ctx) {
        // Array* arr = reinterpret_cast<Array*>(ctx->GetVM().GetPointer(-3));
        // int32_t index = ctx->GetVM().GetInt(-2);
        // 
        // ctx->GetVM().SetMemory(-1, reinterpret_cast<uint8_t*>(arr->Data) + index * arr->MemberSize);
        BLUA_ASSERT(false, "TODO");
    }

} // namespace BlackLua::Internal