#include "internal/compiler/ast/node_list.hpp"
#include "context.hpp"
#include "internal/allocator.hpp"

namespace BlackLua::Internal {

    void NodeList::Append(Context* ctx, Node* n) {
        if (Capacity == 0) {
            Items = reinterpret_cast<Node**>(ctx->GetAllocator()->Allocate(sizeof(Node*) * 1));
            Capacity = 1;
        }
    
        if (Size >= Capacity) {
            Capacity *= 2;
    
            Node** newItems = reinterpret_cast<Node**>(ctx->GetAllocator()->Allocate(sizeof(Node*) * Capacity));
            memcpy(newItems, Items, sizeof(Node*) * Size);
            Items = newItems;
        }
    
        Items[Size] = n;
        Size++;
    }

} // namespace BlackLua::Internal