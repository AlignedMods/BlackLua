#include "internal/compiler/type_info.hpp"
#include "context.hpp"

namespace BlackLua::Internal {

    TypeInfo* TypeInfo::Create(Context* ctx, PrimitiveType type, decltype(TypeInfo::Data) data) {
        TypeInfo* t = ctx->GetAllocator()->AllocateNamed<TypeInfo>();
        t->Type = type;
        t->Data = data;
    
        return t;
    }

} // namespace BlackLua::Internal 