#include "AstBuilder.h"

namespace AstBuilder{

std::any AstBuild::visitFile(vcalc::VCalcParser::FileContext *ctx){
    return visit(ctx->block());
}

std::any AstBuild::AddOperationNode(vcalc::VCalcParser::ExprContext *ctx){
    std::shared_ptr<Ast::AstNode> operation_node;
    if (ctx->ADD() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->ADD()->getSymbol());
    }
    else if (ctx->SUB() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->SUB()->getSymbol());
    }
    else if (ctx->MUL() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->MUL()->getSymbol());
    }
    else if (ctx->DIV() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->DIV()->getSymbol());
    }
    else if (ctx->LESS() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->LESS()->getSymbol());
    }
    else if (ctx->GREATER() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->GREATER()->getSymbol());
    }
    else if (ctx->LOGEQ() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->LOGEQ()->getSymbol());
    }
    else if (ctx->LOGNEQ() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->LOGNEQ()->getSymbol());
    }
    else if (ctx->DOTS() != nullptr){
        operation_node = std::make_shared<Ast::AstNode>(ctx->DOTS()->getSymbol());
    }
    else{ // an index operation
        operation_node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::INDEX);
    }
    return operation_node;
}

std::any AstBuild::visitExpr(vcalc::VCalcParser::ExprContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::EXPR);
    std::shared_ptr<Ast::AstNode> token_node;
    std::shared_ptr<Ast::AstNode> id_node;
    if (ctx->ID() != nullptr){ // leaf expr node with child ID token
        token_node = std::make_shared<Ast::AstNode>(ctx->ID()->getSymbol());
        node->AddChild(token_node);
    }
    else if (ctx->INT() != nullptr){ // leaf expr node with child INT token
        token_node = std::make_shared<Ast::AstNode>(ctx->INT()->getSymbol());
        node->AddChild(token_node);
    }
    else if (ctx->generator() != nullptr){ // just a single generator child
        node->AddChild(visit(ctx->generator()->expr(0)));
        token_node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::GENERATOR);
        id_node = std::make_shared<Ast::AstNode>(ctx->generator()->ID()->getSymbol());
        token_node->AddChild(id_node);
        node->AddChild(token_node);
        node->AddChild(visit(ctx->generator()->expr(1)));
    }
    else if (ctx->filter() != nullptr){ // just a single filter child
        node->AddChild(visit(ctx->filter()->expr(0)));
        token_node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::FILTER);
        id_node = std::make_shared<Ast::AstNode>(ctx->filter()->ID()->getSymbol());
        token_node->AddChild(id_node);
        node->AddChild(token_node);
        node->AddChild(visit(ctx->filter()->expr(1)));
    }
    else if (ctx->expr(1) == nullptr){ // parenthesis expr node
        return visit(ctx->expr(0));
    }
    else{ // other cases with 2 expr nodes and middle token
        node->AddChild(visit(ctx->expr(0)));
        node->AddChild(AddOperationNode(ctx)); // have to check each of the possible operation tokens manually sadly 
        node->AddChild(visit(ctx->expr(1)));
    }
    return node;
}

std::any AstBuild::visitStatement(vcalc::VCalcParser::StatementContext *ctx){
    return visit(ctx->children[0]);
}

std::any AstBuild::visitBlock(vcalc::VCalcParser::BlockContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::BLOCK);
    for (auto *child : ctx->children){
        node->AddChild(visit(child));
    }
    return node;
}
std::any AstBuild::AddAssignNode(vcalc::VCalcParser::DeclarationContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::ASSIGN);
    std::shared_ptr<Ast::AstNode> id_node = std::make_shared<Ast::AstNode>(ctx->ID()->getSymbol());
    node->AddChild(id_node);
    node->AddChild(visit(ctx->expr()));
    return node;
}

std::any AstBuild::visitDeclaration(vcalc::VCalcParser::DeclarationContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::DECL);
    std::shared_ptr<Ast::AstNode> type_node = std::make_shared<Ast::AstNode>(ctx->TYPE()->getSymbol());
    std::shared_ptr<Ast::AstNode> id_node = std::make_shared<Ast::AstNode>(ctx->ID()->getSymbol());
    node->AddChild(type_node);
    node->AddChild(id_node);
    if (ctx->expr() != nullptr){
        node->AddChild(AddAssignNode(ctx));
    }
    return node;
}

std::any AstBuild::visitAssignment(vcalc::VCalcParser::AssignmentContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::ASSIGN);
    std::shared_ptr<Ast::AstNode> id_node = std::make_shared<Ast::AstNode>(ctx->ID()->getSymbol());
    node->AddChild(id_node);
    node->AddChild(visit(ctx->expr()));
    return node;
}

std::any AstBuild::visitIf_stat(vcalc::VCalcParser::If_statContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::IF_BLOCK);
    node->AddChild(visit(ctx->expr()));
    node->AddChild(visit(ctx->block()));
    return node;
}

std::any AstBuild::visitLoop(vcalc::VCalcParser::LoopContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::LOOP_BLOCK);
    node->AddChild(visit(ctx->expr()));
    node->AddChild(visit(ctx->block()));
    return node;
}

std::any AstBuild::visitPrint(vcalc::VCalcParser::PrintContext *ctx){
    std::shared_ptr<Ast::AstNode> node = std::make_shared<Ast::AstNode>(vcalc::VCalcParser::PRINT);
    node->AddChild(visit(ctx->expr()));
    return node;
}

}