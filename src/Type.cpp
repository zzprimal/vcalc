#include "Type.h"

namespace Type{
    
VCalcTypes DataType::GetType(){
    return type;
}

void DataType::SetType(VCalcTypes new_type){
    type = new_type;
}

bool DataType::IsType(VCalcTypes comp_type){
    if (comp_type == type){
        return true;
    }
    return false;
}

}