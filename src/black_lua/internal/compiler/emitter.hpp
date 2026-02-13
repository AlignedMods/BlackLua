#pragma once

#include "internal/compiler/ast.hpp"
#include "internal/vm.hpp"
#include "internal/compiler/reflection/compiler_reflection.hpp"

namespace BlackLua::Internal {

    struct CompileStackSlot {
        StackSlotIndex Slot = 0;
        bool Relative = false;
    };

    class Emitter {
        public:
            using OpCodes = std::vector<OpCode>;

            static Emitter Emit(const ASTNodes* nodes, Context* ctx);
            const CompilerReflectionData& GetReflectionData() const;
            const OpCodes& GetOpCodes() const;
    
        private:
            void EmitImpl();
    
            Node* Peek(size_t amount = 0);
            Node* Consume();
    
            void EmitConstant(Node* node);
    
            void EmitNodeScope(Node* node);
    
            void EmitNodeVarDecl(Node* node);
            void EmitNodeParamDecl(Node* node);

            void EmitNodeFunctionDecl(Node* node);

            void EmitNodeStructDecl(Node* node);
    
            void EmitNodeWhile(Node* node);
    
            void EmitNodeIf(Node* node);
    
            void EmitNodeReturn(Node* node);
    
            CompileStackSlot EmitNodeExpression(Node* node);
    
            StackSlotIndex CompileToRuntimeStackSlot(CompileStackSlot slot);
    
            int32_t CreateLabel(const std::string& debugData = {});
            void PushBytes(size_t bytes, const std::string& debugData = {});
            void IncrementStackSlotCount();

            // This function expects the object is already pushed on the stack!
            void CallConstructor(VariableType* type);
    
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
                CompileStackSlot ReturnSlot;
                std::unordered_map<std::string, Declaration> DeclaredSymbols;
            };
    
            Scope* m_CurrentScope = nullptr;

            CompilerReflectionData m_ReflectionData;
            Context* m_Context = nullptr;
        };

} // namespace BlackLua::Internal