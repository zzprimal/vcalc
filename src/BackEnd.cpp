#include <assert.h>

#include "BackEnd.h"
#include "VCalcParser.h"
#include "Type.h"

BackEnd::BackEnd() : loc(mlir::UnknownLoc::get(&context)) {
    // Load Dialects.
    context.loadDialect<mlir::LLVM::LLVMDialect>();
    context.loadDialect<mlir::arith::ArithDialect>();
    context.loadDialect<mlir::scf::SCFDialect>();
    context.loadDialect<mlir::cf::ControlFlowDialect>();
    context.loadDialect<mlir::memref::MemRefDialect>(); 

    // Initialize the MLIR context 
    builder = std::make_shared<mlir::OpBuilder>(&context);
    module = mlir::ModuleOp::create(builder->getUnknownLoc());
    builder->setInsertionPointToStart(module.getBody());

    // Initalize types
    int_type = mlir::IntegerType::get(&context, 32);
    ptr_type = mlir::LLVM::LLVMPointerType::get(&context);
    vector_type = mlir::LLVM::LLVMStructType::getLiteral(&context, {int_type, ptr_type});

    createGlobalString("%c\0", "char_format");
    createGlobalString("%d\0", "int_format");
    createGlobalString("%d\n\0", "int_new_line_format");
    createGlobalString("%d \0", "int_space_format");

    // Some intial setup to get off the ground 
    setupPrintf();
    DeclarePrintIntSpace();

    /// Int Arithmetic
    CreateIntArithmeticOperation(vcalc::VCalcParser::ADD);
    CreateIntArithmeticOperation(vcalc::VCalcParser::SUB);
    CreateIntArithmeticOperation(vcalc::VCalcParser::MUL);
    CreateIntArithmeticOperation(vcalc::VCalcParser::DIV);

    /// Int Bool
    CreateIntBooleanOperation(vcalc::VCalcParser::LESS);
    CreateIntBooleanOperation(vcalc::VCalcParser::GREATER);
    CreateIntBooleanOperation(vcalc::VCalcParser::LOGEQ);
    CreateIntBooleanOperation(vcalc::VCalcParser::LOGNEQ);
    CreateCastBoolToInt();

    /// Vector Misc
    CreateIntToVectorFunction();
    CreatePrintVectorOperation();
    CreateVectorRangeOperation();
    CreateVectorIndexOperation();
    CreateVectorIndexVectorOperation();

    /// Vector Operations
    CreateConditionalSetVectorFunc();
    CreateVectorSizePromotionFunction();
    CreateVectorMatchSizeFunction();

    CreateVectorOperationFunction(vcalc::VCalcParser::ADD);
    CreateVectorOperationFunction(vcalc::VCalcParser::SUB);
    CreateVectorOperationFunction(vcalc::VCalcParser::MUL);
    CreateVectorOperationFunction(vcalc::VCalcParser::DIV);
    CreateVectorOperationFunction(vcalc::VCalcParser::LESS);
    CreateVectorOperationFunction(vcalc::VCalcParser::GREATER);
    CreateVectorOperationFunction(vcalc::VCalcParser::LOGEQ);
    CreateVectorOperationFunction(vcalc::VCalcParser::LOGNEQ);
}

int BackEnd::emitModule() {
    // Create a main function

    mlir::Type intType = mlir::IntegerType::get(&context, 32);
    auto mainType = mlir::LLVM::LLVMFunctionType::get(intType, {}, false);
    builder->setInsertionPointToEnd(module.getBody());
    main_func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "main", mainType);
    mlir::Block *entry = main_func.addEntryBlock();
    builder->setInsertionPointToStart(entry);
    const_one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    const_zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);

    return 0;
}

int BackEnd::lowerDialects() {
    // Set up the MLIR pass manager to iteratively lower all the Ops
    mlir::PassManager pm(&context);

    // Lower SCF to CF (ControlFlow)
    pm.addPass(mlir::createConvertSCFToCFPass());

    // Lower Arith to LLVM
    pm.addPass(mlir::createArithToLLVMConversionPass());

    // Lower MemRef to LLVM
    pm.addPass(mlir::createFinalizeMemRefToLLVMConversionPass());

    // Lower CF to LLVM
    pm.addPass(mlir::createConvertControlFlowToLLVMPass());

    // Finalize the conversion to LLVM dialect
    pm.addPass(mlir::createReconcileUnrealizedCastsPass());

    // Run the passes
    if (mlir::failed(pm.run(module))) {
        llvm::errs() << "Pass pipeline failed\n";
        return 1;
    }
    return 0;
}

void BackEnd::dumpLLVM(std::ostream &os) {  
    // The only remaining dialects in our module after the passes are builtin
    // and LLVM. Setup translation patterns to get them to LLVM IR.
    mlir::registerBuiltinDialectTranslation(context);
    mlir::registerLLVMDialectTranslation(context);
    llvm_module = mlir::translateModuleToLLVMIR(module, llvm_context);

    // Create llvm ostream and dump into the output file
    llvm::raw_os_ostream output(os);
    output << *llvm_module;
}

void BackEnd::setupPrintf() {
    // Create a function declaration for printf, the signature is:
    //   * `i32 (ptr, ...)`
    auto llvmFnType = mlir::LLVM::LLVMFunctionType::get(int_type, ptr_type,true);
    // Insert the printf function into the body of the parent module.
    builder->create<mlir::LLVM::LLVMFuncOp>(loc, "printf", llvmFnType);
}

