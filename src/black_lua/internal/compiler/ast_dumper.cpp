#include "internal/compiler/ast_dumper.hpp"

namespace BlackLua::Internal {

    ASTDumper ASTDumper::DumpAST(ASTNodes* nodes) {
        ASTDumper d;
        d.m_ASTNodes = nodes;
        d.DumpASTImpl();
        return d;
    }

    std::string& ASTDumper::GetOutput() {
        return m_Output;
    }

    void ASTDumper::DumpASTImpl() {
        for (Node* node : *m_ASTNodes) {
            DumpASTNode(node);
        }
    }

    void ASTDumper::DumpASTNode(Node* n, size_t indentation) {
        std::string ident;
        ident.append(indentation, ' ');
        m_Output += ident;

        if (n == nullptr) {
            m_Output += "NULL\n";
        } else {
            m_Output += fmt::format("{}:{} ", n->Line, n->Column);

            switch (n->Type) {
                case NodeType::Constant: {
                    NodeConstant* constant = std::get<NodeConstant*>(n->Data);

                    m_Output += "Constant, Value: \n";
                    ident.append(4, ' ');
                    m_Output += ident;

                    switch (constant->Type) {
                        case NodeType::Bool: {
                            NodeBool* b = std::get<NodeBool*>(constant->Data);

                            if (b->Value) {
                                m_Output += "Bool, Value: true\n";
                            } else {
                                m_Output += "Bool, Value: false\n";
                            }

                            break;
                        }

                        case NodeType::Int: {
                            NodeInt* i = std::get<NodeInt*>(constant->Data);

                            if (i->Unsigned) {  
                                m_Output += fmt::format("Int, Value: {}, Signed: false\n", static_cast<uint32_t>(i->Int));
                            } else {
                                m_Output += fmt::format("Int, Value: {}, Signed: true\n", static_cast<int32_t>(i->Int));
                            }

                            break;
                        }

                        case NodeType::Long: {
                            NodeLong* l = std::get<NodeLong*>(constant->Data);

                            if (l->Unsigned) {  
                                m_Output += fmt::format("Long, Value: {}, Signed: false\n", static_cast<uint64_t>(l->Long));
                            } else {
                                m_Output += fmt::format("Long, Value: {}, Signed: true\n", static_cast<int64_t>(l->Long));
                            }

                            break;
                        }

                        case NodeType::Float: {
                            NodeFloat* f = std::get<NodeFloat*>(constant->Data);

                            m_Output += fmt::format("Float, Value: {:.5f}", f->Float);
                            break;
                        }

                        case NodeType::Double: {
                            NodeDouble* d = std::get<NodeDouble*>(constant->Data);

                            m_Output += fmt::format("Double, Value: {:.15f}", d->Double);
                            break;
                        }

                        case NodeType::String: {
                            NodeString* str = std::get<NodeString*>(constant->Data);

                            m_Output += fmt::format("String, Value: \"{}\"\n", str->String);
                            break;
                        }

                        case NodeType::InitializerList: {
                            NodeInitializerList* list = std::get<NodeInitializerList*>(constant->Data);

                            m_Output += "InitializerList, Values: \n";
                            for (size_t i = 0; i < list->Nodes.Size; i++) {
                                DumpASTNode(list->Nodes.Items[i], indentation + 4);
                            }

                            break;
                        }
                    }

                    break;
                }

                case NodeType::Scope: {
                    NodeScope* scope = std::get<NodeScope*>(n->Data);

                    m_Output += "Scope, Body: \n";
                    for (size_t i = 0; i < scope->Nodes.Size; i++) {
                        DumpASTNode(scope->Nodes.Items[i], indentation + 4);
                    }

                    break;
                }
                
                case NodeType::VarDecl: {
                    NodeVarDecl* decl = std::get<NodeVarDecl*>(n->Data);

                    m_Output += fmt::format("VarDecl, Type: {}, Name: {}, Value:\n", VariableTypeToString(decl->ResolvedType), decl->Identifier);
                    DumpASTNode(decl->Value, indentation + 4);

                    break;
                }

                case NodeType::ParamDecl: {
                    NodeParamDecl* decl = std::get<NodeParamDecl*>(n->Data);

                    m_Output += fmt::format("ParamDecl, Type: {}, Name: {}\n", VariableTypeToString(decl->ResolvedType), decl->Identifier);
                    break;
                }

                case NodeType::VarRef: {
                    NodeVarRef* ref = std::get<NodeVarRef*>(n->Data);

                    m_Output += fmt::format("VarRef, Name: {}\n", ref->Identifier);
                    break;
                }

                case NodeType::StructDecl: {
                    NodeStructDecl* decl = std::get<NodeStructDecl*>(n->Data);

                    m_Output += fmt::format("StructDecl, Name: {}, Fields:\n", decl->Identifier);

                    for (size_t i = 0; i < decl->Fields.Size; i++) {
                        DumpASTNode(decl->Fields.Items[i], indentation + 4);
                    }

                    break;
                }

                case NodeType::FieldDecl: {
                    NodeFieldDecl* decl = std::get<NodeFieldDecl*>(n->Data);

                    m_Output += fmt::format("FieldDecl, Type: {}, Name: {}\n", "TODO", decl->Identifier);
                    break;
                }

                case NodeType::MethodDecl: {
                    NodeMethodDecl* decl = std::get<NodeMethodDecl*>(n->Data);

                    m_Output += fmt::format("MethodDecl, Name: {}, Return type: {}\n", decl->Name, VariableTypeToString(decl->ResolvedType));
                    for (size_t i = 0; i < decl->Parameters.Size; i++) {
                        DumpASTNode(decl->Parameters.Items[i], indentation + 4);
                    }
                    DumpASTNode(decl->Body, indentation + 4);

                    break;
                }

                case NodeType::FunctionDecl: {
                    NodeFunctionDecl* decl = std::get<NodeFunctionDecl*>(n->Data);

                    m_Output += fmt::format("FunctionDecl, Name: {}, Return type: {}, Extern: {}\n", decl->Name, VariableTypeToString(decl->ResolvedType), decl->Extern);
                    for (size_t i = 0; i < decl->Parameters.Size; i++) {
                        DumpASTNode(decl->Parameters.Items[i], indentation + 4);
                    }

                    if (decl->Body) {
                        DumpASTNode(decl->Body, indentation + 4);
                    }

                    break;
                }

                case NodeType::While: {
                    NodeWhile* wh = std::get<NodeWhile*>(n->Data);

                    m_Output += "WhileStatement:\n";
                    DumpASTNode(wh->Condition, indentation + 4);
                    DumpASTNode(wh->Body, indentation + 4);

                    break;
                }

                case NodeType::DoWhile: {
                    NodeDoWhile* dowh = std::get<NodeDoWhile*>(n->Data);

                    std::cout << "DoWhileStatement:\n";
                    DumpASTNode(dowh->Body, indentation + 4);
                    DumpASTNode(dowh->Condition, indentation + 4);

                    break;
                }

                case NodeType::For: {
                    NodeFor* nfor = std::get<NodeFor*>(n->Data);

                    m_Output += "ForStatement:\n";
                    DumpASTNode(nfor->Prologue, indentation + 4);
                    DumpASTNode(nfor->Condition, indentation + 4);
                    DumpASTNode(nfor->Epilogue, indentation + 4);
                    DumpASTNode(nfor->Body, indentation + 4);

                    break;
                }

                case NodeType::If: {
                    NodeIf* nif = std::get<NodeIf*>(n->Data);

                    m_Output += "IfStatement:\n";
                    DumpASTNode(nif->Condition, indentation + 4);
                    DumpASTNode(nif->Body, indentation + 4);
                    DumpASTNode(nif->ElseBody, indentation + 4);

                    break;
                }

                case NodeType::Break: {
                    m_Output += "Break\n";
                    break;
                }

                case NodeType::Return: {
                    NodeReturn* ret = std::get<NodeReturn*>(n->Data);

                    m_Output += "Return, Value:\n";
                    DumpASTNode(ret->Value, indentation + 4);
                    break;
                }

                case NodeType::ArrayAccessExpr: {
                    NodeArrayAccessExpr* expr = std::get<NodeArrayAccessExpr*>(n->Data);

                    m_Output += "ArrayAccessExpr:\n";
                    DumpASTNode(expr->Parent, indentation + 4);
                    DumpASTNode(expr->Index, indentation + 4);
                    break;
                }

                case NodeType::MemberExpr: {
                    NodeMemberExpr* expr = std::get<NodeMemberExpr*>(n->Data);

                    m_Output += fmt::format("MemberExpr: {}\n", expr->Member);
                    DumpASTNode(expr->Parent, indentation + 4);
                    break;
                }

                case NodeType::MethodCallExpr: {
                    NodeMethodCallExpr* call = std::get<NodeMethodCallExpr*>(n->Data);

                    m_Output += fmt::format("MethodCallExpr, Name: {}\n", call->Member);
                    for (size_t i = 0; i < call->Arguments.Size; i++) {
                        DumpASTNode(call->Arguments.Items[i], indentation + 4);
                    }
                    DumpASTNode(call->Parent, indentation + 4);

                    break;
                }

                case NodeType::FunctionCallExpr: {
                    NodeFunctionCallExpr* call = std::get<NodeFunctionCallExpr*>(n->Data);

                    m_Output += fmt::format("FunctionCallExpr, Name: {}, Extern: {}\n", call->Name, call->Extern);
                    for (size_t i = 0; i < call->Arguments.Size; i++) {
                        DumpASTNode(call->Arguments.Items[i], indentation + 4);
                    }

                    break;
                }

                case NodeType::ParenExpr: {
                    NodeParenExpr* expr = std::get<NodeParenExpr*>(n->Data);

                    m_Output += "ParenExpr, Expression:\n";
                    DumpASTNode(expr->Expression, indentation + 4);
                    break;
                }

                case NodeType::CastExpr: {
                    NodeCastExpr* expr = std::get<NodeCastExpr*>(n->Data);

                    m_Output += fmt::format("CastExpr, Type: {}\n", VariableTypeToString(expr->ResolvedDstType));
                    DumpASTNode(expr->Expression, indentation + 4);
                    break;
                }

                case NodeType::UnaryExpr: {
                    NodeUnaryExpr* expr = std::get<NodeUnaryExpr*>(n->Data);

                    m_Output += fmt::format("UnaryExpr, Operation: {}\n", UnaryExprTypeToString(expr->Type));
                    DumpASTNode(expr->Expression, indentation + 4);
                    break;
                }

                case NodeType::BinExpr: {
                    NodeBinExpr* expr = std::get<NodeBinExpr*>(n->Data);

                    m_Output += fmt::format("BinExpr, Operation: {}\n", BinExprTypeToString(expr->Type));
                    DumpASTNode(expr->LHS, indentation + 4);
                    DumpASTNode(expr->RHS, indentation + 4);
                    break;
                }
            }
        }
    }

} // namespace BlackLua::Internal