#include <iostream>
#include <stdio.h>
#include <fstream>
#include <filesystem>
#include <thread>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

static_assert(sizeof(f32) == 4);
static_assert(sizeof(f64) == 8);

typedef i8 byte;

constexpr u64 KILO_BYTES = 1000;
constexpr u64 MEGA_BYTES = 1000 * KILO_BYTES;
constexpr u64 GIGA_BYTES = 1000 * MEGA_BYTES;
constexpr u64 TERA_BYTES = 1000 * GIGA_BYTES;

constexpr u32 MILI_SEC = 1000;
constexpr u32 MICRO_SEC = 1000 * MILI_SEC; 
constexpr u32 NANO_SEC = 1000 * MICRO_SEC;

#define LOGASSERT(x , y ) if( !(x) ) {std::cout << #x << " " << y << " " << __FILE__ << " " << __LINE__ << std::endl; __builtin_trap(); }
#define ASSERT(x) if( !(x) ) __builtin_trap();

char* ReadFileTerminated(char* fileName) {

    char* sourceString = nullptr;
    FILE* file = fopen(fileName ,"r");
    if(file) {

        fseek(file , 0, SEEK_END);
        u32 size = ftell(file);
        fseek(file ,0, SEEK_SET);


        sourceString = (char*)malloc(size + 1);
        fread(sourceString , size , 1 , file);
        sourceString[size] = 0;

    }
    fclose(file);

    return sourceString;
}

enum TokenType {
    TOKEN_IDENTIFIER,
    TOKEN_OPEN_PAREN,
    TOKEN_CLOSE_PAREN,
    TOKEN_COLON,
    TOKEN_STRING,
    TOKEN_SEMICOLON,
    TOKEN_ASTERISK,
    TOKEN_OPEN_BRACKET,
    TOKEN_CLOSE_BRACKET,
    TOKEN_OPEN_BRACES,
    TOKEN_CLOSE_BRACES,
    TOKEN_POUND,
    TOKEN_EQUAl,
    TOKEN_NUMBER,

    TOKEN_UNKNOWN,
    TOKEN_EOS,
};
struct Token {
    char* text;
    u64 lenght;
    TokenType type;
};
struct Tokenizer {
    char* at;
};

bool IsWhiteSpace(char c) {
    return  (c == ' ') ||
            (c == '\n') ||
            (c == '\t') ||
            (c == '\r');
}

u32 GetLineNumber(char* source ,Tokenizer* tokenizer) {
    u32 c = 1;
    while( source != tokenizer->at ) {
        if( *source == '\n' ) c++;
        source++;
    }
    return c;
}

bool IsAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}
bool IsNumeric(char c) {
    return (c >= '0' && c <= '9');
}
bool TokenEquals(Token t , const char* match) {

    const char* m = match;
    for(u32 i = 0; i < t.lenght ; i++) {

        if(*m == 0 || t.text[i] != *m ) {
            return false;
        }

        m++;
    }
    return (*m == 0);
}
bool TokensEquals(Token t0 , Token t1) {

    if(t0.lenght != t1.lenght) {
        return false;
    }

    for(u32 i = 0; i < t0.lenght ; i++) {
        if( t0.text[i] != t1.text[i] ) {
            return false;
        }
    }

    return true;
}

constexpr const char* primite_types[11] = {
    "bool",

    "i8",
    "i16",
    "i32",
    "i64",

    "u8",
    "u16",
    "u32",
    "u64",

    "f32",
    "f64",
};

struct MetaType {
    Token token;
    bool primitive;
};

struct Vector {
    MetaType* meta_types = nullptr;
    u32 meta_types_count = 0;
    u32 meta_types_cap = 0;    
};

Vector meta_types;
Vector missing_meta_types;

bool IsPrimitive(Token token) {

    for(u32 i = 0; i < 11 ; i++) {
        if(TokenEquals(token , primite_types[i])) {
            return true;
        }
    }
    return false;
}