void BackEnd::DeclarePrintIntSpace() {
    mlir::Type void_type = mlir::LLVM::LLVMVoidType::get(&context);
    auto type = mlir::LLVM::LLVMFunctionType::get(void_type, {int_type,int_type},true);
    std::string name = "cond_print_space";

    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, name, type);
    auto *entry_block = func.addEntryBlock();
    auto *print_space_block = func.addBlock();
    auto *merge = func.addBlock();

    /// Entry Block
    builder->setInsertionPointToStart(entry_block);
    mlir::Value arg0 = entry_block->getArgument(0);
    mlir::Value arg1 = entry_block->getArgument(1);
    mlir::Value space_condition = builder->create<mlir::LLVM::ICmpOp>(
            loc, mlir::LLVM::ICmpPredicate::ne, arg0, arg1);
    builder->create<mlir::LLVM::CondBrOp>(loc, space_condition, print_space_block, merge);

    /// Print Space
    builder->setInsertionPointToStart(print_space_block);
    PrintChar(' ');
    builder->create<mlir::LLVM::BrOp>(loc, merge);

    /// Merge
    builder->setInsertionPointToStart(merge);
    builder->create<mlir::LLVM::ReturnOp>(loc,nullptr);
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreateConditionalSetVectorFunc() {
    mlir::Type void_type = mlir::LLVM::LLVMVoidType::get(&context);
    auto type = mlir::LLVM::LLVMFunctionType::get(void_type, {ptr_type,ptr_type,int_type,int_type,int_type},true);

    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "cond_copy_arr", type);
    auto *entry_block = func.addEntryBlock();
    auto *in_bounds_block = func.addBlock();
    auto *out_bounds_block = func.addBlock();
    auto *merge = func.addBlock();

    /// ENTRY BLOCK
    builder->setInsertionPointToStart(entry_block);
    mlir::Value arr = entry_block->getArgument(0);
    mlir::Value arr_to_copy = entry_block->getArgument(1);
    mlir::Value index = entry_block->getArgument(2);
    mlir::Value old_size = entry_block->getArgument(3);
    mlir::Value set_value = entry_block->getArgument(4);
    mlir::Value value_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr,
            mlir::ValueRange{index}
    );

    mlir::Value in_bounds = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, index, old_size);
    builder->create<mlir::LLVM::CondBrOp>(loc, in_bounds, in_bounds_block, out_bounds_block);

    /// IN BOUNDS BLOCK
    builder->setInsertionPointToStart(in_bounds_block);
    mlir::Value value_to_copy_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr_to_copy,
            mlir::ValueRange{index}
    );
    mlir::Value copy_value = builder->create<mlir::LLVM::LoadOp>(loc,int_type,value_to_copy_addr);
    builder->create<mlir::LLVM::StoreOp>(loc,copy_value, value_addr);
    builder->create<mlir::LLVM::BrOp>(loc, merge);

    /// OUT OF BOUNDS BLOCK
    builder->setInsertionPointToStart(out_bounds_block);
    builder->create<mlir::LLVM::StoreOp>(loc,set_value, value_addr);
    builder->create<mlir::LLVM::BrOp>(loc, merge);

    // MERGE BLOCK
    builder->setInsertionPointToStart(merge);
    builder->create<mlir::LLVM::ReturnOp>(loc,nullptr);

    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::createGlobalString(const char *str, const char *stringName) {

    mlir::Type charType = mlir::IntegerType::get(&context, 8);

    // create string and string type
    auto mlirString = mlir::StringRef(str, strlen(str) + 1);
    auto mlirStringType = mlir::LLVM::LLVMArrayType::get(charType, mlirString.size());

    builder->create<mlir::LLVM::GlobalOp>(loc, mlirStringType, /*isConstant=*/true,
                            mlir::LLVM::Linkage::Internal, stringName,
                            builder->getStringAttr(mlirString), /*alignment=*/0);
}
void BackEnd::CreateCastBoolToInt(){
    mlir::Type int1_type = mlir::IntegerType::get(&context, 1);
    auto type = mlir::LLVM::LLVMFunctionType::get(int_type, {int1_type},true);
    std::string name = "bool_to_int";
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, name, type);
    auto *entryBlock = func.addEntryBlock();

    /// Entry Block
    builder->setInsertionPointToStart(entryBlock);
    mlir::Value arg0 = entryBlock->getArgument(0); // First argument

    auto true_dest = func.addBlock();
    auto false_dest = func.addBlock();
    builder->create<mlir::LLVM::CondBrOp>(loc, arg0, true_dest, false_dest);

    /// True Dest
    builder->setInsertionPointToStart(true_dest);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    builder->create<mlir::LLVM::ReturnOp>(loc, one);

    /// False Dest
    builder->setInsertionPointToStart(false_dest);
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    builder->create<mlir::LLVM::ReturnOp>(loc, zero);

    builder->setInsertionPointToStart(module.getBody());
}

mlir::LLVM::GlobalOp BackEnd::CreateGlobalInt(int val, const char *name) {
    return builder->create<mlir::LLVM::GlobalOp>(
        loc,
        int_type,
        true,
        mlir::LLVM::Linkage::Internal,
        name,
        builder->getIntegerAttr(int_type,val),
        0
    );
}

