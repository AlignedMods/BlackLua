#include "aria/internal/compiler/types/type_info.hpp"

namespace Aria::Internal {

    TypeInfo* TypeInfo::Create(CompilationContext* ctx, PrimitiveType type, decltype(TypeInfo::Data) data) {
        TypeInfo* t = ctx->Allocate<TypeInfo>();
        t->Type = type;
        t->Data = data;
    
        return t;
    }

} // namespace Aria::Internal 
