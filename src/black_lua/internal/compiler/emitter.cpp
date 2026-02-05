#include "internal/compiler/emitter.hpp"

#include <string>

namespace BlackLua::Internal {

    Emitter Emitter::Emit(const ASTNodes* nodes) {
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
        if (m_Index + count < m_Nodes->size()) {
            return m_Nodes->at(m_Index);
        } else {
            return nullptr;
        }
    }

    Node* Emitter::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes->size(), "Consume() of out bounds!");

        return m_Nodes->at(m_Index++);
    }

    void Emitter::EmitConstant(Node* node) {
        switch (node->Type) {
            case NodeType::Constant: {
                NodeConstant* constant = std::get<NodeConstant*>(node->Data);

                m_OpCodes.emplace_back(OpCodeType::PushBytes, static_cast<int32_t>(GetTypeSize(constant->ResolvedType)));
                m_SlotCount++;

                switch (constant->Type) {
                    case NodeType::Bool: {
                        NodeBool* b = std::get<NodeBool*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, 1, &b->Value, true));
                    
                        break;
                    }
                    
                    case NodeType::Char: {
                        NodeChar* c = std::get<NodeChar*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, 1, &c->Char, true));
                    
                        break;
                    }
                    
                    case NodeType::Int: {
                        NodeInt* i = std::get<NodeInt*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, 4, &i->Int, true));
                    
                        break;
                    }
                    
                    case NodeType::Long: {
                        NodeLong* l = std::get<NodeLong*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, 8, &l->Long, true));
                    
                        break;
                    }
                    
                    case NodeType::Float: {
                        NodeFloat* f = std::get<NodeFloat*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, 4, &f->Float, true));
                    
                        break;
                    }
                    
                    case NodeType::Double: {
                        NodeDouble* d = std::get<NodeDouble*>(std::get<NodeConstant*>(node->Data)->Data);
                    
                        m_OpCodes.emplace_back(OpCodeType::Store, OpCodeStore(-1, 8, &d->Double, true));
                    
                        break;
                    }
                }

                m_ConstantMap[node] = CompileStackSlot(m_SlotCount, false);

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

            case NodeType::While: {
                NodeWhile* wh = std::get<NodeWhile*>(node->Data);

                EmitConstant(wh->Condition);
                EmitConstant(wh->Body);

                break;
            } 

            case NodeType::If: {
                NodeIf* nif = std::get<NodeIf*>(node->Data);

                EmitConstant(nif->Condition);
                EmitConstant(nif->Body);

                if (nif->ElseBody) {
                    EmitConstant(nif->ElseBody);
                }

                break;
            }

            case NodeType::Return: {
                NodeReturn* ret = std::get<NodeReturn*>(node->Data);

                EmitConstant(ret->Value);

                break;
            }

            case NodeType::ArrayAccessExpr: {
                NodeArrayAccessExpr* expr = std::get<NodeArrayAccessExpr*>(node->Data);

                EmitConstant(expr->Index);
                break;
            }

            case NodeType::ParenExpr: {
                NodeParenExpr* expr = std::get<NodeParenExpr*>(node->Data);

                EmitConstant(expr->Expression);
                break;
            }

            case NodeType::CastExpr: {
                NodeCastExpr* expr = std::get<NodeCastExpr*>(node->Data);

                EmitConstant(expr->Expression);
                break;
            }

            case NodeType::FunctionCallExpr: {
                NodeFunctionCallExpr* expr = std::get<NodeFunctionCallExpr*>(node->Data);

                for (size_t i = 0; i < expr->Parameters.size(); i++) {
                    EmitConstant(expr->Parameters[i]);
                }
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
        newScope->SlotCount = (newScope->Parent) ? newScope->Parent->SlotCount : 0;
        m_CurrentScope = newScope;

        for (size_t i = 0; i < scope->Nodes.Size; i++) {
            EmitNode(scope->Nodes.Items[i]);
        }

        m_CurrentScope = m_CurrentScope->Parent;
        m_OpCodes.emplace_back(OpCodeType::PopScope);
    }

    void Emitter::EmitNodeVarDecl(Node* node) {
        NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);
        
        PushBytes(GetTypeSize(decl->ResolvedType), std::format("Declaration of {}", decl->Identifier));
        auto& map = (m_CurrentScope != 0) ? m_CurrentScope->DeclaredSymbols : m_DeclaredSymbols;
        
        Declaration d;
        IncrementStackSlotCount();
        if (m_CurrentScope) {
            d.Index = m_CurrentScope->SlotCount;
        } else {
            d.Index = m_SlotCount;   
        }
        d.Size = GetTypeSize(decl->ResolvedType);
        d.Type = decl->ResolvedType;
        
        map[std::string(decl->Identifier)] = d;
        
        // We convert int var = 5 to
        // int var;
        // var = 5;
        if (decl->Value) {
            NodeBinExpr* expr = GetAllocator()->AllocateNamed<NodeBinExpr>();
            NodeVarRef* ref = GetAllocator()->AllocateNamed<NodeVarRef>();
        
            ref->Identifier = decl->Identifier;
        
            expr->Type = BinExprType::Eq;
            expr->ResolvedType = decl->ResolvedType;
            expr->LHS = GetAllocator()->AllocateNamed<Node>(NodeType::VarRef, ref);
            expr->RHS = decl->Value;
        
            EmitNodeExpression(GetAllocator()->AllocateNamed<Node>(NodeType::BinExpr, expr));
        }
    }

    void Emitter::EmitNodeVarArrayDecl(Node* node) {
        // NodeVarArrayDecl* decl = std::get<NodeVarArrayDecl*>(node->Data);
        // 
        // PushBytes(GetTypeSize(decl->Type), std::format("Declaration of {}", decl->Identifier));
        // auto& map = (m_CurrentScope != 0) ? m_CurrentScope->DeclaredSymbols : m_DeclaredSymbols;
        // 
        // Declaration d;
        // IncrementStackSlotCount();
        // if (m_CurrentScope) {
        //     d.Index = m_CurrentScope->SlotCount;
        // } else {
        //     d.Index = m_SlotCount;   
        // }
        // d.Size = GetTypeSize(decl->Type);
        // d.Type = decl->Type;
        // 
        // map[std::string(decl->Identifier)] = d;
        // 
        // // We convert int var = 5 to
        // // int var;
        // // var = 5;
        // if (decl->Value) {
        //     NodeBinExpr* expr = GetAllocator()->AllocateNamed<NodeBinExpr>();
        //     NodeVarRef* ref = GetAllocator()->AllocateNamed<NodeVarRef>();
        // 
        //     ref->Identifier = decl->Identifier;
        // 
        //     expr->Type = BinExprType::Eq;
        //     expr->VarType = decl->Type;
        //     expr->LHS = GetAllocator()->AllocateNamed<Node>(NodeType::VarRef, ref);
        //     expr->RHS = decl->Value;
        // 
        //     EmitNodeExpression(GetAllocator()->AllocateNamed<Node>(NodeType::BinExpr, expr));
        // }
    }

    void Emitter::EmitNodeFunctionDecl(Node* node) {
        NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(node->Data);
        
        if (decl->Extern) {
            std::string ident(decl->Name);
            Declaration d;
            d.Size = GetTypeSize(decl->ResolvedType);
            d.Extern = true;
            d.Type = decl->ResolvedType;
            m_DeclaredSymbols[ident] = d;
        }
    }

    void Emitter::EmitNodeFunctionImpl(Node* node) {
        NodeFunctionImpl* impl = std::get<NodeFunctionImpl*>(node->Data);
        
        std::string ident(impl->Name);
        Declaration decl;
        decl.Index = CreateLabel(std::format("function {}", impl->Name));
        decl.Size = GetTypeSize(impl->ResolvedType);
        decl.Type = impl->ResolvedType;
        m_DeclaredSymbols[ident] = decl;
        
        // We don't want to create a new scope
        // The VM creates one for us at the call site
        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        newScope->SlotCount = (newScope->Parent) ? newScope->Parent->SlotCount : 0;
        m_CurrentScope = newScope;
        
        size_t returnSlot = (impl->ResolvedType->Type == PrimitiveType::Invalid) ? 0 : 1;
        size_t varsPushed = 0;
        
        // Inject function arguments into the scope
        for (size_t i = 0; i < impl->Arguments.Size; i++) {
            EmitNodeVarDecl(impl->Arguments.Items[i]);
        
            // Copy the parameters into the arguments
            m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(-1, -(impl->Arguments.Size + 1 + returnSlot)));
            varsPushed++;
        }
        
        NodeScope* body = std::get<NodeScope*>(impl->Body->Data);
        
        for (size_t i = 0; i < body->Nodes.Size; i++) {
            EmitNode(body->Nodes.Items[i]);
        }
        
        // Just in case
        m_OpCodes.emplace_back(OpCodeType::Ret);
        
        m_CurrentScope = m_CurrentScope->Parent;
    }

    void Emitter::EmitNodeWhile(Node* node) {
        NodeWhile* wh = std::get<NodeWhile*>(node->Data);

        int32_t loop = m_LabelCount;
        m_LabelCount++;
        int32_t loopEnd = m_LabelCount;
        m_LabelCount++;
        m_OpCodes.emplace_back(OpCodeType::Jmp, loop, "while loop condition");
        m_OpCodes.emplace_back(OpCodeType::Label, loop, "while loop condition");

        m_OpCodes.emplace_back(OpCodeType::PushScope);

        Scope* newScope = GetAllocator()->AllocateNamed<Scope>();
        newScope->Parent = m_CurrentScope;
        newScope->SlotCount = (newScope->Parent) ? newScope->Parent->SlotCount : 0;
        m_CurrentScope = newScope;

        CompileStackSlot slot = EmitNodeExpression(wh->Condition);
        m_OpCodes.emplace_back(OpCodeType::Jf, OpCodeJump(CompileToRuntimeStackSlot(slot), loopEnd), "while loop end");

        NodeScope* scope = std::get<NodeScope*>(wh->Body->Data);

        for (size_t i = 0; i < scope->Nodes.Size; i++) {
            EmitNode(scope->Nodes.Items[i]);
        }

        m_OpCodes.emplace_back(OpCodeType::PopScope);

        m_OpCodes.emplace_back(OpCodeType::Jmp, loop, "while loop condition");

        m_OpCodes.emplace_back(OpCodeType::Label, loopEnd, "while loop end");
        m_OpCodes.emplace_back(OpCodeType::PopScope);
        m_CurrentScope = m_CurrentScope->Parent;
    }

    void Emitter::EmitNodeIf(Node* node) {
        NodeIf* nif = std::get<NodeIf*>(node->Data);

        CompileStackSlot slot = EmitNodeExpression(nif->Condition);
        int32_t ifLabel = m_LabelCount;
        int32_t elseLabel = m_LabelCount + 1;
        int32_t afterIfLabel = m_LabelCount + 2;

        m_OpCodes.emplace_back(OpCodeType::Jt, OpCodeJump(CompileToRuntimeStackSlot(slot), ifLabel));
        m_OpCodes.emplace_back(OpCodeType::Jf, OpCodeJump(CompileToRuntimeStackSlot(slot), elseLabel));
        m_OpCodes.emplace_back(OpCodeType::Jmp, afterIfLabel);

        // If label
        CreateLabel();

        EmitNode(nif->Body);
        m_OpCodes.emplace_back(OpCodeType::Jmp, afterIfLabel);

        // Else label
        CreateLabel();

        EmitNode(nif->ElseBody);
        m_OpCodes.emplace_back(OpCodeType::Jmp, afterIfLabel);

        // After if label
        CreateLabel();
    }

    void Emitter::EmitNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        CompileStackSlot slot = EmitNodeExpression(ret->Value);

        m_OpCodes.emplace_back(OpCodeType::RetValue, CompileToRuntimeStackSlot(slot));
    }

    CompileStackSlot Emitter::EmitNodeExpression(Node* node) {
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
                        return CompileStackSlot(currentScope->DeclaredSymbols.at(ident).Index, true);
                    }

                    currentScope = currentScope->Parent;
                }

                // Check global symbols
                if (m_DeclaredSymbols.contains(std::string(ref->Identifier))) {
                    return CompileStackSlot(m_DeclaredSymbols.at(std::string(ref->Identifier)).Index, false);
                }

                BLUA_ASSERT(false, "Unreachable!");
                return {};
                break;
            }

            case NodeType::ArrayAccessExpr: {
                NodeArrayAccessExpr* expr = std::get<NodeArrayAccessExpr*>(node->Data);

                break;
            }

            case NodeType::MemberExpr: {
                NodeMemberExpr* expr = std::get<NodeMemberExpr*>(node->Data);
                CompileStackSlot slot = EmitNodeExpression(expr->Parent);

                VariableType* structType = expr->ResolvedParentType;
                for (const auto& field : std::get<StructDeclaration>(structType->Data).Fields) {
                    if (field.Identifier == expr->Member) {
                        CompileStackSlot s = slot;
                        s.Slot.Offset += field.Offset;
                        s.Slot.Size = GetTypeSize(field.ResolvedType);
                        return s;
                    }
                }

                BLUA_ASSERT(false, "Unreachable!");

                return {};
                break;
            }

            case NodeType::FunctionCallExpr: {
                NodeFunctionCallExpr* expr = std::get<NodeFunctionCallExpr*>(node->Data);
                Declaration decl = m_DeclaredSymbols.at(std::string(expr->Name));

                std::vector<CompileStackSlot> m_Parameters; 

                for (size_t i = 0; i < expr->Parameters.size(); i++) {
                    CompileStackSlot slot = EmitNodeExpression(expr->Parameters[i]);

                    m_Parameters.push_back(slot);
                }

                for (const auto& p : m_Parameters) {
                    // meaty expression
                    m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(p));
                    IncrementStackSlotCount();
                }

                if (decl.Size != 0) {
                    PushBytes(decl.Size, "return slot");
                    IncrementStackSlotCount();
                }

                if (decl.Extern) {
                    m_OpCodes.emplace_back(OpCodeType::CallExtern, std::string(expr->Name));
                } else {
                    m_OpCodes.emplace_back(OpCodeType::Call, decl.Index);
                }
                
                return CompileStackSlot((m_CurrentScope) ? m_CurrentScope->SlotCount : m_SlotCount, (m_CurrentScope) ? true : false);
                break;
            }

            case NodeType::ParenExpr: {
                NodeParenExpr* expr = std::get<NodeParenExpr*>(node->Data);

                return EmitNodeExpression(expr->Expression);
                break;
            }

            case NodeType::CastExpr: {
                // NodeCastExpr* expr = std::get<NodeCastExpr*>(node->Data);
                // 
                // CompileStackSlot slot = EmitNodeExpression(expr->Expression);
                // 
                // if (expr->SourceType == CreateVarType(PrimitiveType::Bool) || expr->SourceType == CreateVarType(PrimitiveType::Char) || expr->SourceType == CreateVarType(PrimitiveType::Short) || expr->SourceType == CreateVarType(PrimitiveType::Int) || expr->SourceType == CreateVarType(PrimitiveType::Long)) {
                //     if (expr->Type == CreateVarType(PrimitiveType::Bool) || expr->Type == CreateVarType(PrimitiveType::Char) || expr->Type == CreateVarType(PrimitiveType::Short) || expr->Type == CreateVarType(PrimitiveType::Int) || expr->Type == CreateVarType(PrimitiveType::Long)) {
                //         m_OpCodes.emplace_back(OpCodeType::CastIntegralToIntegral, OpCodeCast(CompileToRuntimeStackSlot(slot), GetTypeSize(expr->Type)));
                //     } else if (expr->Type == CreateVarType(PrimitiveType::Float) || expr->Type == CreateVarType(PrimitiveType::Double)) {
                //         m_OpCodes.emplace_back(OpCodeType::CastIntegralToFloating, OpCodeCast(CompileToRuntimeStackSlot(slot), GetTypeSize(expr->Type)));
                //     }
                // } else if (expr->SourceType == CreateVarType(PrimitiveType::Float) || expr->SourceType == CreateVarType(PrimitiveType::Double)) {
                //     if (expr->Type == CreateVarType(PrimitiveType::Float) || expr->Type == CreateVarType(PrimitiveType::Double)) {
                //         m_OpCodes.emplace_back(OpCodeType::CastFloatingToFloating, OpCodeCast(CompileToRuntimeStackSlot(slot), GetTypeSize(expr->Type)));
                //     } else if (expr->Type == CreateVarType(PrimitiveType::Bool) || expr->Type == CreateVarType(PrimitiveType::Char) || expr->Type == CreateVarType(PrimitiveType::Short) || expr->Type == CreateVarType(PrimitiveType::Int) || expr->Type == CreateVarType(PrimitiveType::Long)) {
                //         m_OpCodes.emplace_back(OpCodeType::CastFloatingToIntegral, OpCodeCast(CompileToRuntimeStackSlot(slot), GetTypeSize(expr->Type)));
                //     }
                // }
                // 
                // return CompileStackSlot((m_CurrentScope) ? m_CurrentScope->SlotCount : m_SlotCount, (m_CurrentScope) ? true : false);
                break;
            }

            case NodeType::UnaryExpr: {
                NodeUnaryExpr* expr = std::get<NodeUnaryExpr*>(node->Data);
                
                CompileStackSlot slot = EmitNodeExpression(expr->Expression);
                
                switch (expr->Type) {
                    case UnaryExprType::Negate: {
                        if (expr->ResolvedType->Type == PrimitiveType::Bool || expr->ResolvedType->Type == PrimitiveType::Char || expr->ResolvedType->Type == PrimitiveType::Short || expr->ResolvedType->Type == PrimitiveType::Int || expr->ResolvedType->Type == PrimitiveType::Long) {
                            m_OpCodes.emplace_back(OpCodeType::NegateIntegral, CompileToRuntimeStackSlot(slot));
                        } else if (expr->ResolvedType->Type == PrimitiveType::Float || expr->ResolvedType->Type == PrimitiveType::Double) {
                            m_OpCodes.emplace_back(OpCodeType::NegateFloating, CompileToRuntimeStackSlot(slot));
                        } else {
                            BLUA_ASSERT(false, "UnaryExpr has no type!, Did you forget to run this through the type checker?");
                        }
                
                        break;
                    }
                }
                
                IncrementStackSlotCount();
                
                return CompileStackSlot((m_CurrentScope) ? m_CurrentScope->SlotCount : m_SlotCount, (m_CurrentScope) ? true : false);
                break;
            }

            case NodeType::BinExpr: {
                NodeBinExpr* expr = std::get<NodeBinExpr*>(node->Data);
                
                CompileStackSlot rhs = EmitNodeExpression(expr->RHS);
                CompileStackSlot lhs = EmitNodeExpression(expr->LHS);
                
                #define MATH_OP(op, vmOp) case BinExprType::op: { \
                    if (expr->ResolvedType->Type == PrimitiveType::Bool || expr->ResolvedType->Type == PrimitiveType::Char || expr->ResolvedType->Type == PrimitiveType::Short || expr->ResolvedType->Type == PrimitiveType::Int || expr->ResolvedType->Type == PrimitiveType::Long) { \
                        m_OpCodes.emplace_back(OpCodeType::vmOp##Integral, OpCodeMath(CompileToRuntimeStackSlot(lhs), CompileToRuntimeStackSlot(rhs))); \
                    } else if (expr->ResolvedType->Type == PrimitiveType::Float || expr->ResolvedType->Type == PrimitiveType::Double) { \
                        m_OpCodes.emplace_back(OpCodeType::vmOp##Floating, OpCodeMath(CompileToRuntimeStackSlot(lhs), CompileToRuntimeStackSlot(rhs))); \
                    } else { \
                        BLUA_ASSERT(false, "BinExpr has no type!, Did you forget to run this through the type checker?"); \
                    } \
                    IncrementStackSlotCount(); \
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
                        m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(CompileToRuntimeStackSlot(lhs), CompileToRuntimeStackSlot(rhs)));
                        return lhs;
                
                        break;
                    }
                }

                #undef MATH_OP
                
                return CompileStackSlot((m_CurrentScope) ? m_CurrentScope->SlotCount : m_SlotCount, (m_CurrentScope) ? true : false);
                break;
            }
        }

        return {};
    }

    StackSlotIndex Emitter::CompileToRuntimeStackSlot(CompileStackSlot slot) {
        if (slot.Relative) {
            if (m_CurrentScope) {
                return StackSlotIndex(slot.Slot.Slot - static_cast<int32_t>(m_CurrentScope->SlotCount) - 1, slot.Slot.Offset, slot.Slot.Size);
            } else {
                return StackSlotIndex(slot.Slot.Slot - m_SlotCount - 1, slot.Slot.Offset, slot.Slot.Size);
            }
        } else {
            return slot.Slot;
        }
        
        BLUA_ASSERT(false, "Unreachable!");
    }

    int32_t Emitter::CreateLabel(const std::string& debugData) {
        m_OpCodes.emplace_back(OpCodeType::Label, static_cast<int32_t>(m_LabelCount), debugData);
        m_LabelCount++;
        return static_cast<int32_t>(m_LabelCount - 1);
    }

    void Emitter::PushBytes(size_t bytes, const std::string& debugData) {
        m_OpCodes.emplace_back(OpCodeType::PushBytes, static_cast<int32_t>(bytes), debugData);
    }

    void Emitter::IncrementStackSlotCount() {
        (m_CurrentScope) ? m_CurrentScope->SlotCount++ : m_SlotCount++;
    }

    void Emitter::EmitNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::VarDecl) {
            EmitNodeVarDecl(node);
        } else if (t == NodeType::VarArrayDecl) {
            EmitNodeVarArrayDecl(node);
        } else if (t == NodeType::FunctionDecl) {
            EmitNodeFunctionDecl(node);
        } else if (t == NodeType::FunctionImpl) {
            EmitNodeFunctionImpl(node);
        } else if (t == NodeType::StructDecl) {
            // continue
        } else if (t == NodeType::While) {
            EmitNodeWhile(node);
        } else if (t == NodeType::If) {
            EmitNodeIf(node);
        } else if (t == NodeType::ArrayAccessExpr) {
            EmitNodeExpression(node);
        } else if (t == NodeType::MemberExpr) {
            EmitNodeExpression(node);
        } else if (t == NodeType::FunctionCallExpr) {
            EmitNodeExpression(node);
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
