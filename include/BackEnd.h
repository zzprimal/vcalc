#pragma once
// Pass manager
#include "mlir/Pass/Pass.h"
#include "mlir/Pass/PassManager.h"
#include "mlir/Conversion/SCFToControlFlow/SCFToControlFlow.h"
#include "mlir/Conversion/ControlFlowToLLVM/ControlFlowToLLVM.h"
#include "mlir/Conversion/ArithToLLVM/ArithToLLVM.h"
#include "mlir/Conversion/MemRefToLLVM/MemRefToLLVM.h"
#include "mlir/Conversion/FuncToLLVM/ConvertFuncToLLVM.h"
#include "mlir/Conversion/ReconcileUnrealizedCasts/ReconcileUnrealizedCasts.h"

// Translation
#include "mlir/Target/LLVMIR/Dialect/LLVMIR/LLVMToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Dialect/Builtin/BuiltinToLLVMIRTranslation.h"
#include "mlir/Target/LLVMIR/Export.h"
#include "llvm/Support/raw_os_ostream.h"

// MLIR IR
#include "mlir/IR/BuiltinAttributes.h"
#include "mlir/IR/TypeRange.h"
#include "mlir/IR/Value.h"
#include "mlir/IR/ValueRange.h"
#include "mlir/IR/Verifier.h"
#include "mlir/IR/Builders.h"
#include "mlir/IR/MLIRContext.h"
#include "mlir/IR/BuiltinOps.h"
// Dialects 
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h"
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/ControlFlow/IR/ControlFlow.h"
#include "mlir/Dialect/LLVMIR/FunctionCallUtils.h"

// Other
#include <assert.h>
#include "VCalcParser.h"
#include "Type.h"
enum BackendMLIRType {
    Int,
    Ptr,
    Vector
};

class BackEnd {
    public:
        BackEnd();

        int emitModule();
        int lowerDialects();
        void dumpLLVM(std::ostream &os);
        mlir::ModuleOp GetModule();
        mlir::Location GetLocation();
        std::shared_ptr<mlir::OpBuilder> GetBuilder();
        mlir::Type GetMLIRType(BackendMLIRType type);
        void PrintInt(mlir::Value value);
        void PrintChar(char c);
        std::string GetOperationFunc(size_t op, size_t data_type);

    
    protected:
        void setupPrintf();
        void createGlobalString(const char *str, const char *string_name);
        // Generates an MLIR int arithmetic function for a given arithmetic op (ADD, SUB, MUL, DIV) from VCalcParser.h
        void CreateIntArithmeticOperation(size_t op);

        // Generates an MLIR vector function for a given op from (ADD, SUB, MUL, DIV, LOQEQ, NLOQEQ, LESS, GREATER)  VCalcParser.h
        void CreateVectorOperationFunction(size_t op);

        // Generates an MLIR vector function which promotes an int to a vector
        void CreateIntToVectorFunction();

        void TestArithmeticInt(mlir::ValueRange args, mlir::LLVM::LLVMFuncOp func, mlir::Value formatStringPtr);

        mlir::MLIRContext context;
        mlir::ModuleOp module;
        std::shared_ptr<mlir::OpBuilder> builder;
        mlir::Location loc;

        // LLVM 
        llvm::LLVMContext llvm_context;
        std::unique_ptr<llvm::Module> llvm_module;

        // Types
        mlir::Type vector_type, int_type, ptr_type;

        // Constants
        mlir::Value const_one, const_zero;

        // Functions
        mlir::LLVM::LLVMFuncOp main_func;
        
        // Generates an MLIR func which returns an empty vector* with size arr_size
        mlir::Value GenerateVectorTypePtr(mlir::Value arr_size);

        void CreateCastBoolToInt();

        // Generates an MLIR vector function which prints a vector
        void CreatePrintVectorOperation();

        // Generates an MLIR int bool function for a given op (LOQEQ, NLOQEQ, LESS, GREATER) from VCalcParser.h
        void CreateIntBooleanOperation(size_t op);

        // Generates a int pointer which points an int with given value
        mlir::Value CreateIntPointer(mlir::Value value);

        // Generates an MLIR function which returns a vector* between a given int interval
        // Eg: 1..3->[1,2,3,4]*
        void CreateVectorRangeOperation();

        void TestVectorOp(mlir::ValueRange args, mlir::LLVM::LLVMFuncOp func);
        void TestIndex(mlir::ValueRange args);

        // Generates an MLIR function which prints a space when two integer values are equal
        // Useful for printing vectors
        void DeclarePrintIntSpace();

        // Generates an MLIR function which returns the value of a vector in a given index
        void CreateVectorIndexOperation();

        // Generates an MLIR global const int
        //
        // @param val The value of the global int
        // @param name The name of the global const
        mlir::LLVM::GlobalOp CreateGlobalInt(int val, const char *name);

