#ifndef _ASTBUILDER_H
#define _ASTBUILDER_H
#include "VCalcBaseVisitor.h"
#include "Ast.h"

namespace AstBuilder{

class AstBuild: public vcalc::VCalcBaseVisitor{
    public:
        std::any visitFile(vcalc::VCalcParser::FileContext *ctx) override;
        std::any visitExpr(vcalc::VCalcParser::ExprContext *ctx) override;
        std::any visitStatement(vcalc::VCalcParser::StatementContext *ctx) override;
        std::any visitBlock(vcalc::VCalcParser::BlockContext *ctx) override;
        std::any visitDeclaration(vcalc::VCalcParser::DeclarationContext *ctx) override;
        std::any visitAssignment(vcalc::VCalcParser::AssignmentContext *ctx) override;
        std::any visitIf_stat(vcalc::VCalcParser::If_statContext *ctx) override;
        std::any visitLoop(vcalc::VCalcParser::LoopContext *ctx) override;
        std::any visitPrint(vcalc::VCalcParser::PrintContext *ctx) override;
        std::any AddOperationNode(vcalc::VCalcParser::ExprContext *ctx);
        std::any AddAssignNode(vcalc::VCalcParser::DeclarationContext *ctx);
};

    
}
#endif