void BackEnd::CreateIntArithmeticOperation(size_t op) {
    auto type = mlir::LLVM::LLVMFunctionType::get(int_type, {int_type,int_type},true);
    std::string name = GetOperationFunc(op,Type::VCalcTypes::INT);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, name, type);
    auto *entryBlock = func.addEntryBlock();
    builder->setInsertionPointToStart(entryBlock);

    mlir::Value arg0 = entryBlock->getArgument(0); // First argument
    mlir::Value arg1 = entryBlock->getArgument(1); // Second argument
    mlir::Value result;
    switch (op) {
        case vcalc::VCalcParser::ADD:
            result = builder->create<mlir::LLVM::AddOp>(loc, arg0, arg1);
            break;
        case vcalc::VCalcParser::SUB:
            result = builder->create<mlir::LLVM::SubOp>(loc, arg0, arg1);
            break;
        case vcalc::VCalcParser::DIV:
            result = builder->create<mlir::LLVM::SDivOp>(loc, arg0, arg1);
            break;
        case vcalc::VCalcParser::MUL:
            result = builder->create<mlir::LLVM::MulOp>(loc, arg0, arg1);
            break;
        default:
            break;
    }
    builder->create<mlir::LLVM::ReturnOp>(loc, result);
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreateIntBooleanOperation(size_t op) {
    auto type = mlir::LLVM::LLVMFunctionType::get(int_type, {int_type,int_type},true);
    std::string name = GetOperationFunc(op,Type::VCalcTypes::INT);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, name, type);
    auto *entryBlock = func.addEntryBlock();

    /// Entry Block
    builder->setInsertionPointToStart(entryBlock);
    mlir::Value arg0 = entryBlock->getArgument(0); // First argument
    mlir::Value arg1 = entryBlock->getArgument(1); // Second argument
    mlir::Value predicate;

    switch (op) {
        case vcalc::VCalcParser::LESS:
            predicate = builder->create<mlir::LLVM::ICmpOp>(
                    loc, mlir::LLVM::ICmpPredicate::slt, arg0, arg1);
            break;
        case vcalc::VCalcParser::GREATER:
            predicate = builder->create<mlir::LLVM::ICmpOp>(
                    loc, mlir::LLVM::ICmpPredicate::slt, arg1, arg0);
            break;
        case vcalc::VCalcParser::LOGEQ:
            predicate = builder->create<mlir::LLVM::ICmpOp>(
                    loc, mlir::LLVM::ICmpPredicate::eq, arg0, arg1);
            break;
        case vcalc::VCalcParser::LOGNEQ:
            predicate = builder->create<mlir::LLVM::ICmpOp>(
                    loc, mlir::LLVM::ICmpPredicate::ne, arg0, arg1);
            break;
        default:
            break;
    }

    auto true_dest = func.addBlock();
    auto false_dest = func.addBlock();

    builder->create<mlir::LLVM::CondBrOp>(loc, predicate, true_dest, false_dest);

    /// True Dest
    builder->setInsertionPointToStart(true_dest);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    builder->create<mlir::LLVM::ReturnOp>(loc, one);

    /// False Dest
    builder->setInsertionPointToStart(false_dest);
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    builder->create<mlir::LLVM::ReturnOp>(loc, zero);

    builder->setInsertionPointToStart(module.getBody());
}

mlir::Value BackEnd::CreateIntPointer(mlir::Value value) {
    mlir::LLVM::LLVMFuncOp mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, int_type);

    mlir::Value four = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 4);
    mlir::Value ptr = builder->create<mlir::LLVM::CallOp>(loc, mallocFn, mlir::ValueRange{four}).getResult();
    builder->create<mlir::LLVM::StoreOp>(loc, value, ptr);
    return ptr;
}

std::string BackEnd::GetOperationFunc(size_t op, size_t data_type = -1) {
    std::string data_name;
    switch (data_type) {
        case Type::VCalcTypes::INT:
            data_name = "int";
            break;
        case Type::VCalcTypes::VECTOR:
            data_name = "vector";
            break;
        default:
            break;
    }
    std::string op_name;
    switch (op) {
        case vcalc::VCalcParser::ADD:
            op_name = "add";
            break;
        case vcalc::VCalcParser::SUB:
            op_name = "sub";
            break;
        case vcalc::VCalcParser::DIV:
            op_name = "div";
            break;
        case vcalc::VCalcParser::MUL:
            op_name = "mul";
            break;
        case vcalc::VCalcParser::LESS:
            op_name = "less_than";
            break;
        case vcalc::VCalcParser::GREATER:
            op_name = "greater_than";
            break;
        case vcalc::VCalcParser::LOGEQ:
            op_name = "equal";
            break;
        case vcalc::VCalcParser::LOGNEQ:
            op_name = "nequal";
            break;
        default:
            break;
    }
    return data_name + "_" + op_name;
}

