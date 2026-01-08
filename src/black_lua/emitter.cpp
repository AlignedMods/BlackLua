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

    void Emitter::EmitNode(Node* node) {
        if (node->Type == NodeType::Var) {
            EmitNodeVar(node);
        } else if (node->Type == NodeType::Function) {
            EmitNodeFunction(node);
        } else if (node->Type == NodeType::FunctionCall) {
            EmitNodeFunctionCall(node);
        } else if (node->Type == NodeType::If) {
            EmitNodeIf(node);
        } else if (node->Type == NodeType::Return) {
            EmitNodeReturn(node);
        }
    }

    void Emitter::EmitNodeExpression(Node* node) {
        switch (node->Type) {
            case NodeType::Number: {
                NodeNumber* number = std::get<NodeNumber*>(node->Data);

                m_Output += std::to_string(number->Number);
                break;
            }
            case NodeType::Var: {
                NodeVar* var = std::get<NodeVar*>(node->Data);

                m_Output += var->Identifier.Data;
                break;
            }
            case NodeType::VarRef: {
                NodeVarRef* varRef = std::get<NodeVarRef*>(node->Data);

                m_Output += varRef->Identifier.Data;
                break;
            }
            case NodeType::Binary: {
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
            case NodeType::FunctionCall: {
                EmitNodeFunctionCall(node);
                break;
            }
            default: BLUA_ASSERT(false, "Unreachable!"); break;
        }
    }

    void Emitter::EmitNodeVar(Node* node) {
        NodeVar* var = std::get<NodeVar*>(node->Data);
        
        if (!var->Global) {
            m_Output += "local ";
        }

        m_Output += var->Identifier.Data;
        
        if (var->Value) {
            m_Output += " = ";
            EmitNodeExpression(var->Value);
        }

        m_Output += '\n';
    }

    void Emitter::EmitNodeFunction(Node* node) {
        NodeFunction* func = std::get<NodeFunction*>(node->Data);

        m_Output += "function ";
        m_Output += func->Signature;
        m_Output += '(';

        for (const auto& arg : func->Arguments) {
            EmitNodeExpression(arg);
        }

        m_Output += ")\n";

        for (const auto& statement : func->Body) {
            m_Output += "    ";
            EmitNode(statement);
        }

        m_Output += "end\n";
    }

    void Emitter::EmitNodeFunctionCall(Node* node) {
        NodeFunctionCall* funcCall = std::get<NodeFunctionCall*>(node->Data);

        m_Output += funcCall->Signature;
        m_Output += '(';

        for (const auto& param : funcCall->Paramaters) {
            EmitNodeExpression(param);
        }

        m_Output += ')';
        m_Output += '\n';
    }

    void Emitter::EmitNodeIf(Node* node) {
        NodeIf* nif = std::get<NodeIf*>(node->Data);

        m_Output += "if ";
        EmitNodeExpression(nif->Expression);
        m_Output += " then\n";

        for (const auto& statement : nif->Body) {
            m_Output += "    ";
            EmitNode(statement);
        }

        m_Output += "end\n";
    }

    void Emitter::EmitNodeReturn(Node* node) {
        NodeReturn* ret = std::get<NodeReturn*>(node->Data);

        m_Output += "return ";
        EmitNodeExpression(ret->Value);
        m_Output += '\n';
    }

} // namespace BlackLua
