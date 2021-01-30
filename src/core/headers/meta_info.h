enum MetaType {meta_type_u32,meta_type_T,meta_type_T0,};
struct StructMemberInfoType {MetaType type;const char* name;u32 offset;};StructMemberInfoType meta_info_of_T1[] =
{
{meta_type_u32, "b",__builtin_offsetof(T1,b)},
{meta_type_T, "c",__builtin_offsetof(T1,c)},

};
StructMemberInfoType meta_info_of_T[] =
{
{meta_type_T0, "w",__builtin_offsetof(T,w)},
{meta_type_u32, "a",__builtin_offsetof(T,a)},

};

#define META_TYPE_CASE_DUMP(memberPtr,indentLevel,T,userPtr) {\
       case meta_type_u32: T<u32>::Func(indentLevel,userPtr, *((u32*)memberPtr)); break;\
       case meta_type_T: DumpStruct<T>(memberPtr,meta_info_of_T, userPtr , indentLevel + 1); break;\
       case meta_type_T0: DumpStruct<T>(memberPtr,meta_info_of_T0, userPtr , indentLevel + 1); break;\
}