#ifndef _TYPE_H
#define _TYPE_H
namespace Type{

enum VCalcTypes{
    INT = 0,
    VECTOR = 1
};

class DataType{
    private:
        // represents the type
        VCalcTypes type;
    public:
        // constructor, just initializes type member
        DataType(VCalcTypes init_type) : type(init_type) {}

        // returns type member
        VCalcTypes GetType();

        // sets type member to new_type
        void SetType(VCalcTypes new_type);

        // checks if type member is same as comp_type
        bool IsType(VCalcTypes comp_type);

};

}
#endif