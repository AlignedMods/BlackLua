#include "compiler.hpp"

#include <string>

namespace BlackLua::Internal {

    // There are certain default lua functions that we don't want to mangle
    // For example we really do not want "print("hello world")" to become "f_1print("hello world")"
    static std::unordered_map<std::string_view, bool> s_NoMangle = {
        { "print", true }
    };

    TypeChecker TypeChecker::Check(const Parser::Nodes& nodes) {
        TypeChecker checker;
        checker.m_Nodes = nodes;
        checker.CheckImpl();

        return checker;
    }

    const Parser::Nodes& TypeChecker::GetCheckedNodes() const {
        return m_Nodes;
    }

    void TypeChecker::CheckImpl() {
        while (Peek()) {
            CheckNode(Consume());
        }
    }

    Node* TypeChecker::Peek(size_t count) {
        if (m_Index + count < m_Nodes.size()) {
            return m_Nodes.at(m_Index);
        } else {
            return nullptr;
        }
    }

    Node* TypeChecker::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes.size(), "Consume() of out bounds!");

        return m_Nodes.at(m_Index++);
    }

    void TypeChecker::CheckNode(Node* node) {
        if (node->Type == NodeType::Var) {
            CheckNodeVar(node);
        } else if (node->Type == NodeType::Function) {
            CheckNodeFunction(node);
        } else if (node->Type == NodeType::FunctionCall) {
            CheckNodeFunctionCall(node);
        } else if (node->Type == NodeType::If) {
            CheckNodeIf(node);
        } else if (node->Type == NodeType::While) {
            CheckNodeWhile(node);
        } else if (node->Type == NodeType::Return) {
            CheckNodeReturn(node);
        }
    }

    void TypeChecker::CheckNodeExpression(Node* node) {
        switch (node->Type) {
            case NodeType::FunctionCall:
                CheckNodeFunctionCall(node);
                break;
        }
    }

    void TypeChecker::CheckNodeVar(Node* node) {
        NodeVar* var = std::get<NodeVar*>(node->Data);

        if (var->Value) {
            CheckNodeExpression(var->Value);
        }
    }

    void TypeChecker::CheckNodeFunction(Node* node) {
        NodeFunction* func = std::get<NodeFunction*>(node->Data);

        const std::string& name = func->Name.Data;
        std::string sig = "f_";
        
        sig += std::to_string(func->Arguments.size());
        sig += name;

        func->Signature = sig;
    }

    void TypeChecker::CheckNodeFunctionCall(Node* node) {
        NodeFunctionCall* funcCall = std::get<NodeFunctionCall*>(node->Data);

        const std::string& name = funcCall->Name.Data;

        if (!s_NoMangle.contains(name)) {
            std::string sig = "f_";
        
            sig += std::to_string(funcCall->Paramaters.size());
            sig += name;

            funcCall->Signature = sig;
        } else {
            funcCall->Signature = name;
        }
    }

    void TypeChecker::CheckNodeIf(Node* node) {
        NodeIf* nif = std::get<NodeIf*>(node->Data);

        for (const auto& statement : nif->Body) {
            CheckNode(statement);
        }

        if (nif->Else) {
            for (const auto& statement : nif->Else->Body) {
                CheckNode(statement);
            }
        }
    }

    void TypeChecker::CheckNodeWhile(Node* node) {
        NodeWhile* nwhile = std::get<NodeWhile*>(node->Data);

        for (const auto& statement : nwhile->Body) {
            CheckNode(statement);
        }
    }

    void TypeChecker::CheckNodeReturn(Node* node) {
        // Nothing to do
    }

} // namespace BlackLua::Internal