void BackEnd::CreateVectorOperationFunction(size_t op) {
    std::string func_name = GetOperationFunc(op,Type::VCalcTypes::VECTOR);
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {ptr_type,ptr_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);
    auto *entryBlock = func.addEntryBlock();

    /// ENTRY
    builder->setInsertionPointToStart(entryBlock);
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value arg0 = entryBlock->getArgument(0); // First argument
    mlir::Value arg1 = entryBlock->getArgument(1); // Second argument

    mlir::LLVM::LLVMFuncOp check_size_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("match_vector_size");

    // Increases lhs vector size if required
    mlir::ValueRange args = {arg0,arg1,zero};
    arg0 = builder->create<mlir::LLVM::CallOp>(loc, check_size_func, args).getResult();

    // Prevents division by zero when rhs size increased
    mlir::Value default_value;
    if (op == vcalc::VCalcParser::DIV) {
        default_value = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    } else {
        default_value = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    }

    // Increases rhs vector size if required
    mlir::ValueRange args1 = {arg1,arg0,default_value};
    arg1 = builder->create<mlir::LLVM::CallOp>(loc, check_size_func, args1).getResult();


    mlir::Value arr_0 = builder->create<mlir::LLVM::GEPOp>(loc,ptr_type,int_type,arg0,mlir::ValueRange{one});
    mlir::Value arr0_size_addr = builder->create<mlir::LLVM::GEPOp>(loc,ptr_type,int_type,arg0,mlir::ValueRange{zero});
    mlir::Value arr_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr0_size_addr);

    mlir::Value arr_1 = builder->create<mlir::LLVM::GEPOp>(loc,ptr_type,int_type,arg1,mlir::ValueRange{one});

    mlir::Value result_ptr = GenerateVectorTypePtr(arr_size);

    std::unordered_set<size_t> arithmetic_operations = {
            vcalc::VCalcParser::ADD,
            vcalc::VCalcParser::SUB,
            vcalc::VCalcParser::MUL,
            vcalc::VCalcParser::DIV
    };

    std::unordered_set<size_t> bool_operations = {
            vcalc::VCalcParser::LESS,
            vcalc::VCalcParser::GREATER,
            vcalc::VCalcParser::LOGEQ,
            vcalc::VCalcParser::LOGNEQ
    };

    auto it = arithmetic_operations.find(op);
    if (it != arithmetic_operations.end()) {
        auto vector_func = VectorArithmeticOperationFunction(op, arr_0, arr_1);
        vector_func.Generate(this,func,result_ptr,arr_size);
    }
    it = bool_operations.find(op);
    if (it != arithmetic_operations.end()) {
        auto vector_func = VectorBooleanOperationFunction(op, arr_0, arr_1);
        vector_func.Generate(this,func,result_ptr,arr_size);
    }

    builder->create<mlir::LLVM::ReturnOp>(loc, result_ptr);
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreateVectorMatchSizeFunction() {
    //mlir::Type void_type = mlir::LLVM::LLVMVoidType::get(&context);
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {ptr_type,ptr_type,int_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, "match_vector_size", type);
    auto *entryBlock = func.addEntryBlock();
    auto *check_arg0_size = func.addBlock();
    auto *arg0_smaller_block = func.addBlock();
    auto *merge = func.addBlock();

    /// Entry Block
    builder->setInsertionPointToStart(entryBlock);
    mlir::Value arg0 = entryBlock->getArgument(0);
    mlir::Value arg1 = entryBlock->getArgument(1);
    mlir::Value default_value = entryBlock->getArgument(2);

    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    //mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    mlir::LLVM::LLVMFuncOp copy_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("increase_vector_size");

    mlir::Value arr0_size_addr = builder->create<mlir::LLVM::GEPOp>(loc,ptr_type,int_type,arg0,mlir::ValueRange{zero});
    mlir::Value arr_0_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr0_size_addr);

    mlir::Value arr1_size_addr = builder->create<mlir::LLVM::GEPOp>(loc,ptr_type,int_type,arg1,mlir::ValueRange{zero});
    mlir::Value arr_1_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr1_size_addr);
    builder->create<mlir::LLVM::BrOp>(loc,check_arg0_size);

    /// Check arr_0 size block
    builder->setInsertionPointToStart(check_arg0_size);
    mlir::Value arr_0_smaller = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, arr_0_size, arr_1_size);
    builder->create<mlir::LLVM::CondBrOp>(loc, arr_0_smaller, arg0_smaller_block, merge);

    /// arr_0 smaller block
    builder->setInsertionPointToStart(arg0_smaller_block);
    mlir::ValueRange args = {arg0,arr_1_size,default_value};
    mlir::Value arg0_corrected = builder->create<mlir::LLVM::CallOp>(loc, copy_func, args).getResult();
    builder->create<mlir::LLVM::ReturnOp>(loc, arg0_corrected);

    /// Merge
    builder->setInsertionPointToStart(merge);
    builder->create<mlir::LLVM::ReturnOp>(loc, arg0);
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::SwitchAndFreeVector(mlir::Value vector_ptr, mlir::Value copy_vector_ptr) {
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value arr_ptr = builder->create<mlir::LLVM::GEPOp>(loc, ptr_type, vector_type, vector_ptr, mlir::ValueRange{one});
    mlir::Value arr_size_ptr = builder->create<mlir::LLVM::GEPOp>(loc, ptr_type, vector_type, vector_ptr, mlir::ValueRange{zero});

    mlir::Value copy_arr_ptr = builder->create<mlir::LLVM::GEPOp>(loc, ptr_type, vector_type, copy_vector_ptr, mlir::ValueRange{one});
    mlir::Value copy_arr_size_ptr = builder->create<mlir::LLVM::GEPOp>(loc, ptr_type, vector_type, copy_vector_ptr, mlir::ValueRange{zero});
    mlir::Value copy_arr_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,copy_arr_size_ptr);

    //mlir::Value loaded_value = builder->create<mlir::LLVM::LoadOp>(loc, ptr_type, copy_arr_ptr);
    mlir::Value copy_arr_address = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, copy_arr_ptr.getType(), copy_arr_ptr, mlir::ValueRange{}
    );
    builder->create<mlir::LLVM::StoreOp>(loc,copy_arr_address,arr_ptr);
    builder->create<mlir::LLVM::StoreOp>(loc, copy_arr_size, arr_size_ptr);
}

