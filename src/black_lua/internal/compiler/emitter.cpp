#include "internal/compiler/emitter.hpp"
#include "context.hpp"

#include <string>

namespace BlackLua::Internal {

    Emitter Emitter::Emit(const ASTNodes* nodes, Context* ctx) {
        Emitter e;
        e.m_Nodes = nodes;
        e.m_Context = ctx;
        e.EmitImpl();
        return e;
    }

    const CompilerReflectionData& Emitter::GetReflectionData() const {
        return m_ReflectionData;
    }

    const Emitter::OpCodes& Emitter::GetOpCodes() const {
        return m_OpCodes;
    }

    void Emitter::EmitImpl() {
        // Emit the constants first
        // while (Peek()) {
        //     EmitConstant(Consume());
        // }
        // 
        // // Reset the index
        // m_Index = 0;

        while (Peek()) {
            EmitNode(Consume());
        }
    }

    Node* Emitter::Peek() {
        if (m_Index < m_Nodes->size()) {
            return m_Nodes->at(m_Index);
        } else {
            return nullptr;
        }
    }

    Node* Emitter::Consume() {
        BLUA_ASSERT(m_Index < m_Nodes->size(), "Consume() of out bounds!");

        return m_Nodes->at(m_Index++);
    }

    StackSlotIndex Emitter::CompileToRuntimeStackSlot(CompileStackSlot slot) {
        if (slot.Relative) {
            if (m_CurrentStackFrame) {
                return StackSlotIndex(slot.Slot.Slot - static_cast<int32_t>(m_CurrentStackFrame->SlotCount) - 1, slot.Slot.Offset, slot.Slot.Size);
            } else {
                return StackSlotIndex(slot.Slot.Slot - m_SlotCount - 1, slot.Slot.Offset, slot.Slot.Size);
            }
        } else {
            return slot.Slot;
        }
        
        BLUA_ASSERT(false, "Unreachable!");
    }

    int32_t Emitter::CreateLabel(const std::string& debugData) {
        m_OpCodes.emplace_back(OpCodeType::Label, static_cast<int32_t>(m_LabelCount), debugData);
        m_LabelCount++;
        return static_cast<int32_t>(m_LabelCount - 1);
    }

    void Emitter::PushBytes(size_t bytes, const std::string& debugData) {
        m_OpCodes.emplace_back(OpCodeType::Push, static_cast<int32_t>(bytes), debugData);
    }

    void Emitter::IncrementStackSlotCount() {
        (m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount++ : m_SlotCount++;
    }

    void Emitter::PushStackFrame() {
        m_OpCodes.emplace_back(OpCodeType::PushStackFrame);

        StackFrame* newStackFrame = m_Context->GetAllocator()->AllocateNamed<StackFrame>();
        newStackFrame->Parent = m_CurrentStackFrame;
        newStackFrame->SlotCount = (newStackFrame->Parent) ? newStackFrame->Parent->SlotCount : 0;
        m_CurrentStackFrame = newStackFrame;
    }

    void Emitter::PushCompilerStackFrame() {
        StackFrame* newStackFrame = m_Context->GetAllocator()->AllocateNamed<StackFrame>();
        newStackFrame->Parent = m_CurrentStackFrame;
        newStackFrame->SlotCount = (newStackFrame->Parent) ? newStackFrame->Parent->SlotCount : 0;
        m_CurrentStackFrame = newStackFrame;
    }

    void Emitter::PopStackFrame() {
        m_OpCodes.emplace_back(OpCodeType::PopStackFrame);
        m_CurrentStackFrame = m_CurrentStackFrame->Parent;
    }

    void Emitter::PopCompilerStackFrame() {
        m_CurrentStackFrame = m_CurrentStackFrame->Parent;
    }

    CompileStackSlot Emitter::EmitNodeExpression(NodeExpr* expr) {
        if (ExprConstant* constant = GetNode<ExprConstant>(expr)) {
            IncrementStackSlotCount();

            if (ConstantBool* b = GetNode<ConstantBool>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadI8, OpCodeLoad(b->Value));
            }

            if (ConstantChar* c = GetNode<ConstantChar>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadI8, OpCodeLoad(c->Char));
            }

            if (ConstantInt* i = GetNode<ConstantInt>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadI32, OpCodeLoad(i->Int));
            }

