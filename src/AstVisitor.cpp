#include "AstVisitor.h"

namespace AstVisitor{

// AstVisitor Base methods
void AstWalker::Visit(std::shared_ptr<Ast::AstNode> current_node){
    switch (current_node->GetNodeType() ) {
        case vcalc::VCalcParser::BLOCK:
            VisitBLOCK(current_node);
            break;
        case vcalc::VCalcParser::IF_BLOCK:
            VisitIF_BLOCK(current_node);
            break;
        case vcalc::VCalcParser::LOOP_BLOCK:
            VisitLOOP_BLOCK(current_node);
            break;
        case vcalc::VCalcParser::DECL:
            VisitDECL(current_node);
            break;
        case vcalc::VCalcParser::ASSIGN:
            VisitASSIGN(current_node);
            break;
        case vcalc::VCalcParser::PRINT:
            VisitPRINT(current_node);
            break;
        case vcalc::VCalcParser::EXPR:
            VisitEXPR(current_node);
            break;
        case vcalc::VCalcParser::ID:
            VisitID(current_node);
            break;
        case vcalc::VCalcParser::INT:
            VisitINT(current_node);
            break;
        default: // The other nodes we don't care about just have their children visited
            VisitChildren(current_node);
    }
    return; // infinite recursion somewhere, idk where
}

void AstWalker::VisitChildren(std::shared_ptr<Ast::AstNode> current_node){
    for ( const auto& child : current_node->GetChildren() ){
        Visit(child);
    }
}

// DefRef Visitor methods
void DefRef::VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node) {
    if (program_flags & DEBUG){
        std::cout << "AT BLOCK\n";
    }
    bool global_scope = !current_scope;
    if (global_scope) {
        current_scope = std::make_shared<Scope::GlobalScope>(nullptr);
        current_node->SetScope(current_scope);
        std::vector<std::shared_ptr<Symbol::BuiltInTypeSymbol>> builtInTypes = {
                std::make_shared<Symbol::BuiltInTypeSymbol>("int",current_scope,Type::VCalcTypes::INT),
                std::make_shared<Symbol::BuiltInTypeSymbol>("vector",current_scope,Type::VCalcTypes::VECTOR)
        };
        for (const std::shared_ptr<Symbol::BuiltInTypeSymbol>& builtInType : builtInTypes) {
            current_scope->Define(builtInType);
        }
        if (program_flags & DEBUG) {
            std::cout << "Initialized Built In Types: ";
            current_scope->PrintAllScopes();
        }
        VisitChildren(current_node);
        
        if (program_flags & DEBUG) {
            std::cout << "Global Scope: ";
            current_scope->PrintScope();
        }
        current_scope = current_scope->GetEnclosingScope();
        

    } else {
        current_scope = std::make_shared<Scope::LocalScope>(current_scope);
        current_node->SetScope(current_scope);
        VisitChildren(current_node);
        if (program_flags & DEBUG) {
            std::cout << "Created Block Scope: ";
            current_scope->PrintScope();
        }
        current_scope = current_scope->GetEnclosingScope();
    }
    if (program_flags & DEBUG){
        std::cout << "OUT BLOCK\n";
    }

}
void DefRef::VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT IF_BLOCK\n";
    }
    VisitChildren(current_node);
    auto expr_node = current_node->GetChildren()[0];
    auto expr_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(expr_node->GetReference());
    if (expr_type->GetType() != Type::INT) {
        throw std::runtime_error("Type mismatch at line " + std::to_string(current_node->GetLine()) + ": expression value must be int");
    }
}
void DefRef::VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT LOOP_BLOCK\n";
    }
    VisitChildren(current_node);
    auto expr_node = current_node->GetChildren()[0];
    auto expr_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(expr_node->GetReference());
    if (expr_type->GetType() != Type::INT) {
        throw std::runtime_error("Type mismatch at line " + std::to_string(current_node->GetLine()) + ": expression value must be int");
    }
}
void DefRef::VisitDECL(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT DECL\n";
    }
    auto type_node = current_node->GetChildren()[0];
    auto id_node = current_node->GetChildren()[1];
    std::cout << "here\n";

    auto symbol_val = current_scope->Resolve(type_node->GetText());
    if (!symbol_val) {
        throw std::runtime_error("Invalid declaration keyword " + std::to_string(current_node->GetLine()) + ": " + type_node->GetText());
    }
    auto type_symbol = std::dynamic_pointer_cast<Symbol::BuiltInTypeSymbol>(symbol_val);
    if (!type_symbol) {
        throw std::runtime_error("Invalid declaration keyword " + std::to_string(current_node->GetLine()) + ": " + type_node->GetText());
    }
    auto sym_map = current_scope->GetSymbols();
    if (sym_map.find(id_node->GetText()) != sym_map.end()) {
        throw std::runtime_error("Invalid declaration twice " + std::to_string(current_node->GetLine()) + ": " + type_node->GetText());
    }


    auto var_symbol = std::make_shared<Symbol::VarSymbol>(id_node->GetText(), current_scope, type_symbol);
    current_scope->Define(var_symbol);
    current_node->SetScope((current_scope));
    current_node->SetReference(var_symbol);

    bool assign = current_node->GetChildren().size() > 2;
    if (assign) {
        auto assign_node = current_node->GetChildren()[2];
        VisitASSIGN(assign_node);
    }
    if (program_flags & DEBUG){
        std::cout << "OUT DECL\n";
    }

}
void DefRef::VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT ASSIGN\n";
    }
    std::shared_ptr<Ast::AstNode> id_node = current_node->GetChildren()[0];
    std::shared_ptr<Ast::AstNode> expression_node = current_node->GetChildren()[1];

    VisitEXPR(expression_node);
    auto expr_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(expression_node->GetReference());

    auto symbol = current_scope->Resolve(id_node->GetText());
    if (!symbol) {
        throw std::runtime_error("Tried to access undefined variable at line " + std::to_string(current_node->GetLine()));
    }

    auto var_symbol = std::static_pointer_cast<Symbol::VarSymbol>(symbol);
    if (expr_type->GetType() != var_symbol->GetTypeSymbol()->GetType()) {
        throw std::runtime_error("Type mismatch at line " + std::to_string(current_node->GetLine()) + ": " + var_symbol->GetName() + " has different type than expression");
    }
}
void DefRef::VisitEXPR(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT EXPR\n";
    }
    current_node->SetScope(current_scope);
    if (current_node->GetChildren().size() == 1) { // Parentheses, INT & ID cases
        auto child = current_node->GetChildren()[0];
        switch (child->GetNodeType()) {
            case vcalc::VCalcParser::ID: { // Block is required to prevent jump bypass switch statement???
                auto var_symbol = std::static_pointer_cast<Symbol::VarSymbol>(current_scope->Resolve(child->GetText()));
                if (!var_symbol) {
                    throw std::runtime_error("Tried to access undefined variable at line " + std::to_string(child->GetLine()) + ": " + child->GetText());
                }
                current_node->SetReference(var_symbol->GetTypeSymbol());
                return;
            }
            case vcalc::VCalcParser::INT:
                current_node->SetReference(GetBuiltInTypeData("int"));
                return;
            case vcalc::VCalcParser::EXPR:
                VisitEXPR(child);
                return;
            default:
                std::cerr << "Did not cover case for: " << current_node->GetNodeType() << std::endl;
        }
    }

    size_t operation = current_node->GetChildren()[1]->GetNodeType();
    OperationType operation_group = Misc;
    for (const auto& tuple : operation_to_type) {
        const auto& set = std::get<0>(tuple);
        auto it = set.find(operation);
        if (it != set.end()) {
            OperationType type = std::get<1>(tuple);
            operation_group = type;
            break;
        }
    }

    if (operation_group == Scoped) {
        std::shared_ptr<Ast::AstNode> iterator_expression_node = current_node->GetChildren()[0];
        std::shared_ptr<Ast::AstNode> id_node = current_node->GetChildren()[1]->GetChildren()[0];
        std::shared_ptr<Ast::AstNode> eval_expression_node = current_node->GetChildren()[2];

        VisitEXPR(iterator_expression_node);
        auto built_in_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(iterator_expression_node->GetReference());
        if (built_in_type->GetType() != Type::VECTOR) {
            throw std::runtime_error("Type mismatch at line " + std::to_string(current_node->GetLine()) + ": iterator is not vector");
        }
        current_scope = std::make_shared<Scope::LocalScope>(current_scope);

        // Define new int variable
        auto int_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(current_scope->Resolve("int"));
        std::string var_name = id_node->GetText();
        std::shared_ptr<Symbol::VarSymbol> var_symbol = std::make_shared<Symbol::VarSymbol>(var_name, current_scope, int_type);
        current_scope->Define(var_symbol);
        id_node->SetReference(var_symbol);
        id_node->SetScope(current_scope);

        // Visit evaluation expression
        VisitEXPR(eval_expression_node);

        if (program_flags & DEBUG) {
            std::cout << "Created Expression Scope: ";
            current_scope->PrintScope();
        }
        // Pop scope
        current_scope = current_scope->GetEnclosingScope();

        // Generator and Filter return vector types
        current_node->SetReference(GetBuiltInTypeData("vector"));
        return;
    }
    size_t op_type = current_node->GetChildren()[1]->GetNodeType();
    std::shared_ptr<Ast::AstNode> left = current_node->GetChildren()[0];
    std::shared_ptr<Ast::AstNode> right = current_node->GetChildren()[2];

    VisitEXPR(left);
    VisitEXPR(right);
    auto left_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(left->GetReference());
    auto right_type = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(right->GetReference());

    size_t left_type_val = left_type->GetType();
    size_t right_type_val = right_type->GetType();

    switch (operation_group) {
        case Arithmetic:
            if (left_type_val == right_type_val) {
                current_node->SetReference(left_type);
                return;
            }
            // Promote int to vector. Only two types so we can hard code this.
            current_node->SetReference(GetBuiltInTypeData("vector"));
            return;
        case Bool:
            if (left_type_val == Type::VCalcTypes::VECTOR || right_type_val == Type::VCalcTypes::VECTOR) { 
                current_node->SetReference(GetBuiltInTypeData("vector"));
            }
            else{
                current_node->SetReference(GetBuiltInTypeData("int"));
            }
            return;
        default:
            break;
    }
    switch (op_type) {
        case vcalc::VCalcParser::DOTS: // RANGE
            if (left_type_val != Type::VCalcTypes::INT || right_type_val != Type::VCalcTypes::INT) {
                throw std::runtime_error("Type mismatch at line " + std::to_string(current_node->GetLine()) + ": range values must be int");
            }
            current_node->SetReference(GetBuiltInTypeData("vector"));
            return;
        case vcalc::VCalcParser::INDEX:
            if (left_type_val != Type::VCalcTypes::VECTOR) {
                throw std::runtime_error("Type mismatch at line " + std::to_string(current_node->GetLine()) + ": the value being indexed must be a vector");
            }
            if (right_type_val == Type::VCalcTypes::INT) {
                current_node->SetReference(GetBuiltInTypeData("int"));
            }
            else{
                current_node->SetReference(GetBuiltInTypeData("vector"));
            }
            return;
        default:
            break;
    }
    std::cerr << "Did not cover case operation for: " << op_type << std::endl;
}
void DefRef::VisitPRINT(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT PRINT\n";
    }
    VisitChildren(current_node);
}
void DefRef::VisitID(std::shared_ptr<Ast::AstNode> current_node){

}
void DefRef::VisitINT(std::shared_ptr<Ast::AstNode> current_node){

}