void PushToken(Token token) {

    bool primitive = IsPrimitive(token);
    if(!primitive) {

        for(u32 i = 0; i < missing_meta_types.meta_types_count ; i++) {
            if( TokensEquals(missing_meta_types.meta_types[i].token , token)) {
                return;
            }
        }

        if( missing_meta_types.meta_types_cap < missing_meta_types.meta_types_count + 1 ) {

            missing_meta_types.meta_types_cap++;
            missing_meta_types.meta_types_cap *= 2;
            MetaType* tmp = (MetaType*)malloc( missing_meta_types.meta_types_cap * sizeof(MetaType) );

            for(u32 i = 0; i < missing_meta_types.meta_types_count ; i++) {
                tmp[i] = missing_meta_types.meta_types[i];
            }
            free(missing_meta_types.meta_types);
            missing_meta_types.meta_types = tmp;
        }

        missing_meta_types.meta_types[missing_meta_types.meta_types_count++] = {token, false};
    }

    for(u32 i = 0; i < meta_types.meta_types_count ; i++) {
        if( TokensEquals(meta_types.meta_types[i].token , token)) {
            return;
        }
    }

    if( meta_types.meta_types_cap < meta_types.meta_types_count + 1 ) {

        meta_types.meta_types_cap++;
        meta_types.meta_types_cap *= 2;
        MetaType* tmp = (MetaType*)malloc( meta_types.meta_types_cap * sizeof(MetaType) );

        for(u32 i = 0; i < meta_types.meta_types_count ; i++) {
            tmp[i] = meta_types.meta_types[i];
        }
        free(meta_types.meta_types);
        meta_types.meta_types = tmp;
    }
    meta_types.meta_types[meta_types. meta_types_count++] = {token, primitive};


}
void FreeAll() {
    free(meta_types.meta_types);
    meta_types.meta_types = nullptr;
    meta_types.meta_types_cap = 0;
    meta_types.meta_types_count = 0;

    free(missing_meta_types.meta_types);
    missing_meta_types.meta_types = nullptr;
    missing_meta_types.meta_types_cap = 0;
    missing_meta_types.meta_types_count = 0;
}

void EatWhiteSpace(Tokenizer* tokenizer) {

    while(true) {
        if( IsWhiteSpace(tokenizer->at[0])  ) {
            tokenizer->at++;
        }
        else if( tokenizer->at[0] == '/' && tokenizer->at[1] == '/' ) {
            tokenizer->at += 2;
            while( !(tokenizer->at[0] == '\n' )) ++tokenizer->at;
            tokenizer->at += 2;
        }
        else if( tokenizer->at[0] == '/' && tokenizer->at[1] == '*' ) {
            tokenizer->at += 2;
            while( !(tokenizer->at[0] == '*' && tokenizer->at[1] == '/') ) ++tokenizer->at;
            tokenizer->at += 2;
        }
        else {
            break;
        }
    }

}

