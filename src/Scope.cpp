#include "Scope.h"

namespace Scope{

void BaseScope::Define(std::shared_ptr<Symbol::BaseSymbol> sym){
    symbols.emplace(sym->GetName(), sym);
    sym->SetScope(shared_from_this());
}

std::shared_ptr<BaseScope> BaseScope::GetEnclosingScope(){
    return enclosing_scope;
}

std::map<std::string, std::shared_ptr<Symbol::BaseSymbol>> BaseScope::GetSymbols(){
    return symbols;
}

void BaseScope::SetEnclosingScope(std::shared_ptr<BaseScope> new_enclosing_scope){
    enclosing_scope = new_enclosing_scope;
}

std::shared_ptr<Symbol::BaseSymbol> BaseScope::Resolve(const std::string &name){
    auto iterator = symbols.find(name);
    if (iterator != symbols.end()){
        return iterator->second;
    }
    else if (enclosing_scope != nullptr){
        return enclosing_scope->Resolve(name);
    }
    return nullptr;
}

void BaseScope::PrintScope(){
    std::cout << '[';
    for (auto iterator = symbols.begin(); iterator != symbols.end(); iterator++){
        std::cout << iterator->first;
        std::cout << ' ';
    }
    std::cout << ']' << std::endl;
}

void BaseScope::PrintAllScopes(){
    std::shared_ptr<BaseScope> current_scope = shared_from_this();
    do{
        current_scope->PrintScope();
        current_scope = current_scope->GetEnclosingScope();
    } while (current_scope != nullptr);
}
    LocalScope::LocalScope(const std::shared_ptr<BaseScope> &initScope) : BaseScope(initScope) {}

    LocalScope::~LocalScope() = default;

    GlobalScope::GlobalScope(const std::shared_ptr<BaseScope> &initScope) : BaseScope(initScope) {}

    GlobalScope::~GlobalScope() = default;
}