DefRef::DefRef() {
    std::unordered_set<size_t> arith_ops = {
        vcalc::VCalcParser::ADD,
        vcalc::VCalcParser::SUB,
        vcalc::VCalcParser::DIV,
        vcalc::VCalcParser::MUL
    };
    std::unordered_set<size_t> bool_ops = {
        vcalc::VCalcParser::LESS,
        vcalc::VCalcParser::GREATER,
        vcalc::VCalcParser::LOGEQ,
        vcalc::VCalcParser::LOGNEQ
    };
    std::unordered_set<size_t> scoped_operations = {
        vcalc::VCalcParser::GENERATOR,
        vcalc::VCalcParser::FILTER
    };
    operation_to_type = {
        std::make_tuple(arith_ops, OperationType::Arithmetic),
        std::make_tuple(bool_ops, OperationType::Bool),
        std::make_tuple(scoped_operations, OperationType::Scoped)
    };
}

    std::shared_ptr<Symbol::BuiltInTypeSymbol> DefRef::GetBuiltInTypeData(const std::string &type) {
        return std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(current_scope->Resolve(type));
    }


// CodeGen Visitor methods

CodeGen::CodeGen(): AstWalker(), BackEnd(){}

void CodeGen::GenerateMlir(bool dump, std::shared_ptr<Ast::AstNode> current_node){
    emitModule();
    Visit(current_node);
    builder->create<mlir::LLVM::ReturnOp>(builder->getUnknownLoc(), const_zero);
    if (dump){
        module.dump();
    }
    if (mlir::failed(mlir::verify(module))) {
        module.emitError("module failed to verify");
        exit(-1);
    }
}

