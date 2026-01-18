#include "compiler.hpp"

#include <string>

namespace BlackLua::Internal {

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

        m_Output += "local ";
        m_Output += decl->Identifier;
        if (decl->Value) {
            m_Output += " = ";
            EmitNodeExpression(decl->Value);
        }
        m_Output += '\n';
    }

    // void Emitter::EmitNodeVarSet(Node* node) {
    //     NodeVarSet* decl = std::get<NodeVarSet*>(node->Data);
    // 
    //     m_Output += decl->Identifier;
    //     if (decl->Value) {
    //         m_Output += " = ";
    //         EmitNodeExpression(decl->Value);
    //     }
    //     m_Output += '\n';
    // }

    void Emitter::EmitNodeFunctionDecl(Node* node) {
        NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(node->Data);

        // if (decl->HasBody) {
        //     m_Output += "function ";
        //     m_Output += decl->Signature;
        //     m_Output += "(";
        // 
        //     for (size_t i = 0; i < decl->Arguments.size(); i++) {
        //         NodeVarDecl* var = std::get<NodeVarDecl*>(decl->Arguments[i]->Data);
        // 
        //         m_Output += var->Identifier;
        // 
        //         if (var->Value) {
        //             m_Output += " = ";
        //             EmitNodeExpression(var->Value);
        //         }
        // 
        //         if (i + 1 < decl->Arguments.size()) {
        //             m_Output += ", ";
        //         }
        //     }
        // 
        //     m_Output += ")\n";
        // 
        //     for (const auto& n : decl->Body) {
        //         m_Output += "    ";
        //         EmitNode(n);
        //     }
        // 
        //     m_Output += "end\n";
        // }
    }

    void Emitter::EmitNodeFunctionCall(Node* node) {
        // NodeFunctionCall* call = std::get<NodeFunctionCall*>(node->Data);
        // 
        // m_Output += call->Signature;
        // m_Output += "(";
        // 
        // for (size_t i = 0; i < call->Parameters.size(); i++) {
        //     EmitNodeExpression(call->Parameters[i]);
        // 
        //     if (i + 1 < call->Parameters.size()) {
        //         m_Output += ", ";
        //     }
        // }
        // 
        // m_Output += ")\n";
    }

    void Emitter::EmitNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        m_Output += "return ";
        EmitNodeExpression(ret->Value);
    }

    void Emitter::EmitNodeExpression(Node* node) {
        switch (node->Type) {
            case NodeType::Nil: {
                m_Output += "nil";
                break;
            }
            case NodeType::Bool: {
                NodeBool* nbool = std::get<NodeBool*>(node->Data);

                if (nbool->Value == false) {
                    m_Output += "false";
                } else {
                    m_Output += "true";
                }

                break;
            }
             case NodeType::Int: {
                NodeInt* i = std::get<NodeInt*>(node->Data);

                m_Output += std::to_string(i->Int);
                break;
            }
            case NodeType::Number: {
                NodeNumber* number = std::get<NodeNumber*>(node->Data);

                m_Output += std::to_string(number->Number);
                break;
            }
            case NodeType::String: {
                NodeString* string = std::get<NodeString*>(node->Data);

                m_Output += '"';
                m_Output += string->String;
                m_Output += '"';
                break;
            }
            case NodeType::InitializerList: {
                NodeInitializerList* initList = std::get<NodeInitializerList*>(node->Data);

                m_Output += "{ ";

                for (size_t i = 0; i < initList->Nodes.size(); i++) {
                    EmitNode(initList->Nodes.at(i));

                    if (i != initList->Nodes.size() - 1) {
                        m_Output += ", ";
                    }
                }

                m_Output += " }";

                break;
            }
            case NodeType::BinExpr: {
                NodeBinExpr* binExpr = std::get<NodeBinExpr*>(node->Data);

                EmitNodeExpression(binExpr->LHS);

                if (binExpr->Type == BinExprType::Add) {
                    m_Output += " + ";
                } else if (binExpr->Type == BinExprType::Sub) {
                    m_Output += " - ";
                } else if (binExpr->Type == BinExprType::Mul) {
                    m_Output += " * ";
                } else if (binExpr->Type == BinExprType::Div) {
                    m_Output += " / ";
                } else if (binExpr->Type == BinExprType::Less) {
                    m_Output += " < ";
                } else if (binExpr->Type == BinExprType::LessOrEq) {
                    m_Output += " <= ";
                } else if (binExpr->Type == BinExprType::Greater) {
                    m_Output += " > ";
                } else if (binExpr->Type == BinExprType::GreaterOrEq) {
                    m_Output += " >= ";
                }

                EmitNodeExpression(binExpr->RHS);
                break;
            }
            default: BLUA_ASSERT(false, "Unreachable!"); break;
        }
    }

    void Emitter::EmitNode(Node* node) {
        NodeType t = node->Type;

        if (t == NodeType::VarDecl) {
            EmitNodeVarDecl(node);
        // } else if (t == NodeType::VarSet) {
            EmitNodeVarSet(node);
        } else if (t == NodeType::FunctionDecl) {
            EmitNodeFunctionDecl(node);
        // } else if (t == NodeType::FunctionCall) {
            EmitNodeFunctionCall(node);
        }
    }

} // namespace BlackLua::Internal
