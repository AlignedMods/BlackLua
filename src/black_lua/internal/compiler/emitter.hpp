#pragma once

#include "internal/compiler/ast.hpp"
#include "internal/vm.hpp"

namespace BlackLua::Internal {

    struct CompileStackSlot {
        StackSlotIndex Slot = 0;
        bool Relative = false;
    };

    class Emitter {
        public:
            using OpCodes = std::vector<OpCode>;

            static Emitter Emit(const ASTNodes* nodes);
            const OpCodes& GetOpCodes() const;
    
        private:
            void EmitImpl();
    
            Node* Peek(size_t amount = 0);
            Node* Consume();
    
            void EmitConstant(Node* node);
    
            void EmitNodeScope(Node* node);
    
            void EmitNodeVarDecl(Node* node);
            void EmitNodeVarArrayDecl(Node* node);
            void EmitNodeFunctionDecl(Node* node);
    
            void EmitNodeFunctionImpl(Node* node);
    
            void EmitNodeWhile(Node* node);
    
            void EmitNodeIf(Node* node);
    
            void EmitNodeReturn(Node* node);
    
            CompileStackSlot EmitNodeExpression(Node* node);
    
            StackSlotIndex CompileToRuntimeStackSlot(CompileStackSlot slot);
    
            int32_t CreateLabel(const std::string& debugData = {});
            void PushBytes(size_t bytes, const std::string& debugData = {});
            void IncrementStackSlotCount();
    
            void EmitNode(Node* node);
    
        private:
            OpCodes m_OpCodes;
            size_t m_Index = 0;
            const ASTNodes* m_Nodes;
    
            size_t m_SlotCount = 0;
            size_t m_LabelCount = 0;
            std::unordered_map<Node*, CompileStackSlot> m_ConstantMap;
    
            struct Declaration {
                int32_t Index = 0;
                size_t Size = 0;
                VariableType* Type = nullptr;
                bool Extern = false;
            };
    
            std::unordered_map<std::string, Declaration> m_DeclaredSymbols;
    
            struct Scope {
                Scope* Parent = nullptr;
                size_t SlotCount = 0;
                std::unordered_map<std::string, Declaration> DeclaredSymbols;
            };
    
            Scope* m_CurrentScope = nullptr;
        };

} // namespace BlackLua::Internal