void CodeGen::VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT BLOCK\n";
    }
    current_scope = current_node->GetScope();
    VisitChildren(current_node);
    current_scope = current_scope->GetEnclosingScope();
    if (program_flags & DEBUG){
        std::cout << "OUT BLOCK\n";
    }
}
void CodeGen::VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT IF_BLOCK\n";
    }
    Visit(current_node->GetChildren()[0]); // expr node
    mlir::Value result = opperands.top();
    opperands.pop();

    mlir::Block *if_block = main_func.addBlock();
    mlir::Block *merge = main_func.addBlock();

    mlir::Value condition = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, result, const_zero);
    builder->create<mlir::LLVM::CondBrOp>(loc, condition, if_block, merge);
    builder->setInsertionPointToStart(if_block);
    Visit(current_node->GetChildren()[1]); // visit block child
    builder->create<mlir::LLVM::BrOp>(loc, merge);
    builder->setInsertionPointToStart(merge);

    if (program_flags & DEBUG){
        std::cout << "OUT IF_BLOCK\n";
    }

}
void CodeGen::VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT LOOP_BLOCK\n";
    }
    mlir::Block *header = main_func.addBlock();
    mlir::Block *body = main_func.addBlock();
    mlir::Block *merge = main_func.addBlock();
    builder->create<mlir::LLVM::BrOp>(loc, header);
    
    builder->setInsertionPointToStart(header);
    Visit(current_node->GetChildren()[0]); // expr node
    mlir::Value result = opperands.top();
    opperands.pop();
    mlir::Value condition = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, result, const_zero);
    builder->create<mlir::LLVM::CondBrOp>(loc, condition, body, merge);
    builder->setInsertionPointToStart(body);
    Visit(current_node->GetChildren()[1]);
    builder->create<mlir::LLVM::BrOp>(loc, header);
    builder->setInsertionPointToStart(merge);
    if (program_flags & DEBUG){
        std::cout << "OUT LOOP_BLOCK\n";
    }
}
void CodeGen::VisitDECL(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT DECL\n";
    }
    auto var_symbol = std::static_pointer_cast<Symbol::VarSymbol>(current_node->GetReference());
    if (var_symbol->GetTypeSymbol()->IsType(Type::INT)){
        var_symbol->SetValue(builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int_type, const_one));

    }
    else if (var_symbol->GetTypeSymbol()->IsType(Type::VECTOR)){
        var_symbol->SetValue(builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, ptr_type, const_one));
    }
    else{
        std::cout << "ERROR IN DECL\n";
        throw -1;
    }
    if (current_node->GetChildren().size() == 3){
        Visit(current_node->GetChildren()[2]);
    }
    if (program_flags & DEBUG){
        std::cout << "OUT DECL\n";
    }
}
void CodeGen::VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT ASSIGN\n";
    }
    Visit(current_node->GetChildren()[1]);
    mlir::Value result = opperands.top(); // generator not pushing
    opperands.pop();
    // need to free here
    auto variable = std::static_pointer_cast<Symbol::VarSymbol>(current_scope->Resolve(current_node->GetChildren()[0]->GetText()));
    builder->create<mlir::LLVM::StoreOp>(loc, result, variable->GetValue());
    if (program_flags & DEBUG){
        std::cout << "OUT ASSIGN\n";
    }
}
void CodeGen::VisitEXPR(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT EXPR\n";
    }
    if (current_node->GetChildren().size() == 1) { // (expr), ID & INT
        VisitChildren(current_node);
        if (program_flags & DEBUG){
            std::cout << "OUT EXPR\n";
        }
        return;
    }

    std::shared_ptr<Ast::AstNode> right = current_node->GetChildren()[2];
    std::shared_ptr<Ast::AstNode> left = current_node->GetChildren()[0];
    mlir::ValueRange args; 
    mlir::Value result;
    size_t op_type = current_node->GetChildren()[1]->GetNodeType();
    auto r_opperand_sym = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(right->GetReference());
    auto l_opperand_sym = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(left->GetReference());
    mlir::LLVM::LLVMFuncOp op_func;
    mlir::Value r_opperand;
    mlir::Value l_opperand;

    if (op_type == vcalc::VCalcParser::GENERATOR || op_type == vcalc::VCalcParser::FILTER){ // left child vector, right child int
        Visit(left);
        mlir::Value gen_filter_vector = opperands.top();
        opperands.pop();
        current_scope = current_node->GetChildren()[1]->GetChildren()[0]->GetScope();
        auto iterator_sym = std::static_pointer_cast<Symbol::VarSymbol>(current_node->GetChildren()[1]->GetChildren()[0]->GetReference());
        iterator_sym->SetValue(builder->create<mlir::LLVM::AllocaOp>(loc, ptr_type, int_type, const_one));
        mlir::Value gen_filter_index = iterator_sym->GetValue();

        mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(loc, ptr_type, int_type, gen_filter_vector, mlir::ValueRange{const_zero});
        mlir::Value size = builder->create<mlir::LLVM::LoadOp>(loc, int_type, size_addr);

        result = GenerateVectorTypePtr(size);
        mlir::Value result_size_ptr;

        if (op_type == vcalc::VCalcParser::FILTER){ 
            result_size_ptr = builder->create<mlir::LLVM::GEPOp>(
                loc,
                ptr_type,
                int_type,
                result,
                mlir::ValueRange{const_zero}
            );
            builder->create<mlir::LLVM::StoreOp>(loc, const_zero, result_size_ptr);
        }
        mlir::Value result_arr_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            result,
            mlir::ValueRange{const_one}
        );

        mlir::scf::ForOp for_loop = builder->create<mlir::scf::ForOp>(loc, const_zero, size, const_one);
        mlir::Value loop_index = for_loop.getInductionVar();
        mlir::Block *for_loop_body = for_loop.getBody();
        mlir::OpBuilder::InsertPoint save = builder->saveInsertionPoint();
        builder->setInsertionPointToStart(for_loop_body);
        // set iterator
        op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_index");
        args = {gen_filter_vector, loop_index};
        mlir::Value gen_filter_vector_elem = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
        builder->create<mlir::LLVM::StoreOp>(loc, gen_filter_vector_elem, gen_filter_index);
    
        Visit(right);
        // set result index
        mlir::Value gen_filter_expr_result = opperands.top();
        opperands.pop();

        mlir::Value arr_index_ptr;
        if (op_type == vcalc::VCalcParser::FILTER){ // filter use cmp op
            mlir::Value condition = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::ne, gen_filter_expr_result, const_zero);
            mlir::scf::IfOp if_statement = builder->create<mlir::scf::IfOp>(loc, condition);
            mlir::Block *if_body = if_statement.getBody();
            mlir::OpBuilder::InsertPoint save = builder->saveInsertionPoint();
            builder->setInsertionPointToStart(if_body);
            mlir::Value result_size = builder->create<mlir::LLVM::LoadOp>(loc, int_type, result_size_ptr);
            arr_index_ptr = builder->create<mlir::LLVM::GEPOp>(
                loc,
                ptr_type,
                int_type,
                result_arr_ptr,
                mlir::ValueRange{result_size}
            );

            builder->create<mlir::LLVM::StoreOp>(loc, gen_filter_vector_elem, arr_index_ptr);
            mlir::Value new_size = builder->create<mlir::LLVM::AddOp>(loc, result_size, const_one);
            builder->create<mlir::LLVM::StoreOp>(loc, new_size, result_size_ptr);

            builder->restoreInsertionPoint(save);
        }
        else{
            arr_index_ptr = builder->create<mlir::LLVM::GEPOp>(
                loc,
                ptr_type,
                int_type,
                result_arr_ptr,
                mlir::ValueRange{loop_index}
            );
            builder->create<mlir::LLVM::StoreOp>(loc, gen_filter_expr_result, arr_index_ptr);
        }
        builder->restoreInsertionPoint(save);

        opperands.push(result);
        current_scope = current_scope->GetEnclosingScope();
        if (program_flags & DEBUG){
            std::cout << "OUT EXPR\n";
        }
        return;
    }

    VisitChildren(current_node);
    r_opperand = opperands.top();
    opperands.pop();
    l_opperand = opperands.top();
    opperands.pop();

    if (op_type == vcalc::VCalcParser::INDEX){ // left child vector, right child int or vector
        if (r_opperand_sym->IsType(Type::INT)){
            op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_index");
        }
        
        else if (r_opperand_sym->IsType(Type::VECTOR)){
            op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_index_vector");
        }
        
        args = {l_opperand, r_opperand};
        result = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
        
    }
    else if (op_type == vcalc::VCalcParser::DOTS){ // both children ints
        op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_range");
        args = {l_opperand, r_opperand};
        result = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
    }
    else if (l_opperand_sym->IsType(Type::INT) && r_opperand_sym->IsType(Type::INT)){
        switch (op_type) {
            case vcalc::VCalcParser::ADD:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_add");
                break;
            case vcalc::VCalcParser::SUB:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_sub"); 
                break;
            case vcalc::VCalcParser::MUL:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_mul");
                break;
            case vcalc::VCalcParser::DIV:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_div");
                break;
            case vcalc::VCalcParser::LESS:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_less_than");
                break;
            case vcalc::VCalcParser::GREATER:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_greater_than");
                break;
            case vcalc::VCalcParser::LOGEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_equal"); 
                break;
            case vcalc::VCalcParser::LOGNEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_nequal"); 
                break;
            default:
                break;
        }
        args = {l_opperand, r_opperand};
        result = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
    }
    else if (l_opperand_sym->IsType(Type::VECTOR) && r_opperand_sym->IsType(Type::INT)){
        mlir::LLVM::LLVMFuncOp promotion_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_to_vector");
        mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(loc, ptr_type, int_type, l_opperand, mlir::ValueRange{const_zero});
        mlir::Value size = builder->create<mlir::LLVM::LoadOp>(loc, int_type, size_addr);
        args = {size, r_opperand};
        mlir::Value promoted_int = builder->create<mlir::LLVM::CallOp>(loc, promotion_func, args).getResult();
        switch (op_type) {
            case vcalc::VCalcParser::ADD:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_add");
                break;
            case vcalc::VCalcParser::SUB:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_sub");
                break;
            case vcalc::VCalcParser::MUL:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_mul");
                break;
            case vcalc::VCalcParser::DIV:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_div");
                break;
            case vcalc::VCalcParser::LESS:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_less_than");
                break;
            case vcalc::VCalcParser::GREATER:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_greater_than");
                break;
            case vcalc::VCalcParser::LOGEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_equal"); 
                break;
            case vcalc::VCalcParser::LOGNEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_nequal"); 
                break;
            default:
                break;
        }
        args = {l_opperand, promoted_int};
        result = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
        
    }
    else if (l_opperand_sym->IsType(Type::INT) && r_opperand_sym->IsType(Type::VECTOR)){
        mlir::LLVM::LLVMFuncOp promotion_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("int_to_vector");
        mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, int_type, r_opperand, mlir::ValueRange{const_zero});
        mlir::Value size = builder->create<mlir::LLVM::LoadOp>(loc, int_type, size_addr);
        args = {size, l_opperand};
        mlir::Value promoted_int = builder->create<mlir::LLVM::CallOp>(loc, promotion_func, args).getResult();
        switch (op_type) {
            case vcalc::VCalcParser::ADD:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_add");
                break;
            case vcalc::VCalcParser::SUB:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_sub"); 
                break;
            case vcalc::VCalcParser::MUL:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_mul");
                break;
            case vcalc::VCalcParser::DIV:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_div");
                break;
            case vcalc::VCalcParser::LESS:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_less_than");
                break;
            case vcalc::VCalcParser::GREATER:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_greater_than");
                break;
            case vcalc::VCalcParser::LOGEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_equal"); 
                break;
            case vcalc::VCalcParser::LOGNEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_nequal"); 
                break;
            default:
                break;
        }
        args = {promoted_int, r_opperand};
        result = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
    }
    else if (l_opperand_sym->IsType(Type::VECTOR) && r_opperand_sym->IsType(Type::VECTOR)){
        switch (op_type) {
            case vcalc::VCalcParser::ADD:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_add");
                break;
            case vcalc::VCalcParser::SUB:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_sub"); 
                break;
            case vcalc::VCalcParser::MUL:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_mul");
                break;
            case vcalc::VCalcParser::DIV:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_div");
                break;
            case vcalc::VCalcParser::LESS:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_less_than");
                break;
            case vcalc::VCalcParser::GREATER:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_greater_than");
                break;
            case vcalc::VCalcParser::LOGEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_equal"); 
                break;
            case vcalc::VCalcParser::LOGNEQ:
                op_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_nequal"); 
                break;
            default:
                break;
        }
        args = {l_opperand, r_opperand};
        result = builder->create<mlir::LLVM::CallOp>(loc, op_func, args).getResult();
    }
    else{
        std::cerr << "error if we get here\n";
        exit(-1);
    }
    opperands.push(result);
    if (program_flags & DEBUG){
        std::cout << "OUT EXPR\n";
    }
}
void CodeGen::VisitPRINT(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT PRINT\n";
    }
    VisitChildren(current_node);
    mlir::Value result = opperands.top();
    opperands.pop();
    auto type_sym = std::static_pointer_cast<Symbol::BuiltInTypeSymbol>(current_node->GetChildren()[0]->GetReference());
    if (type_sym->IsType(Type::INT)){
        mlir::LLVM::LLVMFuncOp printf_function = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
        mlir::LLVM::GlobalOp int_format = module.lookupSymbol<mlir::LLVM::GlobalOp>("int_new_line_format");
        mlir::Value int_format_ptr = builder->create<mlir::LLVM::AddressOfOp>(loc, int_format); 
        mlir::ValueRange args = {int_format_ptr, result};
        builder->create<mlir::LLVM::CallOp>(loc, printf_function, args);
    }
    else if (type_sym->IsType(Type::VECTOR)){
        mlir::LLVM::LLVMFuncOp printf_function = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("print_vector");
        mlir::ValueRange args = {result};
        builder->create<mlir::LLVM::CallOp>(loc, printf_function, args);
        
    } 
    else{
        std::cout << "ERROR IN PRINT\n";
        throw -1;
    }
    if (program_flags & DEBUG){
        std::cout << "OUT PRINT\n";
    }
}

