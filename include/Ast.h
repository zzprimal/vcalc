#ifndef _AST_H
#define _AST_H
#include "Symbol.h"
#include "Scope.h"
#include "antlr4-runtime.h" 
#include <memory>
#include <vector>
namespace Ast{

class AstNode{
    private:
        // symbol ast node references (can be nullptr), expr will reference a type
        std::shared_ptr<Symbol::BaseSymbol> reference;

        // list of all children of the current ast node (amount differs depending on current node)
        std::vector<std::shared_ptr<AstNode>> children; 

        // pointer to corrosponding antr token for ast node
        std::shared_ptr<antlr4::Token> token;

        // current scope (only really needs to be non-null for ast nodes that imply a scope change which is only BLOCK)
        std::shared_ptr<Scope::BaseScope> scope;

    public:
        // constructor using enums tokens in VCalcParser.h
        AstNode(size_t token_type);

        // constructor using just a token pointer (in visitor use for example ctx->ID()->getSymbol() for initialization)
        AstNode(antlr4::Token* init_token);

        // returns children member
        std::vector<std::shared_ptr<AstNode>> GetChildren(); 

        // takes return value from corrosponding AstBuilder visit methods and any casts it to call other AddChild method
        void AddChild(std::any t);

        // adds child into children member
        void AddChild(std::shared_ptr<AstNode> t); 

        // returns node type using token member
        size_t GetNodeType();

        // get text associated with token using token member, if none return "nil" string
        std::string GetText(); 

        // returns reference member
        std::shared_ptr<Symbol::BaseSymbol> GetReference();

        // sets reference member
        void SetReference(std::shared_ptr<Symbol::BaseSymbol> new_reference);

        // sets scope member
        void SetScope(std::shared_ptr<Scope::BaseScope> new_scope);

        std::shared_ptr<Scope::BaseScope> GetScope();
        // returns parse token of node
        std::shared_ptr<antlr4::Token> GetToken();

        // returns parse line
        size_t GetLine();



};


}
#endif
