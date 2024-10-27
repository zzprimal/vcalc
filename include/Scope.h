#ifndef _SCOPE_H
#define _SCOPE_H
#include "Symbol.h"
#include <string>
#include <map>
#include <memory>
#include <iostream>
namespace Scope{

class BaseScope: public std::enable_shared_from_this<BaseScope>{
    private:
        // member that represents all the symbols in the scope, is a map so requires you give as input the symbol
        // name then the corrosponding symbol object is returned (Resolve can be used for this)
        std::map<std::string, std::shared_ptr<Symbol::BaseSymbol>> symbols; // symbol name is gotten by getText method on corrosponding antr4 token

        // member thats represents the scope that encloses current scope
        std::shared_ptr<BaseScope> enclosing_scope;

    public:
        // constructor for a scope, we create a new scope when we encounter AST node that defines new scope
        // (in the case of VCalc this should only be the BLOCK node)
        BaseScope(std::shared_ptr<BaseScope> init_scope): enclosing_scope(init_scope) {}

        // adds a symbol to the scope
        virtual void Define(std::shared_ptr<Symbol::BaseSymbol> sym);

        // returns the enclosing_scope member
        virtual std::shared_ptr<BaseScope> GetEnclosingScope();

        std::map<std::string, std::shared_ptr<Symbol::BaseSymbol>> GetSymbols();

        // sets new enclosing scope
        virtual void SetEnclosingScope(std::shared_ptr<BaseScope> new_enclosing_scope);

        // resolve symbol with name in current scope and all enclosing scopes
        virtual std::shared_ptr<Symbol::BaseSymbol> Resolve(const std::string &name);

        // prints only the current scope and the names of all symbols in it, used for debugging if necessary
        virtual void PrintScope();

        // prints the current scope and all enclosing scopes symbol names, used for debugging if necessary
        virtual void PrintAllScopes();
};

class GlobalScope: public BaseScope {
public:
    explicit GlobalScope(const std::shared_ptr<BaseScope> &initScope);

    virtual ~GlobalScope();

};

class LocalScope: public BaseScope {
public:
    explicit LocalScope(const std::shared_ptr<BaseScope> &initScope);

    virtual ~LocalScope();

};

}
#endif