void CodeGen::VisitID(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT ID\n";
    }
    std::string var_name = current_node->GetText();
    auto var_symb = std::static_pointer_cast<Symbol::VarSymbol>(current_scope->Resolve(var_name));
    mlir::Value value_ptr = var_symb->GetValue();
    mlir::Value value;
    if (var_symb->GetTypeSymbol()->IsType(Type::INT)){
        value = builder->create<mlir::LLVM::LoadOp>(loc, int_type, value_ptr);
    }
    else if (var_symb->GetTypeSymbol()->IsType(Type::VECTOR)){
        value = builder->create<mlir::LLVM::LoadOp>(loc, ptr_type, value_ptr);
    }

    opperands.push(value);
    if (program_flags & DEBUG){
        std::cout << "OUT ID\n";
    }
}

void CodeGen::VisitINT(std::shared_ptr<Ast::AstNode> current_node){
    if (program_flags & DEBUG){
        std::cout << "AT INT\n";
    }
    int value = std::stoi(current_node->GetText());
    mlir::Value int_value = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, value);
    opperands.push(int_value);
    if (program_flags & DEBUG){
        std::cout << "OUT INT\n";
    }

}

// Astprogram_flags & DEBUGger Visitor methods
void AstDebugger::DfsTraversal(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "This is the depth first search traversal of the AST:\n";
    Visit(current_node);
    std::cout << std::endl;
}

