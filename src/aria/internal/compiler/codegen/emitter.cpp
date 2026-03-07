#include "aria/internal/compiler/codegen/emitter.hpp"
#include "aria/internal/compiler/ast/ast.hpp"

namespace Aria::Internal {

    // Some code i got from cppreference (https://en.cppreference.com/w/cpp/utility/variant/visit)
    template<class... Ts>
    struct Overloads : Ts... { using Ts::operator()...; };

    Emitter::Emitter(CompilationContext* ctx) {
        m_Context = ctx;
        m_RootASTNode = ctx->GetRootASTNode();

        EmitImpl();
    }

    void Emitter::EmitImpl() {
        m_OpCodes.emplace_back(OpCodeType::Function, "_start$");
        m_OpCodes.emplace_back(OpCodeType::Label, "_entry$");
        PushStackFrame();

        EmitStmt(m_RootASTNode);

        m_OpCodes.emplace_back(OpCodeType::Ret);

        // EmitFunctions();

        m_Context->SetOpCodes(m_OpCodes);
    }

    Emitter::CompileMemRef Emitter::EmitBooleanConstantExpr(Expr* expr) {
        BooleanConstantExpr* bc = GetNode<BooleanConstantExpr>(expr);

        m_OpCodes.emplace_back(OpCodeType::LoadI8, OpCodeLoad(static_cast<i8>(bc->GetValue()), bc->GetResolvedType()));
        return GetStackTop();
    }

    Emitter::CompileMemRef Emitter::EmitCharacterConstantExpr(Expr* expr) {
        CharacterConstantExpr* cc = GetNode<CharacterConstantExpr>(expr);

        m_OpCodes.emplace_back(OpCodeType::LoadI8, OpCodeLoad(cc->GetValue(), cc->GetResolvedType()));
        return GetStackTop();
    }