mlir::Value BackEnd::GenerateVectorTypePtr(mlir::Value arr_size) {
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);


    mlir::LLVM::LLVMFuncOp mallocFn = mlir::LLVM::lookupOrCreateMallocFn(module, int_type);
    mlir::Value eight = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 8);

    mlir::Value vectorAddr = builder->create<mlir::LLVM::CallOp>(
            loc, mallocFn, mlir::ValueRange{eight}).getResult();

    // Calculate size in bytes
    mlir::Value int_size = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 4);
    mlir::Value alloc_arr_size = builder->create<mlir::LLVM::MulOp>(loc,arr_size,int_size);

    // Allocate space for int32 arr
    mlir::Value arrayAddr = builder->create<mlir::LLVM::CallOp>(
            loc, mallocFn, mlir::ValueRange{alloc_arr_size}).getResult();

    mlir::Value arrayPtr = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, vector_type, vectorAddr, mlir::ValueRange{one});
    builder->create<mlir::LLVM::StoreOp>(loc,arrayAddr,arrayPtr);

    mlir::Value sizeAddr = builder->create<mlir::LLVM::GEPOp>(
            loc, ptr_type, vector_type, vectorAddr, mlir::ValueRange{zero});
    builder->create<mlir::LLVM::StoreOp>(loc, arr_size, sizeAddr);
    return vectorAddr;
}
// Generates a mlir function which creates promotes an int to a vector of size arg1 with values arg0.
// Eg (5,4) -> [5,5,5,5]
void BackEnd::CreateIntToVectorFunction() {
    std::string func_name = "int_to_vector";
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {int_type, int_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);

    auto *entryBlock = func.addEntryBlock();
    builder->setInsertionPointToStart(entryBlock);

    mlir::Value arr_size = entryBlock->getArgument(0);
    mlir::Value const_value = entryBlock->getArgument(1);

    mlir::Value vector_ptr = GenerateVectorTypePtr(arr_size);

    auto vector_func = IntVectorPromotionFunction(const_value);
    vector_func.Generate(this,func,vector_ptr,arr_size);

    builder->create<mlir::LLVM::ReturnOp>(loc, vector_ptr);

    builder->setInsertionPointToStart(module.getBody());
}

