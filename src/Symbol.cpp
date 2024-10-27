#include "Symbol.h"

namespace Symbol{

std::string BaseSymbol::GetName(){
    return name;
}

void BaseSymbol::SetName(std::string new_name){
    name = new_name;
}

std::shared_ptr<Scope::BaseScope> BaseSymbol::GetScope(){
    return scope;
}


void BaseSymbol::SetScope(std::shared_ptr<Scope::BaseScope> new_scope){
    scope = new_scope;
}

std::shared_ptr<BuiltInTypeSymbol> VarSymbol::GetTypeSymbol(){
    return type_symbol;
}

mlir::Value VarSymbol::GetValue(){
    return value;
}

void VarSymbol::SetValue(mlir::Value new_value){
    value = new_value;
}

void VarSymbol::SetTypeSymbol(std::shared_ptr<BuiltInTypeSymbol> new_symbol){
    type_symbol = new_symbol;
}

}