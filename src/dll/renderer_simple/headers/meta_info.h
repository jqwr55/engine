enum MetaType {meta_type_u32,meta_type_i32,};
struct StructMemberInfoType {MetaType type;const char* name;u32 offset;};StructMemberInfoType meta_info_of_MeshInternal[] =
{
{meta_type_u32, "vertexCount",__builtin_offsetof(MeshInternal,vertexCount)},
{meta_type_u32, "vertexOffset",__builtin_offsetof(MeshInternal,vertexOffset)},
{meta_type_u32, "indexCount",__builtin_offsetof(MeshInternal,indexCount)},
{meta_type_u32, "indexOffset",__builtin_offsetof(MeshInternal,indexOffset)},
{meta_type_u32, "shadowVertexCount",__builtin_offsetof(MeshInternal,shadowVertexCount)},
{meta_type_u32, "shadwoVertexOffset",__builtin_offsetof(MeshInternal,shadwoVertexOffset)},
{meta_type_u32, "shadowIndexCount",__builtin_offsetof(MeshInternal,shadowIndexCount)},
{meta_type_u32, "shadowIndexOffset",__builtin_offsetof(MeshInternal,shadowIndexOffset)},

};
StructMemberInfoType meta_info_of_RendererSettingsState[] =
{
{meta_type_i32, "callbackCount",__builtin_offsetof(RendererSettingsState,callbackCount)},
{meta_type_u32, "windowWidth",__builtin_offsetof(RendererSettingsState,windowWidth)},
{meta_type_u32, "windowHeight",__builtin_offsetof(RendererSettingsState,windowHeight)},

};

#define META_TYPE_CASE_DUMP(memberPtr,indentLevel,T,userPtr) {\
       case meta_type_u32: T<u32>::Func(indentLevel,userPtr, *((u32*)memberPtr)); break;\
       case meta_type_i32: T<i32>::Func(indentLevel,userPtr, *((i32*)memberPtr)); break;\
}