Token GetToken(Tokenizer* tokenizer) {

    EatWhiteSpace(tokenizer);

    Token token{};
    token.lenght = 1;

    char c = tokenizer->at[0];
    token.text = tokenizer->at++;

    switch (c) {

        case '\0': {token.type = TOKEN_EOS;             } break;
        case '(' : {token.type = TOKEN_OPEN_PAREN;      } break;
        case ')' : {token.type = TOKEN_CLOSE_PAREN;     } break;
        case ':' : {token.type = TOKEN_COLON;           } break;
        case ';' : {token.type = TOKEN_SEMICOLON;       } break;
        case '*' : {token.type = TOKEN_ASTERISK;        } break;
        case '[' : {token.type = TOKEN_OPEN_BRACKET;    } break;
        case ']' : {token.type = TOKEN_CLOSE_BRACKET;   } break;
        case '{' : {token.type = TOKEN_OPEN_BRACES;     } break;
        case '}' : {token.type = TOKEN_CLOSE_BRACES;    } break;
        case '=' : {token.type = TOKEN_EQUAl;           } break;

        case '"': 
            token.text = tokenizer->at;
            token.type = TOKEN_STRING;

            while(  tokenizer->at[0] && tokenizer->at[0] != '"') {
                
                if(tokenizer->at[0] == '\\' && tokenizer->at[1] ) {
                    ++tokenizer->at;
                }
                ++tokenizer->at;
            }
            token.lenght = tokenizer->at - token.text;
            if( tokenizer->at[0] == '"' ) ++tokenizer->at;

        break;

        default:
            if( IsAlpha(c) ) {
                token.type = TOKEN_IDENTIFIER;
                while(IsAlpha(tokenizer->at[0]) || IsNumeric(tokenizer->at[0]) || tokenizer->at[0] == '_' ) ++tokenizer->at;
                token.lenght = tokenizer->at - token.text;
            }
#if 1
            else if(IsNumeric(c)) {
                token.type = TOKEN_NUMBER;
                while(IsNumeric(tokenizer->at[0]) ) ++tokenizer->at;
                token.lenght = tokenizer->at - token.text;
            }
#endif
            else {
                token.type = TOKEN_UNKNOWN;
            }
            break;
    }

    return token;
}


Token ParseStructMember2(char* source , Tokenizer* tokenizer ,Token structName , std::ofstream& file ) {

    bool valid = true;
    bool parsing = true;
    u32 identiferCount = 0;

    Token t;
    Token type;
    Token name;

    while(parsing) {

        t = GetToken(tokenizer);
        
        switch (t.type)
        {
        case TOKEN_IDENTIFIER:

            if(identiferCount == 0) {
                type = t;
            }
            else if(identiferCount == 1) {
                name = t;
            }

            identiferCount++;
            break;

        case TOKEN_EQUAl:
            break;
        case TOKEN_NUMBER:
            break;
        case TOKEN_SEMICOLON:
            if(identiferCount < 2) {
                valid = false;
            }
            parsing = false;
            break;

        case TOKEN_CLOSE_BRACES:
            if(identiferCount < 2) {
                valid = false;
            }
            parsing = false;
            break;

        case TOKEN_EOS:
            if(identiferCount < 2) {
                valid = false;
            }
            parsing = false;
            break;


        default:
            valid = false;
            break;
        }

    }

    if(valid) {
        file << "{meta_type_";
        
#if 0
        for(u32 i = 0; i < type.lenght; i++) {
            file << type.text[i];
        }
        file << ", \"";
        for(u32 i = 0; i < name.lenght;i++) {
            file << name.text[i];
        }
        file << "\",__builtin_offsetof(";
        for(u32 i = 0; i < structName.lenght;i++) {
            file << structName.text[i];
        }
        file << ",";
        for(u32 i = 0; i < name.lenght;i++) {
            file << name.text[i];
        }
#else
        file.write(type.text , type.lenght);
        file << ", \"";
        file.write(name.text , name.lenght);
        file << "\",__builtin_offsetof(";
        file.write(structName.text , structName.lenght);
        file << ",";
        file.write(name.text , name.lenght);

#endif
        file << ")}," << std::endl;
        PushToken(type);

    }

    return t;
}

