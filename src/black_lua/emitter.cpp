#include "compiler.hpp"

namespace BlackLua {

    Emitter Emitter::Emit(const Parser::Nodes& nodes) {
        Emitter e;
        e.m_Nodes = nodes;
        e.EmitImpl();
        return e;
    }

    const std::string& Emitter::GetOutput() const {
        return m_Output;
    }

    void Emitter::EmitImpl() {
        while (Peek()) {
            if (Peek()->Type == NodeType::Var) {
                EmitNodeVar();
            }
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

    void Emitter::EmitNodeValue(Node* node) {
        switch (node->Type) {
            case NodeType::Number: {
                NodeNumber* number = std::get<NodeNumber*>(node->Data);

                m_Output += std::to_string(number->Number);
                break;
            }
            case NodeType::VarRef: {
                NodeVarRef* varRef = std::get<NodeVarRef*>(node->Data);

                m_Output += varRef->Identifier.Data;
                break;
            }
            case NodeType::Binary: {
                NodeBinExpr* binExpr = std::get<NodeBinExpr*>(node->Data);

                EmitNodeValue(binExpr->LHS);

                if (binExpr->Type == BinExprType::Add) {
                    m_Output += " + ";
                } else if (binExpr->Type == BinExprType::Sub) {
                    m_Output += " - ";
                } else if (binExpr->Type == BinExprType::Mul) {
                    m_Output += " * ";
                } else if (binExpr->Type == BinExprType::Div) {
                    m_Output += " / ";
                }

                EmitNodeValue(binExpr->RHS);
                break;
            }
            default: BLUA_ASSERT(false, "Unreachable!"); break;
        }
    }

    void Emitter::EmitNodeVar() {
        Node* node = Consume();
        NodeVar* var = std::get<NodeVar*>(node->Data);
        
        if (!var->Global) {
            m_Output += "local ";
        }

        m_Output += var->Identifier.Data;
        
        if (var->Value) {
            m_Output += " = ";
            EmitNodeValue(var->Value);
        }

        m_Output += '\n';
    }

} // namespace BlackLua