    Emitter::CompileMemRef Emitter::EmitIntegerConstantExpr(Expr* expr) {
        IntegerConstantExpr* ic = GetNode<IntegerConstantExpr>(expr);

        const auto visitor = Overloads
        {
            [this, ic](i8 i)  { m_OpCodes.emplace_back(OpCodeType::LoadI8,  OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](i16 i) { m_OpCodes.emplace_back(OpCodeType::LoadI16, OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](i32 i) { m_OpCodes.emplace_back(OpCodeType::LoadI32, OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](i64 i) { m_OpCodes.emplace_back(OpCodeType::LoadI64, OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](u8 i)  { m_OpCodes.emplace_back(OpCodeType::LoadU8,  OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](u16 i) { m_OpCodes.emplace_back(OpCodeType::LoadU16, OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](u32 i) { m_OpCodes.emplace_back(OpCodeType::LoadU32, OpCodeLoad(i, ic->GetResolvedType())); },
            [this, ic](u64 i) { m_OpCodes.emplace_back(OpCodeType::LoadU64, OpCodeLoad(i, ic->GetResolvedType())); },
        };

        std::visit(visitor, ic->GetValue());
        return GetStackTop();
    }

    Emitter::CompileMemRef Emitter::EmitFloatingConstantExpr(Expr* expr) {
        FloatingConstantExpr* fc = GetNode<FloatingConstantExpr>(expr);

        const auto visitor = Overloads
        {
            [this, fc](f32 f) { m_OpCodes.emplace_back(OpCodeType::LoadF32, OpCodeLoad(f, fc->GetResolvedType())); },
            [this, fc](f64 f) { m_OpCodes.emplace_back(OpCodeType::LoadF64, OpCodeLoad(f, fc->GetResolvedType())); },
        };

        std::visit(visitor, fc->GetValue());
        return GetStackTop();
    }

    Emitter::CompileMemRef Emitter::EmitStringConstantExpr(Expr* expr) {
        StringConstantExpr* sc = GetNode<StringConstantExpr>(expr);

        m_OpCodes.emplace_back(OpCodeType::LoadStr, OpCodeLoad(sc->GetValue(), sc->GetResolvedType()));
        return GetStackTop();
    }

    Emitter::CompileMemRef Emitter::EmitExpr(Expr* expr) {
        if (GetNode<BooleanConstantExpr>(expr)) {
            return EmitBooleanConstantExpr(expr);
        } else if (GetNode<CharacterConstantExpr>(expr)) {
            return EmitCharacterConstantExpr(expr);
        } else if (GetNode<IntegerConstantExpr>(expr)) {
            return EmitIntegerConstantExpr(expr);
        } else if (GetNode<FloatingConstantExpr>(expr)) {
            return EmitFloatingConstantExpr(expr);
        } else if (GetNode<StringConstantExpr>(expr)) {
            return EmitStringConstantExpr(expr);
        }

        ARIA_UNREACHABLE();
    }

    void Emitter::EmitTranslationUnitDecl(Decl* decl) {
        TranslationUnitDecl* tu = GetNode<TranslationUnitDecl>(decl);

        for (Stmt* stmt : tu->GetStmts()) {
            EmitStmt(stmt);
        }
    }

    void Emitter::EmitVarDecl(Decl* decl) {
        VarDecl* varDecl = GetNode<VarDecl>(decl);

        if (varDecl->GetDefaultValue()) {
            EmitExpr(varDecl->GetDefaultValue());
        } else {
            m_OpCodes.emplace_back(OpCodeType::Alloca, OpCodeAlloca(varDecl->GetResolvedType()->GetSize(), varDecl->GetResolvedType()));
            IncrementStackSlotCount();
        }

        Declaration d;
        d.Mem = GetStackTop();
        d.Type = varDecl->GetResolvedType();

        m_StackFrames.back().DeclaredSymbols.push_back(d);
        m_StackFrames.back().DeclaredSymbolMap[varDecl->GetIdentifier()] = m_StackFrames.back().DeclaredSymbols.size() - 1;

        if (varDecl->IsGlobal()) {
            m_OpCodes.emplace_back(OpCodeType::SetGlobal, OpCodeSetGlobal(varDecl->GetIdentifier()));
        }
    }
    
    void Emitter::EmitParamDecl(Decl* decl) {
        ParamDecl* paramDecl = GetNode<ParamDecl>(decl);

        // PushBytes(paramDecl->GetResolvedType()->GetSize(), paramDecl->GetResolvedType());
        // IncrementStackSlotCount();
    }

    void Emitter::EmitFunctionDecl(Decl* decl) {
        ARIA_ASSERT(false, "todo: Emitter::EmitFunctionDecl()");
        // FunctionDecl* fnDecl = GetNode<FunctionDecl>(decl);

        // if (fnDecl->IsExtern()) { return; }

        // Declaration d;
        // d.Slot.Slot = m_LabelCount++;
        // d.Type = fnDecl->GetResolvedType();

        // m_StackFrames.back().DeclaredSymbols.push_back(d);
        // m_StackFrames.back().DeclaredSymbolMap[fnDecl->GetIdentifier()] = m_StackFrames.back().DeclaredSymbols.size() - 1;
    }

    void Emitter::EmitDecl(Decl* decl) {
        if (GetNode<TranslationUnitDecl>(decl)) {
            EmitTranslationUnitDecl(decl);
            return;
        } else if (GetNode<VarDecl>(decl)) {
            EmitVarDecl(decl);
            return;
        } else if (GetNode<ParamDecl>(decl)) {
            EmitParamDecl(decl);
            return;
        } else if (GetNode<FunctionDecl>(decl)) {
            EmitFunctionDecl(decl);
            return;
        }

        ARIA_UNREACHABLE();
    }

    void Emitter::EmitCompoundStmt(Stmt* stmt) {
        CompoundStmt* compound = GetNode<CompoundStmt>(stmt);

        for (Stmt* stmt : compound->GetStmts()) {
            EmitStmt(stmt);
        }
    }

    void Emitter::EmitWhileStmt(Stmt* stmt) { ARIA_ASSERT(false, "todo: Emitter::EmitWhileStmt()"); }
    void Emitter::EmitDoWhileStmt(Stmt* stmt) { ARIA_ASSERT(false, "todo: Emitter::EmitDoWhileStmt()"); }
    void Emitter::EmitForStmt(Stmt* stmt) { ARIA_ASSERT(false, "todo: Emitter::EmitForStmt()"); }
    void Emitter::EmitIfStmt(Stmt* stmt) { ARIA_ASSERT(false, "todo: Emitter::EmitIfStmt()"); }

    void Emitter::EmitStmt(Stmt* stmt) {
        if (GetNode<CompoundStmt>(stmt)) {
            EmitCompoundStmt(stmt);
            return;
        } else if (GetNode<WhileStmt>(stmt)) {
            EmitWhileStmt(stmt);
            return;
        } else if (GetNode<DoWhileStmt>(stmt)) {
            EmitDoWhileStmt(stmt);
            return;
        } else if (GetNode<ForStmt>(stmt)) {
            EmitForStmt(stmt);
            return;
        } else if (GetNode<IfStmt>(stmt)) {
            EmitIfStmt(stmt);
            return;
        } else if (Expr* expr = GetNode<Expr>(stmt)) {
            EmitExpr(expr);
            return;
        } else if (Decl* decl = GetNode<Decl>(stmt)) {
            EmitDecl(decl);
            return;
        }

        ARIA_UNREACHABLE();
    }

    MemRef Emitter::CompileToRuntimeMemRef(CompileMemRef slot) {
        if (slot.Mem.ContainsStackSlot()) {
            auto s = slot.Mem.GetStackSlot();

            return MemRef(s.Slot - static_cast<i32>(m_StackFrames.back().SlotCount - 1), s.Offset, s.Size);
        } else {
            return slot.Mem;
        }
        
        ARIA_UNREACHABLE();
    }

    bool Emitter::IsStackFrameGlobal() { return m_StackFrames.size() == 1; }

    void Emitter::IncrementStackSlotCount() {
        m_StackFrames.back().SlotCount++;
    }

    Emitter::CompileMemRef Emitter::GetStackTop() {
        return CompileMemRef(MemRef(m_StackFrames.back().SlotCount, 0, 0));
    }

    void Emitter::EmitDestructors(const std::vector<Declaration>& declarations) {
        for (auto it = declarations.rbegin(); it != declarations.rend(); it++) {
            auto& decl = *it;

            // if (decl.Destruct) {
            //     if (decl.Type->Type == PrimitiveType::Array) {
            //         m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(decl.Slot));
            //         m_OpCodes.emplace_back(OpCodeType::CallExtern, "bl__array__destruct__");
            //     }
            // }
        }
    }

    void Emitter::PushStackFrame() {
        m_OpCodes.emplace_back(OpCodeType::PushSF);

        StackFrame sf;
        sf.SlotCount = (m_StackFrames.size() > 0) ? m_StackFrames.back().SlotCount : 0;

        m_StackFrames.push_back(sf);
    }

    void Emitter::PopStackFrame() {
        m_OpCodes.emplace_back(OpCodeType::PopSF);
        m_StackFrames.pop_back();
    }

    void Emitter::EmitFunctions() {
        ARIA_ASSERT(false, "todo: Emitter::EmitFunctions()");
        // for (const auto&[label, stmt] : m_FunctionsToDeclare) {
        //     if (FunctionDecl* decl = GetNode<FunctionDecl>(stmt)) {
        //         if (decl->GetBody()) {
        //             m_OpCodes.emplace_back(OpCodeType::Label, label);

        //             PushCompilerStackFrame();
        //             
        //             size_t returnSlot = (decl->GetResolvedType()->Type == PrimitiveType::Void) ? 0 : 1;
        //             
        //             for (ParamDecl* p : decl->GetParameters()) {
        //                 EmitParamDecl(p);
        //                 int32_t argSlot = -static_cast<int32_t>(decl->GetParameters().Size + 1 + returnSlot); // The slot where the argument got passed from
        //                 m_OpCodes.emplace_back(OpCodeType::Dup, argSlot);
        //             }
        //             
        //             EmitCompoundStmt(decl->GetBody());

        //             if (m_OpCodes.back().Type != OpCodeType::Ret) {
        //                 EmitDestructors(m_StackFrames.back().DeclaredSymbols);
        //                 m_OpCodes.emplace_back(OpCodeType::Ret);
        //             }
        //             
        //             PopCompilerStackFrame();
        //         }
        //     }
        // }
    }

} // namespace Aria::Internal