        void CreateVectorIndexVectorOperation();

        void CreateVectorSizePromotionFunction();

        // Generates an MLIR function used increasing vector size
        // If index is greater than or equal to the original vector size it sets the value at the vector to a provided value
        void CreateConditionalSetVectorFunc();

        void CreateVectorMatchSizeFunction();

        void SwitchAndFreeVector(mlir::Value vector_ptr, mlir::Value copy_vector_ptr);
};

// Used to generate MLIR functions which operate on vectors
class VectorLoopMLIRFunction {
    public:
        VectorLoopMLIRFunction();
        // MLIR indexed for loop template which calls PreHeaderFunc before entering the loop
        // and calls LoopFunc at each iteration
        void Generate(BackEnd* backEnd, mlir::LLVM::LLVMFuncOp func, mlir::Value vector_ptr,mlir::Value upper_bound);
    protected:
        // Called prior to MLIR loop
        virtual void PreHeaderFunc(BackEnd* backend);
        // Called at each iteration of the MLIR loop
        virtual void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,
                            mlir::LLVM::LLVMFuncOp func);
};

// Generates a MLIR loop which assigns a constant value at each index of a vector
class IntVectorPromotionFunction : public VectorLoopMLIRFunction{
    public:
        explicit IntVectorPromotionFunction(mlir::Value const_value) {
            this->const_value = const_value;
        }
    protected:
        void PreHeaderFunc(BackEnd* backend) override;
        void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,
                    mlir::LLVM::LLVMFuncOp func) override;
    private:
        mlir::Value const_value;
        mlir::LLVM::LLVMFuncOp printfFunc;
};

// Generates a MLIR loop which assigns index+lower_bound at each index of a vector
class RangeVectorFunction : public VectorLoopMLIRFunction {
    public:
        explicit RangeVectorFunction(mlir::Value lower_bound) {
            this->lower_bound = lower_bound;
        }

    protected:
        void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,
                    mlir::LLVM::LLVMFuncOp func) override;
    private:
        mlir::Value lower_bound;
};

// Generates a MLIR loop which performs an op (ADD, SUB, MUL, DIV) at each iteration
class VectorArithmeticOperationFunction : public VectorLoopMLIRFunction {
    public:
        explicit VectorArithmeticOperationFunction(size_t op, mlir::Value arr0_ptr, mlir::Value arr1_ptr) {
            this->op = op;
            this->arr0_ptr = arr0_ptr;
            this->arr1_ptr = arr1_ptr;
        }

    protected:
        void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,mlir::LLVM::LLVMFuncOp func) override;
    private:
        size_t op;
        mlir::Value arr0_ptr;
        mlir::Value arr1_ptr;
};

// Generates a MLIR loop which prints the value of a vector at each iteration
class VectorPrintFunction : public VectorLoopMLIRFunction {
    protected:
        void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,mlir::LLVM::LLVMFuncOp func) override;
    private:
        size_t op;
        mlir::Value arr0_ptr;
        mlir::Value arr1_ptr;
};

// Generates a MLIR loop which performs an op (LOGEQ, NLOGEQ, LESS, GREATER) at each iteration
class VectorBooleanOperationFunction : public VectorLoopMLIRFunction {
    public:
        explicit VectorBooleanOperationFunction(size_t op, mlir::Value arr0_ptr, mlir::Value arr1_ptr) {
            this->op = op;
            this->arr0_ptr = arr0_ptr;
            this->arr1_ptr = arr1_ptr;
        }

    protected:
        void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,mlir::LLVM::LLVMFuncOp func) override;

        void PreHeaderFunc(BackEnd *backend) override;

    private:
        mlir::LLVM::LLVMFuncOp bool_int_func;
        size_t op;
        mlir::Value arr0_ptr;
        mlir::Value arr1_ptr;
};

// Generates a MLIR loop which copys the values from copy_arr_ptr into arr_ptr if i is less than old_size
// Otherwise sets the copy_arr_ptr[i] to default value
class IncreaseVectorSizeMLIRFunction : public VectorLoopMLIRFunction {
    public:
        explicit IncreaseVectorSizeMLIRFunction(mlir::Value copy_arr_ptr, mlir::Value old_size, mlir::Value default_value) {
            this->copy_arr_ptr = copy_arr_ptr;
            this->old_size = old_size;
            this->default_value = default_value;
        }

    protected:
        void LoopFunc(BackEnd *backend, mlir::Value i_value, mlir::Value arr_ptr, mlir::Value arr_size,mlir::LLVM::LLVMFuncOp func) override;
        void PreHeaderFunc(BackEnd *backend) override;

    private:
        mlir::Value copy_arr_ptr;
        mlir::Value old_size;
        mlir::Value default_value;
        mlir::LLVM::LLVMFuncOp cond_set_func;
};