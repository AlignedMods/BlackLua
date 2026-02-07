#include "internal/stdlib/string.hpp"

namespace BlackLua::Internal {

    struct String {
        int8_t* DynamicBuffer = nullptr;
        
        int32_t Size = 0;
        int32_t Capacity = 0;
    };

    void bl__string__init__(Context* ctx) {
        String* str = new String();

        // By default the static buffer is used
        str->Size = 0;
        str->Capacity = 1;
        str->DynamicBuffer = new int8_t[1];

        ctx->GetVM().StorePointer(-1, str);
    }

    void bl__string__copy__(Context* ctx) {
        BLUA_ASSERT(false, "TODO");
    }

    void bl__string__destruct__(Context* ctx) {
        String* str = reinterpret_cast<String*>(ctx->GetVM().GetPointer(-1));

        if (str->Size >= 25) {
            delete[] str->DynamicBuffer;
        }

        str->Size = 0;
        str->Capacity = 0;

        delete str;
        ctx->GetVM().StorePointer(-1, nullptr);
    }

    void bl__string__construct_from_literal__(Context* ctx) {
        String* str = reinterpret_cast<String*>(ctx->GetVM().GetPointer(-2));

    }

} // namespace BlackLua::Internal