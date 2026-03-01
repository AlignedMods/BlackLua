#pragma once

#include "internal/compiler/type_info.hpp"
#include "internal/compiler/core/string_builder.hpp"
#include "internal/compiler/ast/expr.hpp"
#include "internal/compiler/ast/stmt.hpp"

namespace BlackLua::Internal {
    
    enum class ConversionType {
        None,
        Promotion,
        Narrowing,
        SignChange,
        LValueToRValue
    };

    struct ConversionCost {
        ConversionType ConversionType = ConversionType::None;
        CastType CastType = CastType::Integral;

        bool CastNeeded = false;
        bool SignedMismatch = false;
        bool LValueMismatch = false;
        bool ImplicitCastPossible = false;
        bool ExplicitCastPossible = false;
    };

    // NOTE: The type checker serves as a prepass to the main semantic analysis
    // Its job is to resolve all type ambiguities and implicit casts
    class TypeChecker {
    public:
        TypeChecker(Context* ctx, ASTNodes* nodes);

    private:
        void CheckImpl();

        TypeInfo* HandleExprConstant(NodeExpr* expr);
        TypeInfo* HandleExprVarRef(NodeExpr* expr);
        TypeInfo* HandleExprArrayAccess(NodeExpr* expr);
        TypeInfo* HandleExprSelf(NodeExpr* expr);
        TypeInfo* HandleExprMember(NodeExpr* expr);
        TypeInfo* HandleExprMethodCall(NodeExpr* expr);
        TypeInfo* HandleExprCall(NodeExpr* expr);
        TypeInfo* HandleExprParen(NodeExpr* expr);
        TypeInfo* HandleExprCast(NodeExpr* expr);
        TypeInfo* HandleExprUnaryOperator(NodeExpr* expr);
        TypeInfo* HandleExprBinaryOperator(NodeExpr* expr);

        TypeInfo* HandleNodeExpression(NodeExpr* expr);

        void HandleStmtCompound(NodeStmt* stmt);
        void HandleStmtVarDecl(NodeStmt* stmt);
        void HandleStmtParamDecl(NodeStmt* stmt);
        void HandleStmtFunctionDecl(NodeStmt* stmt);
        void HandleStmtStructDecl(NodeStmt* stmt);
        void HandleStmtFieldDecl(NodeStmt* stmt);
        void HandleStmtMethodDecl(NodeStmt* stmt);
        void HandleStmtWhile(NodeStmt* stmt);
        void HandleStmtDoWhile(NodeStmt* stmt);
        void HandleStmtFor(NodeStmt* stmt);
        void HandleStmtIf(NodeStmt* stmt);
        void HandleStmtReturn(NodeStmt* stmt);

        void HandleNodeStatement(NodeStmt* stmt);

        void HandleNode(Node* node);

        TypeInfo* GetTypeInfoFromString(StringView str, bool lvalue);

        // type1 is the destination type and type2 is the source type
        ConversionCost GetConversionCost(TypeInfo* dst, TypeInfo* src, bool srcLValue);
        TypeInfo* InsertImplicitCast(TypeInfo* dstType, TypeInfo* srcType, NodeExpr* srcExpr, CastType castType);

        template <typename T>
        T* Allocate() {
            return m_Context->GetAllocator()->AllocateNamed<T>();
        }

        template <typename T, typename... Args>
        T* Allocate(Args&&... args) {
            return m_Context->GetAllocator()->AllocateNamed<T>(std::forward<Args>(args)...);
        }

    private:
        ASTNodes* m_Nodes = nullptr;

        std::vector<std::unordered_map<std::string, TypeInfo*>> m_Declarations;
        TypeInfo* m_ActiveReturnType = nullptr;

        Context* m_Context = nullptr;
        friend class SemanticAnalyzer;
    };

} // namespace BlackLua::Internal