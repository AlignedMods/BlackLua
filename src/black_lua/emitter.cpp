#include "compiler.hpp"

#include <string>

namespace BlackLua::Internal {

    Emitter Emitter::Emit(const Parser::Nodes& nodes) {
        Emitter e;
        e.m_Nodes = nodes;
        e.EmitImpl();
        return e;
    }

    const Emitter::OpCodes& Emitter::GetOpCodes() const {
        return m_OpCodes;
    }

    void Emitter::EmitImpl() {
        // Emit the constants first
        while (Peek()) {
            EmitConstant(Consume());
        }

        // Reset the index
        m_Index = 0;

        while (Peek()) {
            EmitNode(Consume());
        }
    }

    Node* Emitter::Peek(size_t count) {
        if (m_Index + count < m_Nodes.size()) {
            return m_Nodes.at(m_Index);
        } else {
            return nullptr;
        }
    }

    Node* Emitter::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes.size(), "Consume() of out bounds!");

        return m_Nodes.at(m_Index++);
    }

    void Emitter::EmitConstant(Node* node) {
        switch (node->Type) {
            case NodeType::Constant: {
                NodeConstant* constant = std::get<NodeConstant*>(node->Data);

                m_SlotCount++;
                m_OpCodes.emplace_back(OpCodeType::PushBytes, static_cast<int32_t>(GetTypeSize(constant->VarType)));

                switch (constant->Type) {
                    case NodeType::Bool: {
                        NodeBool* b = std::get<NodeBool*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, &b->Value));
                    
                        break;
                    }
                    
                    case NodeType::Char: {
                        NodeChar* c = std::get<NodeChar*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, &c->Char));
                    
                        break;
                    }
                    
                    case NodeType::Int: {
                        NodeInt* i = std::get<NodeInt*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, &i->Int));
                    
                        break;
                    }
                    
                    case NodeType::Long: {
                        NodeLong* l = std::get<NodeLong*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, &l->Long));
                    
                        break;
                    }
                    
                    case NodeType::Float: {
                        NodeFloat* f = std::get<NodeFloat*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, &f->Float));
                    
                        break;
                    }
                    
                    case NodeType::Double: {
                        NodeDouble* d = std::get<NodeDouble*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, &d->Double));
                    
                        break;
                    }
                }

                m_ConstantMap[node] = m_SlotCount;

                break;
            }

            case NodeType::Scope: {
                NodeScope* scope = std::get<NodeScope*>(node->Data);

                for (size_t i = 0; i < scope->Nodes.Size; i++) {
                    EmitConstant(scope->Nodes.Items[i]);
                }

                break;
            }

            case NodeType::VarDecl: {
                NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

                if (decl->Value) {
                    EmitConstant(decl->Value);
                }

                break;
            }

            case NodeType::FunctionImpl: {
                NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(node->Data);

                for (size_t i = 0; i < impl->Arguments.Size; i++) {
                    EmitConstant(impl->Arguments.Items[i]);
                }
                EmitConstant(impl->Body);

                break;
            }

            case NodeType::Return: {
                NodeReturn* ret = std::get<NodeReturn*>(node->Data);

                EmitConstant(ret->Value);

                break;
            }

            case NodeType::ParenExpr: {
                NodeParenExpr* expr = std::get<NodeParenExpr*>(node->Data);

                EmitConstant(expr->Expression);
                break;
            }

            case NodeType::UnaryExpr: {
                NodeUnaryExpr* expr = std::get<NodeUnaryExpr*>(node->Data);

                EmitConstant(expr->Expression);
                break;
            }

            case NodeType::BinExpr: {
                NodeBinExpr* expr = std::get<NodeBinExpr*>(node->Data);

                EmitConstant(expr->LHS);
                EmitConstant(expr->RHS);
                break;
            }
        }
    }

    void Emitter::EmitNodeScope(Node* node) {
        NodeScope* scope = std::get<NodeScope*>(node->Data);

        m_OpCodes.emplace_back(OpCodeType::PushScope);

        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        newScope->Start = m_SlotCount;
        m_CurrentScope = newScope;

        for (size_t i = 0; i < scope->Nodes.Size; i++) {
            EmitNode(scope->Nodes.Items[i]);
        }

        m_SlotCount = m_CurrentScope->Start;
        m_CurrentScope = m_CurrentScope->Parent;
        m_OpCodes.emplace_back(OpCodeType::PopScope);
    }

    void Emitter::EmitNodeVarDecl(Node* node) {
        NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

        m_OpCodes.emplace_back(OpCodeType::PushBytes, static_cast<int32_t>(GetTypeSize(decl->Type)));
        m_SlotCount++;
        auto& map = (m_CurrentScope != 0) ? m_CurrentScope->DeclaredSymbols : m_DeclaredSymbols;
        map[std::string(decl->Identifier)] = m_SlotCount;

        if (decl->Value) {
           int32_t slot = EmitNodeExpression(decl->Value);

           m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(m_SlotCount, slot));
        }
    }

    void Emitter::EmitNodeFunctionImpl(Node* node) {
        NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(node->Data);

        std::string ident(impl->Name);
        m_DeclaredSymbols[ident] = CreateLabel();

        // We don't want to create a new scope
        // The VM creates one for us at the call site
        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        newScope->Start = m_SlotCount;
        m_CurrentScope = newScope;

        // Inject function arguments into the scope
        for (size_t i = 0; i < impl->Arguments.Size; i++) {
            EmitNodeVarDecl(impl->Arguments.Items[i]);
        }

        NodeScope* body = std::get<NodeScope*>(impl->Body->Data);

        for (size_t i = 0; i < body->Nodes.Size; i++) {
            EmitNode(body->Nodes.Items[i]);
        }

        m_CurrentScope = m_CurrentScope->Parent;
    }

    void Emitter::EmitNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        int32_t slot = EmitNodeExpression(ret->Value);

        // Because the current scope the return statement is in, we need to return the value to a specific spot
        int32_t returnSlot = -1 - (m_SlotCount - m_CurrentScope->Start);
        m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(returnSlot, slot));
        m_OpCodes.emplace_back(OpCodeType::Ret);
    }

    int32_t Emitter::EmitNodeExpression(Node* node) {
        switch (node->Type) {
            case NodeType::Constant: {
                return m_ConstantMap.at(node);
                break;
            }

            case NodeType::VarRef: {
                NodeVarRef* ref = std::get<NodeVarRef*>(node->Data);
                std::string ident(ref->Identifier);

                // Loop backward through all the scopes
                Scope* currentScope = m_CurrentScope;
                while (currentScope) {
                    if (currentScope->DeclaredSymbols.contains(ident)) {
                        return currentScope->DeclaredSymbols.at(ident);
                    }

                    currentScope = currentScope->Parent;
                }

                // Check global symbols
                if (m_DeclaredSymbols.contains(std::string(ref->Identifier))) {
                    return m_DeclaredSymbols.at(std::string(ref->Identifier));
                }

                BLUA_ASSERT(false, "Unreachable!");
                return 0;
                break;
            }

            case NodeType::ParenExpr: {
                NodeParenExpr* expr = std::get<NodeParenExpr*>(node->Data);

                return EmitNodeExpression(expr->Expression);
                break;
            }

            case NodeType::UnaryExpr: {
                NodeUnaryExpr* expr = std::get<NodeUnaryExpr*>(node->Data);

                int32_t slot = EmitNodeExpression(expr->Expression);

                switch (expr->Type) {
                    case UnaryExprType::Negate: {
                        if (expr->VarType == VariableType::Bool || expr->VarType == VariableType::Char || expr->VarType == VariableType::Short || expr->VarType == VariableType::Int || expr->VarType == VariableType::Long) {
                            m_OpCodes.emplace_back(OpCodeType::NegateIntegral, slot);
                        } else if (expr->VarType == VariableType::Float || expr->VarType == VariableType::Double) {
                            m_OpCodes.emplace_back(OpCodeType::NegateFloating, slot);
                        }

                        break;
                    }
                }

                m_SlotCount++;

                return m_SlotCount;
                break;
            }

            case NodeType::BinExpr: {
                NodeBinExpr* expr = std::get<NodeBinExpr*>(node->Data);

                int32_t lhs = EmitNodeExpression(expr->LHS);
                int32_t rhs = EmitNodeExpression(expr->RHS);

                #define MATH_OP(op, vmOp) case BinExprType::op: { \
                    if (expr->VarType == VariableType::Bool || expr->VarType == VariableType::Char || expr->VarType == VariableType::Short || expr->VarType == VariableType::Int || expr->VarType == VariableType::Long) { \
                        m_OpCodes.emplace_back(OpCodeType::vmOp##Integral, OpCodeMath(lhs, rhs)); \
                    } else if (expr->VarType == VariableType::Float || expr->VarType == VariableType::Double) { \
                        m_OpCodes.emplace_back(OpCodeType::vmOp##Floating, OpCodeMath(lhs, rhs)); \
                    } \
                    break; \
                }

                switch (expr->Type) {
                    MATH_OP(Add, Add);
                    MATH_OP(Sub, Sub);
                    MATH_OP(Mul, Mul);
                    MATH_OP(Div, Div);
                    MATH_OP(Less, Lt);
                    MATH_OP(LessOrEq, Lte);
                    MATH_OP(Greater, Gt);
                    MATH_OP(GreaterOrEq, Gte);
                    MATH_OP(IsEq, Cmp);
                    
                    case BinExprType::Eq: {
                        m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(lhs, rhs));

                        break;
                    }
                }

                m_SlotCount++;

                return m_SlotCount;
                break;
            }
        }

        return 0;
    }

    size_t Emitter::GetTypeSize(VariableType type) {
        switch (type) {
            case VariableType::Void: return 0;
            case VariableType::Bool: return 1;
            case VariableType::Char: return 1;
            case VariableType::Short: return 2;
            case VariableType::Int: return 4;
            case VariableType::Float: return 4;
            case VariableType::Long: return 8;
            case VariableType::Double: return 8;
            default: BLUA_ASSERT(false, "Unrechable!");
        }

        return 0;
    }

    int32_t Emitter::CreateLabel() {
        m_OpCodes.emplace_back(OpCodeType::Label, static_cast<int32_t>(m_LabelCount));
        m_LabelCount++;
        return static_cast<int32_t>(m_LabelCount - 1);
    }

    void Emitter::EmitNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::VarDecl) {
            EmitNodeVarDecl(node);
        } else if (t == NodeType::FunctionImpl) {
            EmitNodeFunctionImpl(node);
        } else if (t == NodeType::BinExpr) {
            EmitNodeExpression(node);
        } else if (t == NodeType::Scope) {
            EmitNodeScope(node);
        } else if (t == NodeType::Return) {
            EmitNodeReturn(node);
        } else {
            BLUA_ASSERT(false, "Unreachable!");
        }
    }

} // namespace BlackLua::Internal
