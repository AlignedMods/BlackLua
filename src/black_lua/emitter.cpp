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

    void Emitter::EmitNodeVarDecl(Node* node) {
        NodeVarDecl* decl = std::get<NodeVarDecl*>(node->Data);

        OpCode op;
        op.Type = OpCodeType::PushBytes;
        op.Data = static_cast<int32_t>(GetTypeSize(decl->Type));
        m_OpCodes.push_back(op);
        if (decl->Value) {
            EmitNodeExpression(decl->Value);
        }
    }

    void Emitter::EmitNodeExpression(Node* node, bool allocate) {
        switch (node->Type) {
            // case NodeType::Bool: {
            //     if (allocate) {
            //         m_OpCodes.emplace_back(OpCodeType::PushBytes, static_cast<size_t>(1));
            //     }
            //     NodeBool* nbool = std::get<NodeBool*>(node->Data);
            // 
            //     m_OpCodes.emplace_back(OpCodeType::StoreLocal)
            // 
            //     break;
            // }
            //  case NodeType::Int: {
            //     NodeInt* i = std::get<NodeInt*>(node->Data);
            // 
            //     m_Output += std::to_string(i->Int);
            //     break;
            // }
            // case NodeType::Number: {
            //     NodeNumber* number = std::get<NodeNumber*>(node->Data);
            // 
            //     m_Output += std::to_string(number->Number);
            //     break;
            // }
            // case NodeType::String: {
            //     NodeString* string = std::get<NodeString*>(node->Data);
            // 
            //     m_Output += '"';
            //     m_Output += string->String;
            //     m_Output += '"';
            //     break;
            // }
            // case NodeType::InitializerList: {
            //     NodeInitializerList* initList = std::get<NodeInitializerList*>(node->Data);
            // 
            //     m_Output += "{ ";
            // 
            //     // for (size_t i = 0; i < initList->Nodes.size(); i++) {
            //     //     EmitNode(initList->Nodes.at(i));
            //     // 
            //     //     if (i != initList->Nodes.size() - 1) {
            //     //         m_Output += ", ";
            //     //     }
            //     // }
            // 
            //     m_Output += " }";
            // 
            //     break;
            // }
            // case NodeType::BinExpr: {
            //     NodeBinExpr* binExpr = std::get<NodeBinExpr*>(node->Data);
            // 
            //     EmitNodeExpression(binExpr->LHS);
            // 
            //     if (binExpr->Type == BinExprType::Add) {
            //         m_Output += " + ";
            //     } else if (binExpr->Type == BinExprType::Sub) {
            //         m_Output += " - ";
            //     } else if (binExpr->Type == BinExprType::Mul) {
            //         m_Output += " * ";
            //     } else if (binExpr->Type == BinExprType::Div) {
            //         m_Output += " / ";
            //     } else if (binExpr->Type == BinExprType::Less) {
            //         m_Output += " < ";
            //     } else if (binExpr->Type == BinExprType::LessOrEq) {
            //         m_Output += " <= ";
            //     } else if (binExpr->Type == BinExprType::Greater) {
            //         m_Output += " > ";
            //     } else if (binExpr->Type == BinExprType::GreaterOrEq) {
            //         m_Output += " >= ";
            //     }
            // 
            //     EmitNodeExpression(binExpr->RHS);
            //     break;
            // }
            default: BLUA_ASSERT(false, "Unreachable!"); break;
        }
    }

    size_t Emitter::GetTypeSize(VariableType type) {
        switch (type) {
            case VariableType::Void: return 0;
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
