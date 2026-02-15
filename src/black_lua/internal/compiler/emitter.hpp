#pragma once

#include "internal/compiler/ast/expr.hpp"
#include "internal/compiler/ast/stmt.hpp"
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

            Node* Peek();
            Node* Consume();
    
            void EmitConstantExpr(NodeExpr* expr);
            void EmitConstantStatement(NodeStmt* stmt);

            void EmitConstant(Node* node);
    
            StackSlotIndex CompileToRuntimeStackSlot(CompileStackSlot slot);
    
            int32_t CreateLabel(const std::string& debugData = {});
            void PushBytes(size_t bytes, const std::string& debugData = {});
            void IncrementStackSlotCount();

            CompileStackSlot EmitNodeExpression(NodeExpr* expr);

            void EmitNodeScope(NodeStmt* stmt);
    
            void EmitNodeVarDecl(NodeStmt* stmt);
            void EmitNodeParamDecl(NodeStmt* stmt);

            void EmitNodeFunctionDecl(NodeStmt* stmt);

            void EmitNodeStructDecl(NodeStmt* stmt);
    
            void EmitNodeWhile(NodeStmt* stmt);
            void EmitNodeDoWhile(NodeStmt* stmt);
    
            void EmitNodeIf(NodeStmt* stmt);
    
            void EmitNodeReturn(NodeStmt* stmt);

            void EmitNodeStatement(NodeStmt* stmt);
    
            void EmitNode(Node* node);
    
        private:
            OpCodes m_OpCodes;
            size_t m_Index = 0;
            const ASTNodes* m_Nodes;
    
            size_t m_SlotCount = 0;
            size_t m_LabelCount = 0;
            std::unordered_map<NodeExpr*, CompileStackSlot> m_ConstantMap;
    
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