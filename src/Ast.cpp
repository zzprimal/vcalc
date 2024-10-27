#include "Ast.h"
namespace Ast{

AstNode::AstNode(size_t token_type) : token(std::make_shared<antlr4::CommonToken>(token_type)) {}

AstNode::AstNode(antlr4::Token* init_token) : token(std::make_shared<antlr4::CommonToken>(init_token)) {}

std::vector<std::shared_ptr<AstNode>> AstNode::GetChildren(){
    return children;
}

void AstNode::AddChild(std::any child){
    AddChild(std::any_cast<std::shared_ptr<AstNode>>(child));
}

void AstNode::AddChild(std::shared_ptr<AstNode> child){
    children.push_back(child);
}

size_t AstNode::GetNodeType(){
    return token->getType();
}

std::string AstNode::GetText(){
    return token->getText();
}

std::shared_ptr<Symbol::BaseSymbol> AstNode::GetReference(){
    return reference;
}

void AstNode::SetReference(std::shared_ptr<Symbol::BaseSymbol> new_reference){
    reference = new_reference;
}

void AstNode::SetScope(std::shared_ptr<Scope::BaseScope> new_scope){
    scope = new_scope;
}
std::shared_ptr<Scope::BaseScope> AstNode::GetScope(){
    return scope;
}
std::shared_ptr<antlr4::Token> AstNode::GetToken() {
    return token;
}

size_t AstNode::GetLine() {
    return token->getLine();
}

}