            if (ConstantLong* l = GetNode<ConstantLong>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadI64, OpCodeLoad(l->Long));
            }

            if (ConstantFloat* f = GetNode<ConstantFloat>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadF32, OpCodeLoad(f->Float));
            }

            if (ConstantDouble* d = GetNode<ConstantDouble>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadF64, OpCodeLoad(d->Double));
            }

            if (ConstantString* s = GetNode<ConstantString>(constant)) {
                m_OpCodes.emplace_back(OpCodeType::LoadStr, OpCodeLoad(s->String));
            }

            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }

        if (ExprVarRef* ref = GetNode<ExprVarRef>(expr)) {
            std::string ident = fmt::format("{}", ref->Identifier);
        
            // Loop backward through all the scopes
            StackFrame* currentStackFrame = m_CurrentStackFrame;
            while (currentStackFrame) {
                if (currentStackFrame->DeclaredSymbols.contains(ident)) {
                    return CompileStackSlot(currentStackFrame->DeclaredSymbols.at(ident).Index, true);
                }
        
                currentStackFrame = currentStackFrame->Parent;
            }
        
            // Check global symbols
            if (m_DeclaredSymbols.contains(fmt::format("{}", ref->Identifier))) {
                return CompileStackSlot(m_DeclaredSymbols.at(fmt::format("{}", ref->Identifier)).Index, false);
            }
        
            BLUA_ASSERT(false, "Unreachable!");
            return {};
        }

        if (ExprArrayAccess* arrAccess = GetNode<ExprArrayAccess>(expr)) {
            CompileStackSlot slot = EmitNodeExpression(arrAccess->Parent);
            CompileStackSlot index = EmitNodeExpression(arrAccess->Index);
        
            m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(slot));
            IncrementStackSlotCount();
            m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(index));
            IncrementStackSlotCount();
            m_OpCodes.emplace_back(OpCodeType::Push, GetTypeSize(arrAccess->ResolvedType));
            IncrementStackSlotCount();
            m_OpCodes.emplace_back(OpCodeType::CallExtern, "bl__array__index__");
        
            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }

        if (ExprSelf* self = GetNode<ExprSelf>(expr)) {
            return CompileStackSlot(m_CurrentStackFrame->DeclaredSymbols.at("bL__internal__self_restrict__").Index, true);
        }

        if (ExprMember* mem = GetNode<ExprMember>(expr)) {
            CompileStackSlot slot = EmitNodeExpression(mem->Parent);
        
            VariableType* structType = mem->ResolvedParentType;
            for (const auto& field : std::get<StructDeclaration>(structType->Data).Fields) {
                if (field.Identifier == mem->Member) {
                    CompileStackSlot s = slot;
                    s.Slot.Offset += field.Offset;
                    s.Slot.Size = GetTypeSize(field.ResolvedType);
                    return s;
                }
            }
        
            BLUA_ASSERT(false, "Unreachable!");
        
            return {};
        }

        if (ExprMethodCall* call = GetNode<ExprMethodCall>(expr)) {
            CompileStackSlot slot = EmitNodeExpression(call->Parent);
            VariableType* structType = call->ResolvedParentType;
            Declaration decl = m_DeclaredSymbols.at(fmt::format("{}__{}", std::get<StructDeclaration>(structType->Data).Identifier, call->Member));
        
            std::vector<CompileStackSlot> parameters; 
        
            for (size_t i = 0; i < call->Arguments.Size; i++) {
                CompileStackSlot slot = EmitNodeExpression(GetNode<NodeExpr>(call->Arguments.Items[i]));
        
                parameters.push_back(slot);
            }

            m_OpCodes.emplace_back(OpCodeType::Ref, CompileToRuntimeStackSlot(slot));
            IncrementStackSlotCount();
        
            for (const auto& p : parameters) {
                m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(p));
                IncrementStackSlotCount();
            }
        
            if (decl.Size != 0) {
                PushBytes(decl.Size, "return slot");
                IncrementStackSlotCount();
            }
        
            m_OpCodes.emplace_back(OpCodeType::Call, decl.Index);
            
            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }

        if (ExprCall* call = GetNode<ExprCall>(expr)) {
            if (call->Extern) {
                std::vector<CompileStackSlot> parameters; 
        
                for (size_t i = 0; i < call->Arguments.Size; i++) {
                    CompileStackSlot slot = EmitNodeExpression(GetNode<NodeExpr>(call->Arguments.Items[i]));
                    parameters.push_back(slot);
                }
        
                for (const auto& p : parameters) {
                    m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(p));
                    IncrementStackSlotCount();
                }
        
                if (call->ResolvedReturnType->Type != PrimitiveType::Void) {
                    PushBytes(GetTypeSize(call->ResolvedReturnType), "return slot");
                    IncrementStackSlotCount();
                }

                m_OpCodes.emplace_back(OpCodeType::CallExtern, fmt::format("{}", call->Name));
            } else {
                Declaration decl = m_DeclaredSymbols.at(fmt::format("{}", call->Name));
        
                std::vector<CompileStackSlot> parameters; 
        
                for (size_t i = 0; i < call->Arguments.Size; i++) {
                    CompileStackSlot slot = EmitNodeExpression(GetNode<NodeExpr>(call->Arguments.Items[i]));
                    parameters.push_back(slot);
                }
        
                for (const auto& p : parameters) {
                    m_OpCodes.emplace_back(OpCodeType::Dup, CompileToRuntimeStackSlot(p));
                    IncrementStackSlotCount();
                }
        
                if (call->ResolvedReturnType->Type != PrimitiveType::Void) {
                    PushBytes(GetTypeSize(call->ResolvedReturnType), "return slot");
                    IncrementStackSlotCount();
                }

                m_OpCodes.emplace_back(OpCodeType::Call, decl.Index);
            }
            
            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }
        
        if (ExprParen* paren = GetNode<ExprParen>(expr)) {
            return EmitNodeExpression(paren->Expression);
        }
        
        if (ExprCast* cast = GetNode<ExprCast>(expr)) {
            CompileStackSlot slot = EmitNodeExpression(cast->Expression);
        
            #define CASE_INNER(primType, op) case PrimitiveType::primType: m_OpCodes.emplace_back(OpCodeType::op, CompileToRuntimeStackSlot(slot)); break
        
            #define CASE_OUTER(primType, op, prefix) case PrimitiveType::primType: { \
                switch (cast->ResolvedDstType->Type) { \
                    CASE_INNER(Bool, Cast##op##To##prefix##8); \
                    CASE_INNER(Char, Cast##op##To##prefix##8); \
                    CASE_INNER(Short, Cast##op##To##prefix##16); \
                    CASE_INNER(Int, Cast##op##To##prefix##32); \
                    CASE_INNER(Float, Cast##op##ToF32); \
                    CASE_INNER(Double, Cast##op##ToF64); \
                } \
                break; \
            }
        
            if (cast->ResolvedSrcType->Signed) {
                if (cast->ResolvedDstType->Signed) {
                    switch (cast->ResolvedSrcType->Type) {
                        CASE_OUTER(Bool,   I8,  I)
                        CASE_OUTER(Char,   I8,  I)
                        CASE_OUTER(Short,  I16, I)
                        CASE_OUTER(Int,    I32, I)
                        CASE_OUTER(Long,   I64, I)
                        CASE_OUTER(Float,  F32, I)
                        CASE_OUTER(Double, F64, I)
                    }
                } else {
                    switch (cast->ResolvedSrcType->Type) {
                        CASE_OUTER(Bool,   I8,  U)
                        CASE_OUTER(Char,   I8,  U)
                        CASE_OUTER(Short,  I16, U)
                        CASE_OUTER(Int,    I32, U)
                        CASE_OUTER(Long,   I64, U)
                        CASE_OUTER(Float,  F32, U)
                        CASE_OUTER(Double, F64, U)
                    }
                }
            } else {
                if (cast->ResolvedDstType->Signed) {
                    switch (cast->ResolvedSrcType->Type) {
                        CASE_OUTER(Bool,   U8,  I)
                        CASE_OUTER(Char,   U8,  I)
                        CASE_OUTER(Short,  U16, I)
                        CASE_OUTER(Int,    U32, I)
                        CASE_OUTER(Long,   U64, I)
                        CASE_OUTER(Float,  F32, I)
                        CASE_OUTER(Double, F64, I)
                    }
                } else {
                    switch (cast->ResolvedSrcType->Type) {
                        CASE_OUTER(Bool,   U8,  U)
                        CASE_OUTER(Char,   U8,  U)
                        CASE_OUTER(Short,  U16, U)
                        CASE_OUTER(Int,    U32, U)
                        CASE_OUTER(Long,   U64, U)
                        CASE_OUTER(Float,  F32, U)
                        CASE_OUTER(Double, F64, U)
                    }
                }
            }
        
            #undef CASE_INNER
            #undef CASE_OUTER
            
            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }
       
        if (ExprUnaryOperator* unOp = GetNode<ExprUnaryOperator>(expr)) {
            CompileStackSlot slot = EmitNodeExpression(unOp->Expression);
                
            #define MATH_OP(baseOp, type, _enum) \
                if (unOp->ResolvedType->Type == PrimitiveType::_enum) { \
                    m_OpCodes.emplace_back(OpCodeType::baseOp##type, CompileToRuntimeStackSlot(slot)); \
                    IncrementStackSlotCount(); \
                }
            
            #define MATH_OP_GROUP(unaryExpr, op) case UnaryOperatorType::unaryExpr: { \
                if (unOp->ResolvedType->Signed) { \
                    MATH_OP(op, I8, Bool) \
                    MATH_OP(op, I8, Char) \
                    MATH_OP(op, I16, Short) \
                    MATH_OP(op, I32, Int) \
                    MATH_OP(op, I64, Long) \
                } else { \
                    MATH_OP(op, U8, Bool) \
                    MATH_OP(op, U8, Char) \
                    MATH_OP(op, U16, Short) \
                    MATH_OP(op, U32, Int) \
                    MATH_OP(op, U64, Long) \
                } \
                \
                MATH_OP(op, F32, Float) \
                MATH_OP(op, F64, Double) \
                break; \
            }
            
            switch (unOp->Type) {
                MATH_OP_GROUP(Negate, Negate)
            }
            
            #undef MATH_OP
            #undef MATH_OP_GROUP
            
            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }
        
        if (ExprBinaryOperator* binOp = GetNode<ExprBinaryOperator>(expr)) {
            CompileStackSlot rhs = EmitNodeExpression(binOp->RHS);
            CompileStackSlot lhs = EmitNodeExpression(binOp->LHS);
            
            #define MATH_OP(baseOp, type, _enum) \
                if (binOp->ResolvedSourceType->Type == PrimitiveType::_enum) { \
                    m_OpCodes.emplace_back(OpCodeType::baseOp##type, OpCodeMath(CompileToRuntimeStackSlot(lhs), CompileToRuntimeStackSlot(rhs))); \
                    IncrementStackSlotCount(); \
                }
            
            #define MATH_OP_GROUP(binExpr, op) case BinaryOperatorType::binExpr: { \
                if (binOp->ResolvedSourceType->Signed) { \
                    MATH_OP(op, I8, Bool) \
                    MATH_OP(op, I8, Char) \
                    MATH_OP(op, I16, Short) \
                    MATH_OP(op, I32, Int) \
                    MATH_OP(op, I64, Long) \
                } else { \
                    MATH_OP(op, U8, Bool) \
                    MATH_OP(op, U8, Char) \
                    MATH_OP(op, U16, Short) \
                    MATH_OP(op, U32, Int) \
                    MATH_OP(op, U64, Long) \
                } \
                \
                MATH_OP(op, F32, Float) \
                MATH_OP(op, F64, Double) \
                break; \
            }
            
            switch (binOp->Type) {
                MATH_OP_GROUP(Add, Add)
                MATH_OP_GROUP(AddInPlace, Add)
                MATH_OP_GROUP(Sub, Sub);
                MATH_OP_GROUP(SubInPlace, Sub);
                MATH_OP_GROUP(Mul, Mul);
                MATH_OP_GROUP(MulInPlace, Mul);
                MATH_OP_GROUP(Div, Div);
                MATH_OP_GROUP(DivInPlace, Div);
                MATH_OP_GROUP(Mod, Mod);
                MATH_OP_GROUP(ModInPlace, Mod);
            
                MATH_OP_GROUP(Less, Lt);
                MATH_OP_GROUP(LessOrEq, Lte);
                MATH_OP_GROUP(Greater, Gt);
                MATH_OP_GROUP(GreaterOrEq, Gte);
                MATH_OP_GROUP(IsEq, Cmp);
                MATH_OP_GROUP(IsNotEq, Ncmp);
                
                case BinaryOperatorType::Eq: {
                    m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(CompileToRuntimeStackSlot(lhs), CompileToRuntimeStackSlot(rhs)));

                    return lhs;
                }
            }
            
            switch (binOp->Type) {
                case BinaryOperatorType::AddInPlace:
                case BinaryOperatorType::SubInPlace:
                case BinaryOperatorType::MulInPlace:
                case BinaryOperatorType::DivInPlace:
                case BinaryOperatorType::ModInPlace: {
                    m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(CompileToRuntimeStackSlot(lhs), -1));
                    return lhs;
                }
            }
            
            #undef MATH_OP
            #undef MATH_OP_GROUP
            
            return CompileStackSlot((m_CurrentStackFrame) ? m_CurrentStackFrame->SlotCount : m_SlotCount, (m_CurrentStackFrame) ? true : false);
        }
    }

    void Emitter::EmitNodeCompound(NodeStmt* stmt) {
        StmtCompound* compound = GetNode<StmtCompound>(stmt);

        for (size_t i = 0; i < compound->Nodes.Size; i++) {
            EmitNode(compound->Nodes.Items[i]);
        }
    }

    void Emitter::EmitNodeVarDecl(NodeStmt* stmt) {
        StmtVarDecl* decl = GetNode<StmtVarDecl>(stmt);
        
        PushBytes(GetTypeSize(decl->ResolvedType), fmt::format("Declaration of {}", decl->Identifier));
        
        auto& map = (m_CurrentStackFrame != 0) ? m_CurrentStackFrame->DeclaredSymbols : m_DeclaredSymbols;
        
        Declaration d;
        IncrementStackSlotCount();
        if (m_CurrentStackFrame) {
            d.Index = m_CurrentStackFrame->SlotCount;
        } else {
            d.Index = m_SlotCount;

            CompilerReflectionDeclaration dd;
            dd.Type = ReflectionType::Variable;
            dd.Data = d.Index;
            dd.ResolvedType = decl->ResolvedType;
            m_ReflectionData.Declarations[fmt::format("{}", decl->Identifier)] = dd;
        }
        d.Size = GetTypeSize(decl->ResolvedType);
        d.Type = decl->ResolvedType;
        
        map[fmt::format("{}", decl->Identifier)] = d;

        // We convert int var = 5 to
        // int var;
        // var = 5;
        if (decl->Value) {
            ExprBinaryOperator* expr = m_Context->GetAllocator()->AllocateNamed<ExprBinaryOperator>();
            ExprVarRef* ref = m_Context->GetAllocator()->AllocateNamed<ExprVarRef>();
        
            ref->Identifier = decl->Identifier;
        
            expr->Type = BinaryOperatorType::Eq;
            expr->ResolvedType = decl->ResolvedType;
            expr->LHS = m_Context->GetAllocator()->AllocateNamed<NodeExpr>(ref);
            expr->RHS = decl->Value;
        
            EmitNodeExpression(m_Context->GetAllocator()->AllocateNamed<NodeExpr>(expr));
        }
    }

    void Emitter::EmitNodeParamDecl(NodeStmt* stmt) {
        StmtParamDecl* decl = GetNode<StmtParamDecl>(stmt);
        
        PushBytes(GetTypeSize(decl->ResolvedType), fmt::format("Declaration of {}", decl->Identifier));
        auto& map = (m_CurrentStackFrame != 0) ? m_CurrentStackFrame->DeclaredSymbols : m_DeclaredSymbols;
        
        Declaration d;
        IncrementStackSlotCount();
        if (m_CurrentStackFrame) {
            d.Index = m_CurrentStackFrame->SlotCount;
        } else {
            d.Index = m_SlotCount;   
        }
        d.Size = GetTypeSize(decl->ResolvedType);
        d.Type = decl->ResolvedType;
        
        map[fmt::format("{}", decl->Identifier)] = d;
    }

    void Emitter::EmitNodeFunctionDecl(NodeStmt* stmt) {
        StmtFunctionDecl* decl = GetNode<StmtFunctionDecl>(stmt);
        
        if (!decl->Extern && !decl->Body) { return; }

        std::string ident = fmt::format("{}", decl->Name);
        Declaration d;
        d.Size = GetTypeSize(decl->ResolvedType);
        d.Extern = decl->Extern;
        d.Type = decl->ResolvedType;
        m_DeclaredSymbols[ident] = d;

        if (decl->Body) {
            int32_t label = CreateLabel();

            CompilerReflectionDeclaration dd;
            dd.Type = ReflectionType::Function;
            dd.Data = label;
            dd.ResolvedType = decl->ResolvedType;
            m_ReflectionData.Declarations[ident] = dd;

            PushCompilerStackFrame();
            
            size_t returnSlot = (decl->ResolvedType->Type == PrimitiveType::Void) ? 0 : 1;
            size_t varsPushed = 0;
            
            // Inject function parameters into the scope
            for (size_t i = 0; i < decl->Parameters.Size; i++) {
                EmitNodeParamDecl(GetNode<NodeStmt>(decl->Parameters.Items[i]));
            
                // Copy the arguments into the parameters
                m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(-1, -static_cast<int32_t>(decl->Parameters.Size + 1 + returnSlot)));
                varsPushed++;
            }
            
            EmitNodeCompound(decl->Body);

            if (m_OpCodes.back().Type != OpCodeType::Ret) {
                m_OpCodes.emplace_back(OpCodeType::Ret);
            }
            
            PopCompilerStackFrame();
        }
    }

    void Emitter::EmitNodeStructDecl(NodeStmt* stmt) {
        StmtStructDecl* decl = GetNode<StmtStructDecl>(stmt);
        
        for (size_t i = 0; i < decl->Fields.Size; i++) {
            if (StmtMethodDecl* mdecl = GetNode<StmtMethodDecl>(GetNode<NodeStmt>(decl->Fields.Items[i]))) {
                std::string ident = fmt::format("{}__{}", decl->Identifier, mdecl->Name);
                Declaration decl;
                decl.Index = CreateLabel(fmt::format("method {}", mdecl->Name));
                decl.Size = GetTypeSize(mdecl->ResolvedType);
                decl.Type = mdecl->ResolvedType;
                m_DeclaredSymbols[ident] = decl;
                
                PushCompilerStackFrame();
                
                size_t returnSlot = (mdecl->ResolvedType->Type == PrimitiveType::Void) ? 0 : 1;

                Declaration s;
                s.Extern = false;
                s.Index = 0;
                m_CurrentStackFrame->DeclaredSymbols["bL__internal__self_restrict__"] = s;
                m_OpCodes.emplace_back(OpCodeType::Ref, -(mdecl->Parameters.Size + 1 + returnSlot));

                // Inject function parameters into the scope
                for (size_t i = 0; i < mdecl->Parameters.Size; i++) {
                    EmitNodeParamDecl(GetNode<NodeStmt>(mdecl->Parameters.Items[i]));
                
                    // Copy the arguments into the parameters
                    m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(-1, -(mdecl->Parameters.Size + 2 + returnSlot)));
                }
                
                EmitNodeCompound(mdecl->Body);
                
                if (m_OpCodes.back().Type != OpCodeType::Ret) {
                    m_OpCodes.emplace_back(OpCodeType::Ret);
                }
                
                PopCompilerStackFrame();
            }
        }
    }

    void Emitter::EmitNodeWhile(NodeStmt* stmt) {
        StmtWhile* wh = GetNode<StmtWhile>(stmt);

        int32_t loop = m_LabelCount;
        m_LabelCount++;
        int32_t loopEnd = m_LabelCount;
        m_LabelCount++;
        m_OpCodes.emplace_back(OpCodeType::Jmp, loop, "while loop condition");
        m_OpCodes.emplace_back(OpCodeType::Label, loop, "while loop condition");

        PushStackFrame();

        CompileStackSlot slot = EmitNodeExpression(wh->Condition);
        m_OpCodes.emplace_back(OpCodeType::Jf, OpCodeJump(CompileToRuntimeStackSlot(slot), loopEnd), "while loop end");

        EmitNodeCompound(wh->Body);

        PopStackFrame();

        m_OpCodes.emplace_back(OpCodeType::Jmp, loop, "while loop condition");
        m_OpCodes.emplace_back(OpCodeType::Label, loopEnd, "while loop end");
    }

    void Emitter::EmitNodeDoWhile(NodeStmt* stmt) {
        BLUA_ASSERT(false, "TODO");
    }

    void Emitter::EmitNodeIf(NodeStmt* stmt) {
        StmtIf* nif = GetNode<StmtIf>(stmt);

        PushStackFrame();

        CompileStackSlot slot = EmitNodeExpression(nif->Condition);
        int32_t ifLabel = m_LabelCount;
        int32_t elseLabel = m_LabelCount + 1;
        int32_t afterIfLabel = (nif->ElseBody) ? m_LabelCount + 2 : m_LabelCount + 1;
        m_LabelCount += 2;

        m_OpCodes.emplace_back(OpCodeType::Jt, OpCodeJump(CompileToRuntimeStackSlot(slot), ifLabel));

        if (nif->ElseBody) {
            m_OpCodes.emplace_back(OpCodeType::Jf, OpCodeJump(CompileToRuntimeStackSlot(slot), elseLabel));
            m_LabelCount += 1;
        }

        m_OpCodes.emplace_back(OpCodeType::Jmp, afterIfLabel);

        m_OpCodes.emplace_back(OpCodeType::Label, ifLabel, "if");

        EmitNodeCompound(nif->Body);
        m_OpCodes.emplace_back(OpCodeType::Jmp, afterIfLabel);

        if (nif->ElseBody) {
            m_OpCodes.emplace_back(OpCodeType::Label, elseLabel, "else");

            EmitNodeCompound(nif->ElseBody);

            m_OpCodes.emplace_back(OpCodeType::Jmp, afterIfLabel);
        }

        m_OpCodes.emplace_back(OpCodeType::Label, afterIfLabel, "after if");
        PopStackFrame();
    }

    void Emitter::EmitNodeReturn(NodeStmt* stmt) {
        StmtReturn* ret = GetNode<StmtReturn>(stmt);

        CompileStackSlot slot = EmitNodeExpression(ret->Value);

        m_OpCodes.emplace_back(OpCodeType::Copy, OpCodeCopy(CompileToRuntimeStackSlot(CompileStackSlot(-(m_CurrentStackFrame->SlotCount + 1))), CompileToRuntimeStackSlot(slot)), "return");
        m_OpCodes.emplace_back(OpCodeType::Ret);
    }

    void Emitter::EmitNodeStatement(NodeStmt* stmt) {
        if (GetNode<StmtCompound>(stmt)) {
            PushStackFrame();
            EmitNodeCompound(stmt);
            PopStackFrame();
        } else if (GetNode<StmtVarDecl>(stmt)) {
            EmitNodeVarDecl(stmt);
        } else if (GetNode<StmtFunctionDecl>(stmt)) {
            EmitNodeFunctionDecl(stmt);
        } else if (GetNode<StmtStructDecl>(stmt)) {
            EmitNodeStructDecl(stmt);
        } else if (GetNode<StmtWhile>(stmt)) {
            EmitNodeWhile(stmt);
        } else if (GetNode<StmtDoWhile>(stmt)) {
            EmitNodeDoWhile(stmt);
        } else if (GetNode<StmtIf>(stmt)) {
            EmitNodeIf(stmt);
        } else if (GetNode<StmtReturn>(stmt)) {
            EmitNodeReturn(stmt);
        }
    }

    void Emitter::EmitNode(Node* node) {
        if (NodeExpr* expr = GetNode<NodeExpr>(node)) {
            EmitNodeExpression(expr);
        } else if (NodeStmt* stmt = GetNode<NodeStmt>(node)) {
            EmitNodeStatement(stmt);
        }
    }

} // namespace BlackLua::Internal