void AstDebugger::VisitBLOCK(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At BLOCK\n";
    VisitChildren(current_node);
}
void AstDebugger::VisitIF_BLOCK(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At IF_BLOCK\n";
    VisitChildren(current_node);
}
void AstDebugger::VisitLOOP_BLOCK(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At LOOP_BLOCK\n";
    VisitChildren(current_node);
}
void AstDebugger::VisitDECL(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At DECL\n";
    if (current_node->GetChildren()[0]->GetNodeType() == vcalc::VCalcParser::TYPE){
        std::cout << "AT TYPE\n";
    }
    else{
        std::cerr << "ERROR, INVALID DECL FIRST CHILD\n";
        exit(-1);
    }
    VisitChildren(current_node);
}
void AstDebugger::VisitASSIGN(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At ASSIGN\n";
    VisitChildren(current_node);
}
void AstDebugger::ExprPrintOp(std::shared_ptr<Ast::AstNode> current_node){
    std::shared_ptr<Ast::AstNode> oper_node(current_node->GetChildren()[1]);
    switch (oper_node->GetNodeType()){
        case (vcalc::VCalcParser::ADD):
            std::cout << "At ADD\n";
            break;
        case (vcalc::VCalcParser::SUB):
            std::cout << "At SUB\n";
            break;
        case (vcalc::VCalcParser::MUL):
            std::cout << "At MUL\n";
            break;
        case (vcalc::VCalcParser::DIV):
            std::cout << "At DIV\n";
            break;
        case (vcalc::VCalcParser::GREATER):
            std::cout << "At GREATER\n";
            break;
        case (vcalc::VCalcParser::LESS):
            std::cout << "At LESS\n";
            break;
        case (vcalc::VCalcParser::LOGEQ):
            std::cout << "At LOGEQ\n";
            break;
        case (vcalc::VCalcParser::LOGNEQ):
            std::cout << "At LOGNEQ\n";
            break;
        case (vcalc::VCalcParser::INDEX):
            std::cout << "At INDEX\n";
            break;
        case (vcalc::VCalcParser::DOTS):
            std::cout << "At DOTS\n";
            break;
        case (vcalc::VCalcParser::GENERATOR):
            std::cout << "At GENERATOR\n";
            VisitChildren(oper_node);
            break;
        case (vcalc::VCalcParser::FILTER):
            std::cout << "At FILTER\n";
            VisitChildren(oper_node);
            break;
        default:
            std::cerr << "ERROR, INVALID OPERATION NODE\n";
            exit(-1);
    }
}
void AstDebugger::VisitEXPR(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At EXPR\n";
    if (current_node->GetChildren().size() == 1){
        VisitChildren(current_node);
    }
    else{
        Visit(current_node->GetChildren()[0]);
        ExprPrintOp(current_node);
        Visit(current_node->GetChildren()[2]);
    }
}
void AstDebugger::VisitPRINT(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At PRINT\n";
    VisitChildren(current_node);
}
void AstDebugger::VisitID(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At ID\n";
    VisitChildren(current_node);
}
void AstDebugger::VisitINT(std::shared_ptr<Ast::AstNode> current_node){
    std::cout << "At INT\n";
    VisitChildren(current_node);
}

}
