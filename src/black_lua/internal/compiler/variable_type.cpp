#include "internal/compiler/variable_type.hpp"
#include "context.hpp"

namespace BlackLua::Internal {

    VariableType* CreateVarType(Context* ctx, PrimitiveType type, bool lvalue, decltype(VariableType::Data) data) {
        VariableType* t = ctx->GetAllocator()->AllocateNamed<VariableType>();
        t->Type = type;
        t->LValue = lvalue;
        t->Data = data;
    
        return t;
    }

} // namespace BlackLua::Internal 