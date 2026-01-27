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

            case NodeType::BinExpr: {
                NodeBinExpr* expr = std::get<NodeBinExpr*>(node->Data);

                EmitConstant(expr->LHS);
                EmitConstant(expr->RHS);
                break;
            }

            case NodeType::VarDecl: {
                NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

                if (decl->Value) {
                    EmitConstant(decl->Value);
                }
            }
        }
    }

    void Emitter::EmitNodeVarDecl(Node* node) {
        NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

        m_OpCodes.emplace_back(OpCodeType::PushBytes, static_cast<int32_t>(GetTypeSize(decl->Type)));
        m_SlotCount++;

        m_VariableMap[std::string(decl->Identifier)] = m_SlotCount - 1;

        if (decl->Value) {
            EmitNodeExpression(decl->Value, -1);
        }
    }

    void Emitter::EmitNodeExpression(Node* node, int32_t slot) {
        switch (node->Type) {
            case NodeType::Constant: {
                m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(slot, m_ConstantMap.at(node)));
                break;
            }

            case NodeType::VarRef: {
                NodeVarRef* ref = std::get<NodeVarRef*>(node->Data);

                m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(slot, m_VariableMap.at(std::string(ref->Identifier))));
            }
        }
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

    void Emitter::EmitNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::VarDecl) {
            EmitNodeVarDecl(node);
        } else {
            BLUA_ASSERT(false, "Unrechable!");
        }
    }

} // namespace BlackLua::Internal