// Generates a mlir function which creates a vector with values between arg0 and arg1
// Eg (1,4) -> [1,2,3,4]
void BackEnd::CreateVectorRangeOperation() {
    std::string func_name = "vector_range";
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {int_type, int_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);

    auto *entryBlock = func.addEntryBlock();
    builder->setInsertionPointToStart(entryBlock);

    mlir::Value lower_bound = entryBlock->getArgument(0);
    mlir::Value upper_bound = entryBlock->getArgument(1);

    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);

    mlir::Block *if_block = func.addBlock();
    mlir::Block *else_block = func.addBlock();

    mlir::Value condition = builder->create<mlir::LLVM::ICmpOp>(loc, mlir::LLVM::ICmpPredicate::slt, upper_bound, lower_bound);
    builder->create<mlir::LLVM::CondBrOp>(loc, condition, if_block, else_block);

    builder->setInsertionPointToStart(if_block);
    mlir::Value if_vector_ptr = GenerateVectorTypePtr(zero); 
    builder->create<mlir::LLVM::ReturnOp>(loc, if_vector_ptr);

    builder->setInsertionPointToStart(else_block);
    mlir::Value dif = builder->create<mlir::LLVM::SubOp>(loc,upper_bound,lower_bound);
    mlir::Value arr_size = builder->create<mlir::LLVM::AddOp>(loc,dif,one);
    mlir::Value else_vector_ptr =  GenerateVectorTypePtr(arr_size);
    auto vector_func = RangeVectorFunction(lower_bound);
    vector_func.Generate(this,func, else_vector_ptr, arr_size);
    builder->create<mlir::LLVM::ReturnOp>(loc, else_vector_ptr);

    
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreateVectorIndexOperation() {
    std::string func_name = "vector_index";
    auto type = mlir::LLVM::LLVMFunctionType::get(int_type, {ptr_type, int_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);

    auto *entryBlock = func.addEntryBlock();

    auto out_of_bounds_block = func.addBlock();
    auto check_upper_bound = func.addBlock();
    auto valid_index_block = func.addBlock();

    builder->setInsertionPointToStart(entryBlock);
    mlir::Value vector_ptr = entryBlock->getArgument(0);
    mlir::Value index = entryBlock->getArgument(1);

    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value index_less_than_zero = builder->create<mlir::LLVM::ICmpOp>(
        loc,
        mlir::LLVM::ICmpPredicate::slt,
        index,
        zero
    );
    builder->create<mlir::LLVM::CondBrOp>(loc, index_less_than_zero, out_of_bounds_block, check_upper_bound);

    builder->setInsertionPointToStart(check_upper_bound);
    mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            vector_ptr,
            mlir::ValueRange{zero}
    );

    mlir::Value arr_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,size_addr);
    mlir::Value arr_size_minus_one = builder->create<mlir::LLVM::SubOp>(loc,arr_size,one);
    mlir::Value size_minus_one_less_index = builder->create<mlir::LLVM::ICmpOp>(
            loc,
            mlir::LLVM::ICmpPredicate::slt,
            arr_size_minus_one,
            index
    );
    builder->create<mlir::LLVM::CondBrOp>(loc, size_minus_one_less_index,out_of_bounds_block, valid_index_block);

    // Index is not within arr bounds so return zero
    builder->setInsertionPointToStart(out_of_bounds_block);
    builder->create<mlir::LLVM::ReturnOp>(loc, zero);

    builder->setInsertionPointToStart(valid_index_block);
    mlir::Value arr_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            vector_ptr,
            mlir::ValueRange{one}
    );
    mlir::Value value_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr_ptr,
            mlir::ValueRange{index}
    );
    mlir::Value value = builder->create<mlir::LLVM::LoadOp>(loc,int_type,value_addr);
    builder->create<mlir::LLVM::ReturnOp>(loc, value);
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreateVectorIndexVectorOperation() {
    std::string func_name = "vector_index_vector";
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {ptr_type, ptr_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);

    auto *entryBlock = func.addEntryBlock();
    builder->setInsertionPointToStart(entryBlock);

    mlir::Value domain_vector = entryBlock->getArgument(0);
    mlir::Value index_vector = entryBlock->getArgument(1);

    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value index_size_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            index_vector,
            mlir::ValueRange{zero}
    );
    mlir::Value index_arr_size = builder->create<mlir::LLVM::LoadOp>(loc, int_type, index_size_addr);

    mlir::Value result = GenerateVectorTypePtr(index_arr_size);
    mlir::Value result_arr_ptr = builder->create<mlir::LLVM::GEPOp>(
        loc,
        ptr_type,
        int_type,
        result,
        mlir::ValueRange{one}
    );

    mlir::LLVM::LLVMFuncOp index_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_index");
    mlir::scf::ForOp for_loop = builder->create<mlir::scf::ForOp>(loc, zero, index_arr_size, one);
    mlir::Value loop_index = for_loop.getInductionVar();
    mlir::Block *for_loop_body = for_loop.getBody();
    mlir::OpBuilder::InsertPoint save = builder->saveInsertionPoint();
    builder->setInsertionPointToStart(for_loop_body);

    mlir::Value domain_vec_index = builder->create<mlir::LLVM::CallOp>(loc, index_func, mlir::ValueRange{index_vector, loop_index}).getResult();

    mlir::Value domain_vec_val = builder->create<mlir::LLVM::CallOp>(loc, index_func, mlir::ValueRange{domain_vector, domain_vec_index}).getResult();
    
    mlir::Value arr_index_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            result_arr_ptr,
            mlir::ValueRange{loop_index}
    );
    builder->create<mlir::LLVM::StoreOp>(loc, domain_vec_val, arr_index_ptr);
    
    builder->restoreInsertionPoint(save);
    builder->create<mlir::LLVM::ReturnOp>(loc, result);
    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreatePrintVectorOperation() {
    std::string func_name = "print_vector";
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {ptr_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);

    auto *entryBlock = func.addEntryBlock();
    builder->setInsertionPointToStart(entryBlock);

    PrintChar('[');
    mlir::Value vector_ptr = entryBlock->getArgument(0);

    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    //mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            vector_ptr,
            mlir::ValueRange{zero}
    );
    mlir::Value arr_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,size_addr);

    auto vector_func = VectorPrintFunction();
    vector_func.Generate(this,func,vector_ptr,arr_size);

    PrintChar(']');
    PrintChar('\n');
    builder->create<mlir::LLVM::ReturnOp>(loc, vector_ptr);

    builder->setInsertionPointToStart(module.getBody());
}

void BackEnd::CreateVectorSizePromotionFunction() {
    std::string func_name = "increase_vector_size";
    auto type = mlir::LLVM::LLVMFunctionType::get(ptr_type, {ptr_type,int_type,int_type},true);
    auto func = builder->create<mlir::LLVM::LLVMFuncOp>(loc, func_name, type);

    auto *entryBlock = func.addEntryBlock();
    builder->setInsertionPointToStart(entryBlock);

    mlir::Value vector_ptr = entryBlock->getArgument(0);
    mlir::Value new_size = entryBlock->getArgument(1);
    mlir::Value default_value = entryBlock->getArgument(2);

    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value new_vector = GenerateVectorTypePtr(new_size);
    mlir::Value size_addr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            vector_ptr,
            mlir::ValueRange{zero}
    );
    mlir::Value arr_size = builder->create<mlir::LLVM::LoadOp>(loc,int_type,size_addr);

    mlir::Value copy_arr_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            vector_ptr,
            mlir::ValueRange{one}
    );

    auto vector_func = IncreaseVectorSizeMLIRFunction(copy_arr_ptr, arr_size, default_value);
    vector_func.Generate(this,func,new_vector,new_size);

    builder->create<mlir::LLVM::ReturnOp>(loc, new_vector);
    builder->setInsertionPointToStart(module.getBody());
}


void BackEnd::TestArithmeticInt(mlir::ValueRange args, mlir::LLVM::LLVMFuncOp func, mlir::Value formatStringPtr) {
    auto result = builder->create<mlir::LLVM::CallOp>(loc, func, args);
    auto int_result = result.getResult();

    mlir::ValueRange print_args = {formatStringPtr, int_result};
    mlir::LLVM::LLVMFuncOp printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, print_args);
}

void BackEnd::TestIndex(mlir::ValueRange args) {
    auto index_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("vector_index");
    auto index_func_call = builder->create<mlir::LLVM::CallOp>(loc,index_func,args);
    auto val = index_func_call.getResult();
    PrintInt(val);
    PrintChar('\n');
}

