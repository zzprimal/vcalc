#ifndef _SYMBOL_H
#define _SYMBOL_H
#include "Type.h"
#include "BackEnd.h"
#include <string>
#include <memory>

// forward declaration to resolve circular reference with Scope.h
namespace Scope{
    class BaseScope;
}

namespace Symbol{


class BaseSymbol{
    private:
        // name of the symbol
        std::string name;


        // current scope the symbol is in
        std::shared_ptr<Scope::BaseScope> scope;
    public:
        BaseSymbol(std::string init_name, std::shared_ptr<Scope::BaseScope> init_scope): name(init_name), scope(init_scope) {}
        std::string GetName();
        void SetName(std::string new_name);
        std::shared_ptr<Scope::BaseScope> GetScope();
        void SetScope(std::shared_ptr<Scope::BaseScope> new_scope);
        virtual ~BaseSymbol() = default;
};


class BuiltInTypeSymbol: public BaseSymbol, public Type::DataType{
    public:
        BuiltInTypeSymbol(std::string init_name, std::shared_ptr<Scope::BaseScope> init_scope, Type::VCalcTypes init_type): BaseSymbol(init_name, init_scope), DataType(init_type) {}

};

class VarSymbol: public BaseSymbol{
    private:
        std::shared_ptr<BuiltInTypeSymbol> type_symbol;
        mlir::Value value;
    public:
        VarSymbol(std::string init_name, std::shared_ptr<Scope::BaseScope> init_scope, std::shared_ptr<BuiltInTypeSymbol> init_type) : BaseSymbol(init_name, init_scope), type_symbol(init_type) {}
        mlir::Value GetValue();
        void SetValue(mlir::Value new_value);
        std::shared_ptr<BuiltInTypeSymbol> GetTypeSymbol();
        void SetTypeSymbol(std::shared_ptr<BuiltInTypeSymbol> new_symbol);
};


}
#endif