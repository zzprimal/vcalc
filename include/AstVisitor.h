#ifndef _ASTVISITOR_H
#define _ASTVISITOR_H
#include "Ast.h"
#include "BackEnd.h"
#include "Scope.h"
#include "VCalcParser.h"
extern int program_flags;
#define DEBUG 1

namespace AstVisitor{

class AstWalker{
    protected:
        std::shared_ptr<Scope::BaseScope> current_scope;
    public:
        virtual void Visit(std::shared_ptr<Ast::AstNode> current_node);
        virtual void VisitChildren(std::shared_ptr<Ast::AstNode> current_node);
        virtual void VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitDECL(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitEXPR(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitPRINT(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitID(std::shared_ptr<Ast::AstNode> current_node) = 0;
        virtual void VisitINT(std::shared_ptr<Ast::AstNode> current_node) = 0;
};

class DefRef: public AstWalker{
    enum OperationType {
        Arithmetic,
        Bool,
        Scoped,
        Misc
    };
    public:
        DefRef();

    private:
        std::vector<std::tuple<std::unordered_set<size_t>, OperationType>> operation_to_type;
        void VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitDECL(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitEXPR(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitPRINT(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitID(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitINT(std::shared_ptr<Ast::AstNode> current_node) override;
        std::shared_ptr<Symbol::BuiltInTypeSymbol> GetBuiltInTypeData(const std::string& type);

};

class CodeGen: public AstWalker, public BackEnd{
    private:
        std::stack<mlir::Value> opperands;
        void VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitDECL(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitEXPR(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitPRINT(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitID(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitINT(std::shared_ptr<Ast::AstNode> current_node) override;
    public:
        void GenerateMlir(bool dump, std::shared_ptr<Ast::AstNode> current_node);
        CodeGen();

};

class AstDebugger: public AstWalker{
    public:
        void VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitDECL(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitEXPR(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitPRINT(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitID(std::shared_ptr<Ast::AstNode> current_node) override;
        void VisitINT(std::shared_ptr<Ast::AstNode> current_node) override;
        void ExprPrintOp(std::shared_ptr<Ast::AstNode> current_node);
        void DfsTraversal(std::shared_ptr<Ast::AstNode> current_node);
};

    
}
#endif