void ParseStructMember(char* source , Tokenizer* tokenizer , Token* t, std::ofstream& file , Token structName) {

    bool valid = true;
    bool parsing = true;
    bool s = true;
    bool name = false;

    std::string memberName;
    std::string line = "{";

    while(parsing) {

        *t = GetToken(tokenizer);

        switch (t->type) {

        case TOKEN_IDENTIFIER:
            
            if(!name) {
                line += "meta_type_";
            }
            else {
                line += "\"";
            }

            for(u32 i = 0; i < t->lenght ; i++) {
                line += t->text[i];
            }

            if(!name) {
                line += ", ";
                name = true;
            }
            else {
                line += "\" , __builtin_offsetof(";

                for(u32 i = 0; i < structName.lenght ; i++) {
                    line += structName.text[i];
                }
                line += ",";

                for(u32 i = 0; i < t->lenght ; i++) {
                    line += t->text[i];
                }

            }



            break;
        case TOKEN_OPEN_BRACKET:
            if(s) {
                valid = false;
            }
            break;
        case TOKEN_CLOSE_BRACKET:
            if(s) {
                valid = false;
            }
            break;
        case TOKEN_NUMBER:
            if(s) {
                valid = false;
            }
            break;
        case TOKEN_ASTERISK:
            if(s) {
                valid = false;
            }
            break;

        case TOKEN_EQUAl:
            s = false;
            break;

            
        case TOKEN_CLOSE_BRACES:
            valid = false;
            parsing = false;
            break;
        case TOKEN_SEMICOLON:
            parsing = false;
            break;
        case TOKEN_EOS:
            valid = false;
            parsing = false;
        break;
        
        }

    }
    if(valid) {
        file << line << ")}," << std::endl;
    }

}
void ParseStruct(char* source , Tokenizer* tokenizer , std::ofstream& file) {

    Token structName = GetToken(tokenizer);
    if( structName.type == TOKEN_IDENTIFIER ) {
        file.write(structName.text , structName.lenght) << "[] =" << std::endl << "{" << std::endl;
    }
    else {
        u32 line = GetLineNumber(source , tokenizer);
        std::cerr << "ERROR: missing struct name  at line: " << line << std::endl;
    }
    Token t = GetToken(tokenizer);

    if(t.type == TOKEN_OPEN_BRACES) {

        while( t.type != TOKEN_CLOSE_BRACES && t.type != TOKEN_EOS ) {

#if 0            
            ParseStructMember(source , tokenizer , &t , file , structName);
            std::cout.write(t.text , t.lenght) << std::endl;
#else

            t = ParseStructMember2(source,  tokenizer , structName , file);
#endif

        }

        file << std::endl << "};" << std::endl;
    }
    else {
        u32 line = GetLineNumber(source , tokenizer);
        std::cerr << "ERROR: missing open braces  at line: " << line << std::endl;
    }
}

void ParseIntrospectionParams(char* source , Tokenizer* tokenizer , std::ofstream& file) {

    Token t = GetToken(tokenizer);

    if(t.type == TOKEN_IDENTIFIER) {
        t = GetToken(tokenizer);
        if(t.type == TOKEN_CLOSE_PAREN) {

        }
        else {
            u32 line = GetLineNumber(source , tokenizer);
            std::cerr << "ERROR: close parentheses at line: " << line << std::endl;
        }
    }
    else {
        u32 line = GetLineNumber(source , tokenizer);
        std::cerr << "ERROR: missing identifier at line: " << line << std::endl;
    }
}

void ParseIntrospection(char* source , Tokenizer*  tokenizer, std::ofstream& file) {
    Token t = GetToken(tokenizer);

    if( t.type == TOKEN_OPEN_PAREN ) {
        ParseIntrospectionParams(source , tokenizer , file);

        t = GetToken(tokenizer);
        if(TokenEquals(t , "struct")) {

            file << "StructMemberInfoType" << " " << "meta_info_of_";
            ParseStruct(source , tokenizer, file);

        }
        else {
            u32 line = GetLineNumber(source , tokenizer);
            std::cerr << "ERROR: only structs are supported at line: " << line << std::endl;
        }
    }
    else {
        u32 line = GetLineNumber(source , tokenizer);
        std::cerr << "ERROR: missing open parentheses at line: " << line << std::endl;
    }
}