void BackEnd::PrintInt(mlir::Value value) {
    auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("int_format");
    mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    mlir::ValueRange print_args = {formatStringPtr, value};
    mlir::LLVM::LLVMFuncOp printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, print_args);
}
void BackEnd::PrintChar(char c) {
    int8_t charValue = static_cast<int8_t>(c);
    auto charType = mlir::IntegerType::get(builder->getContext(), 8);
    mlir::Value char_const = builder->create<mlir::LLVM::ConstantOp>(
            loc, charType, mlir::APInt(8, charValue));
    auto formatString = module.lookupSymbol<mlir::LLVM::GlobalOp>("char_format");
    mlir::Value formatStringPtr = builder->create<mlir::LLVM::AddressOfOp>(loc, formatString);
    mlir::ValueRange print_args = {formatStringPtr, char_const};
    mlir::LLVM::LLVMFuncOp printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("printf");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, print_args);

}

void BackEnd::TestVectorOp(mlir::ValueRange args, mlir::LLVM::LLVMFuncOp func) {
    auto vec_func_call = builder->create<mlir::LLVM::CallOp>(loc,func,args);
    auto vec = vec_func_call.getResult();

    auto print_vec = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("print_vector");
    builder->create<mlir::LLVM::CallOp>(loc,print_vec,mlir::ValueRange{vec});
}


mlir::ModuleOp BackEnd::GetModule() {
    return module;
}
mlir::Location BackEnd::GetLocation() {
    return loc;
}
std::shared_ptr<mlir::OpBuilder> BackEnd::GetBuilder() {
    return builder;
}

mlir::Type BackEnd::GetMLIRType(BackendMLIRType type) {
    switch (type) {
        case BackendMLIRType::Int:
            return int_type;
        case BackendMLIRType::Ptr:
            return ptr_type;
        case BackendMLIRType::Vector:
            return vector_type;
        default:
            return nullptr;
    }
}


void VectorLoopMLIRFunction::PreHeaderFunc(BackEnd* backend) {

}

void
VectorLoopMLIRFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,
                                 mlir::LLVM::LLVMFuncOp func) {

}

void VectorLoopMLIRFunction::Generate(BackEnd* backend, mlir::LLVM::LLVMFuncOp func, mlir::Value vector_ptr,mlir::Value upper_bound) {
    //auto module = backend->GetModule();
    auto loc = backend->GetLocation();
    auto builder = backend->GetBuilder();
    auto int_type = backend->GetMLIRType(BackendMLIRType::Int);
    auto ptr_type = backend->GetMLIRType(BackendMLIRType::Ptr);
    mlir::Value one1 = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    mlir::Value arr_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            vector_ptr,
            mlir::ValueRange{one1}
    );
    mlir::Block *preHeader = func.addBlock();
    mlir::Block *header = func.addBlock();
    mlir::Block *body = func.addBlock();
    mlir::Block *merge = func.addBlock();

    /// ============== PRE-HEADER ==============
    // First, jump immediately into the pre-header (no fallthrough in LLVM)
    builder->create<mlir::LLVM::BrOp>(loc, preHeader);
    builder->setInsertionPointToStart(preHeader);

    // Init the induction variable
    mlir::Value zero = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 0);
    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);

    mlir::Value iAddr = builder->create<mlir::LLVM::AllocaOp>(
            loc, ptr_type, int_type, one);
    builder->create<mlir::LLVM::StoreOp>(loc, zero, iAddr);

    PreHeaderFunc(backend);

    builder->create<mlir::LLVM::BrOp>(loc, header);

    /// ============== HEADER ==============
    builder->setInsertionPointToStart(header);

    // Load the induction variable and compare
    mlir::Value iValue = builder->create<mlir::LLVM::LoadOp>(loc, int_type, iAddr);
    mlir::Value ltCond = builder->create<mlir::LLVM::ICmpOp>(
            loc, mlir::LLVM::ICmpPredicate::slt, iValue, upper_bound);
    builder->create<mlir::LLVM::CondBrOp>(loc, ltCond, body, merge);

    /// ============== BODY ==============
    builder->setInsertionPointToStart(body);

    LoopFunc(backend, iValue, arr_ptr, upper_bound, func);
    // Iterate iterator
    mlir::Value iIncrement = builder->create<mlir::LLVM::AddOp>(loc, int_type, iValue, one);
    builder->create<mlir::LLVM::StoreOp>(loc, iIncrement, iAddr);


    builder->create<mlir::LLVM::BrOp>(loc, header);

    /// ============== MERGE ==============
    builder->setInsertionPointToStart(merge);
}

VectorLoopMLIRFunction::VectorLoopMLIRFunction() {

}

void IntVectorPromotionFunction::PreHeaderFunc(BackEnd *backend) {

}

void IntVectorPromotionFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size, mlir::LLVM::LLVMFuncOp func) {
    auto loc = backend->GetLocation();
    auto builder = backend->GetBuilder();
    auto int_type = backend->GetMLIRType(BackendMLIRType::Int);
    auto ptr_type = backend->GetMLIRType(BackendMLIRType::Ptr);

    mlir::Value array_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type, arr_ptr,
            mlir::ValueRange{i_value}
    );
    builder->create<mlir::LLVM::StoreOp>(loc, const_value, array_element_ptr);

}

void RangeVectorFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,
                                   mlir::LLVM::LLVMFuncOp func) {
    auto loc = backend->GetLocation();
    auto builder = backend->GetBuilder();
    auto int_type = backend->GetMLIRType(BackendMLIRType::Int);
    auto ptr_type = backend->GetMLIRType(BackendMLIRType::Ptr);

    mlir::Value array_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type, arr_ptr,
            mlir::ValueRange{i_value}
    );
    mlir::Value sum = builder->create<mlir::LLVM::AddOp>(loc,lower_bound,i_value);
    builder->create<mlir::LLVM::StoreOp>(loc, sum, array_element_ptr);
}

void VectorArithmeticOperationFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size, mlir::LLVM::LLVMFuncOp func) {
    auto loc = backend->GetLocation();
    auto builder = backend->GetBuilder();
    auto int_type = backend->GetMLIRType(BackendMLIRType::Int);
    auto ptr_type = backend->GetMLIRType(BackendMLIRType::Ptr);

    mlir::Value arr0_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr0_ptr,
            mlir::ValueRange{i_value}
    );
    mlir::Value arr0_val = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr0_element_ptr);

    mlir::Value arr1_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr1_ptr,
            mlir::ValueRange{i_value}
    );
    mlir::Value arr1_val = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr1_element_ptr);

    mlir::Value result;
    switch (op) {
        case vcalc::VCalcParser::ADD:
            result = builder->create<mlir::LLVM::AddOp>(loc, arr0_val, arr1_val);
            break;
        case vcalc::VCalcParser::SUB:
            result = builder->create<mlir::LLVM::SubOp>(loc, arr0_val, arr1_val);
            break;
        case vcalc::VCalcParser::DIV:
            result = builder->create<mlir::LLVM::SDivOp>(loc, arr0_val, arr1_val);
            break;
        case vcalc::VCalcParser::MUL:
            result = builder->create<mlir::LLVM::MulOp>(loc, arr0_val, arr1_val);
            break;
        default:
            break;
    }

    mlir::Value result_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr_ptr,
            mlir::ValueRange{i_value}
    );
    builder->create<mlir::LLVM::StoreOp>(loc, result, result_element_ptr);
}

void VectorBooleanOperationFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr,mlir::Value arr_size, mlir::LLVM::LLVMFuncOp func) {
    auto loc = backend->GetLocation();
    auto builder = backend->GetBuilder();
    auto int_type = backend->GetMLIRType(BackendMLIRType::Int);
    auto ptr_type = backend->GetMLIRType(BackendMLIRType::Ptr);

    mlir::Value arr0_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr0_ptr,
            mlir::ValueRange{i_value}
    );
    mlir::Value arr0_val = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr0_element_ptr);

    mlir::Value arr1_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr1_ptr,
            mlir::ValueRange{i_value}
    );
    mlir::Value arr1_val = builder->create<mlir::LLVM::LoadOp>(loc,int_type,arr1_element_ptr);

    mlir::ValueRange args = {arr0_val,arr1_val};
    auto func_call = builder->create<mlir::LLVM::CallOp>(loc, bool_int_func, args);
    auto result = func_call.getResult();

    mlir::Value result_element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr_ptr,
            mlir::ValueRange{i_value}
    );

    builder->create<mlir::LLVM::StoreOp>(loc, result, result_element_ptr);

}

void VectorBooleanOperationFunction::PreHeaderFunc(BackEnd *backend) {
    auto module = backend->GetModule();
    std::string func_name = backend->GetOperationFunc(op,Type::VCalcTypes::INT);
    bool_int_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>(func_name);

}

void VectorPrintFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,mlir::LLVM::LLVMFuncOp func) {
    auto loc = backend->GetLocation();
    auto module = backend->GetModule();
    auto builder = backend->GetBuilder();
    auto int_type = backend->GetMLIRType(BackendMLIRType::Int);
    auto ptr_type = backend->GetMLIRType(BackendMLIRType::Ptr);

    mlir::Value one = builder->create<mlir::LLVM::ConstantOp>(loc, int_type, 1);
    mlir::Value iIncrement = builder->create<mlir::LLVM::AddOp>(loc, int_type, i_value, one);

    mlir::Value element_ptr = builder->create<mlir::LLVM::GEPOp>(
            loc,
            ptr_type,
            int_type,
            arr_ptr,
            mlir::ValueRange{i_value}
    );
    mlir::Value val = builder->create<mlir::LLVM::LoadOp>(loc,int_type,element_ptr);
    backend->PrintInt(val);

    mlir::ValueRange print_args = {iIncrement, arr_size};
    mlir::LLVM::LLVMFuncOp printfFunc = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("cond_print_space");
    builder->create<mlir::LLVM::CallOp>(loc, printfFunc, print_args);
}

void IncreaseVectorSizeMLIRFunction::LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr,mlir::Value arr_size, mlir::LLVM::LLVMFuncOp func) {
    auto loc = backend->GetLocation();
    auto builder = backend->GetBuilder();

    // Sets value at arr to the original value if its in bounds, otherwise to default value
    mlir::ValueRange call_args = {arr_ptr, copy_arr_ptr, i_value,old_size,default_value};
    builder->create<mlir::LLVM::CallOp>(loc, cond_set_func, call_args);
}

void IncreaseVectorSizeMLIRFunction::PreHeaderFunc(BackEnd *backend) {
    auto module = backend->GetModule();
    cond_set_func = module.lookupSymbol<mlir::LLVM::LLVMFuncOp>("cond_copy_arr");
}
