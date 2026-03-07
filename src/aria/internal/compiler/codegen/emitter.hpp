#pragma once

#include "aria/internal/compiler/ast/expr.hpp"
#include "aria/internal/compiler/ast/stmt.hpp"
#include "aria/internal/compiler/ast/decl.hpp"
#include "aria/internal/vm/vm.hpp"
#include "aria/internal/compiler/reflection/compiler_reflection.hpp"

namespace Aria::Internal {

    class Emitter {
    private:
        struct CompileMemRef {
            MemRef Mem;
        };

        struct Declaration {
            CompileMemRef Mem;
            TypeInfo* Type = nullptr;
        };

        struct StackFrame {
            size_t SlotCount = 0;
            std::unordered_map<std::string, size_t> DeclaredSymbolMap;
            std::vector<Declaration> DeclaredSymbols;
        };

    public:
        Emitter(CompilationContext* ctx);

    private:
        void EmitImpl();

        CompileMemRef EmitBooleanConstantExpr(Expr* expr);
        CompileMemRef EmitCharacterConstantExpr(Expr* expr);
        CompileMemRef EmitIntegerConstantExpr(Expr* expr);
        CompileMemRef EmitFloatingConstantExpr(Expr* expr);
        CompileMemRef EmitStringConstantExpr(Expr* expr);
        CompileMemRef EmitVarRefExpr(Expr* expr);
        CompileMemRef EmitCallExpr(Expr* expr);
        CompileMemRef EmitParenExpr(Expr* expr);
        CompileMemRef EmitCastExpr(Expr* expr);
        CompileMemRef EmitUnaryOperatorExpr(Expr* expr);
        CompileMemRef EmitBinaryOperatorExpr(Expr* expr);

        CompileMemRef EmitExpr(Expr* expr);

        void EmitTranslationUnitDecl(Decl* decl);
        void EmitVarDecl(Decl* decl);
        void EmitParamDecl(Decl* decl);
        void EmitFunctionDecl(Decl* decl);

        void EmitDecl(Decl* decl);

        void EmitCompoundStmt(Stmt* stmt);
        void EmitWhileStmt(Stmt* stmt);
        void EmitDoWhileStmt(Stmt* stmt);
        void EmitForStmt(Stmt* stmt);
        void EmitIfStmt(Stmt* stmt);
        void EmitReturnStmt(Stmt* stmt);

        void EmitStmt(Stmt* stmt);

        void EmitFunctions(); // Emits all the defined functions

        MemRef CompileToRuntimeMemRef(CompileMemRef mem);

        bool IsStackFrameGlobal();
        void IncrementStackSlotCount();
        CompileMemRef GetStackTop();

        void EmitDestructors(const std::vector<Declaration>& declarations);

        void PushStackFrame();
        void PopStackFrame();

    private:
        std::vector<OpCode> m_OpCodes;
        CompilerReflectionData m_ReflectionData;

        Stmt* m_RootASTNode = nullptr;

        std::vector<StackFrame> m_StackFrames; // There is always one implicit top stack frame (global space)
        std::unordered_map<int32_t, Decl*> m_FunctionsToDeclare; // We do not immediately declare functions, we actually do them last
    
        CompilationContext* m_Context = nullptr;
    };

} // namespace Aria::Internal