void GenerateIntrospectionData(char* inputPath ,char* outPut) {

    std::ofstream file(outPut);

    char* source = ReadFileTerminated(inputPath);
    Tokenizer tokenizer{source};
    bool parsing = true;

    while(parsing) {
        
        Token t = GetToken(&tokenizer);

        switch (t.type) {
            case TOKEN_EOS:
                parsing = false;
                break;

            case TOKEN_UNKNOWN: 
                break;
            case TOKEN_IDENTIFIER:
                if(TokenEquals(t , "introspect")) {
                    ParseIntrospection(source , &tokenizer , file);
                }
                else {
                }
                break;

            default:
                break;
        }

    }

    tokenizer.at = source;
    parsing = true;
    while(parsing) {
        
        Token t = GetToken(&tokenizer);

        switch (t.type) {
            case TOKEN_EOS:
                parsing = false;
                break;

            case TOKEN_UNKNOWN: 
                break;
            case TOKEN_IDENTIFIER:
                if(TokenEquals(t , "struct")) {

                    Tokenizer tmpTokenizer = tokenizer;

                    t = GetToken(&tokenizer);
                    if(t.type == TOKEN_IDENTIFIER) {

                        for(u32 i = 0; i < missing_meta_types.meta_types_count ; i++) {
                            if(missing_meta_types.meta_types[i].primitive == false && TokensEquals(t , missing_meta_types.meta_types[i].token ) ) {
                                file << "StructMemberInfoType" << " " << "meta_info_of_";
                                ParseStruct(source , &tmpTokenizer , file);
                                tokenizer = tmpTokenizer;
                                missing_meta_types.meta_types[i].primitive = true;
                                break;
                            }
                        }
                    }
                    else {
                        u32 line = GetLineNumber(source , &tokenizer);
                        std::cerr << "ERROR: missing struct name  at line: " << line << std::endl;
                    }
                }
                else {
                }
                break;

            default:
                break;
        }

    }

    for(u32 i = 0; i < missing_meta_types.meta_types_count ; i++) {
        if(missing_meta_types.meta_types[i].primitive == false) {
            std::cerr << "ERROR: missing type dependency: ";
            std::cerr.write(missing_meta_types.meta_types[i].token.text , missing_meta_types.meta_types[i].token.lenght) << std::endl;
        }
    }


    u32 size = file.tellp();
    file.close();



    char* outSource = (char*)malloc( size );

    std::ifstream inp(outPut);
    inp.read(outSource , size);
    inp.close();

    file.open(outPut);
    file << "enum MetaType {";
    for(u32 i = 0; i < meta_types.meta_types_count ; i++) {
        file << "meta_type_";
        file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);
        file << ",";
    }
    file << "};\nstruct StructMemberInfoType {MetaType type;const char* name;u32 offset;};";


    file.write(outSource , size);

    file << "\n#define META_TYPE_CASE_DUMP(memberPtr,indentLevel,T,userPtr) {\\\n";
    for(u32 i = 0; i < meta_types.meta_types_count ; i++) {
        file << "       case meta_type_";
        file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);

#if 0
        if(meta_types.meta_types[i].primitive) {
            file << ": std::cout << std::setw(7*indentLevel) << *(";
            file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);
            file << "*)(memberPtr) << std::endl; break; \\\n";

             T<i32>::Func(indentLevel , user , *((i32*)memberPtr) );
        }
        else {
            file << ": std::cout << std::endl; DumpStruct(memberPtr,meta_info_of_";
            file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);
            file << ", indentLevel + 1,functionPtr); break;\\\n";
        }
#else
        if(meta_types.meta_types[i].primitive) {
            file << ": T<";
            file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);
            file << ">::Func(indentLevel,userPtr, *((";
            file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);
            file << "*)memberPtr)); break;\\\n";
        }
#endif
        else {
            file << ": DumpStruct<T>(memberPtr,meta_info_of_";
            file.write(meta_types.meta_types[i].token.text , meta_types.meta_types[i].token.lenght);
            file << ", userPtr , indentLevel + 1); break;\\\n";
        }
    }
    file << "}";


    file.close();
    free(outSource);
    free(source);
    FreeAll();
}


int main(int argc , char** argv) {

    ASSERT(argc == 3);
    std::filesystem::file_time_type time;


    while(true) {

        if( time != std::filesystem::last_write_time(argv[1]) ) {
            time = std::filesystem::last_write_time(argv[1]);
            GenerateIntrospectionData(argv[1],argv[2]);

            std::cout << "Parsed" << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}