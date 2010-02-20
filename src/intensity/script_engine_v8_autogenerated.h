
// i
#define V8_FUNC_i(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        , wrapped_code);


// s
#define V8_FUNC_s(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        , wrapped_code);


// d
#define V8_FUNC_d(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        , wrapped_code);


// o
#define V8_FUNC_o(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        , wrapped_code);


// ii
#define V8_FUNC_ii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        , wrapped_code);


// is
#define V8_FUNC_is(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        , wrapped_code);


// ss
#define V8_FUNC_ss(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        , wrapped_code);


// sd
#define V8_FUNC_sd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        , wrapped_code);


// si
#define V8_FUNC_si(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        int arg2 = args[1]->IntegerValue(); \
        , wrapped_code);


// oi
#define V8_FUNC_oi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        int arg2 = args[1]->IntegerValue(); \
        , wrapped_code);


// ob
#define V8_FUNC_ob(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        bool arg2 = args[1]->BooleanValue(); \
        , wrapped_code);


// os
#define V8_FUNC_os(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        , wrapped_code);


// od
#define V8_FUNC_od(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        , wrapped_code);


// dd
#define V8_FUNC_dd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        , wrapped_code);


// iis
#define V8_FUNC_iis(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        std::string _arg3 = *(v8::String::Utf8Value(args[2])); const char* arg3 = _arg3.c_str(); \
        , wrapped_code);


// iii
#define V8_FUNC_iii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        , wrapped_code);


// iid
#define V8_FUNC_iid(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        , wrapped_code);


// ddd
#define V8_FUNC_ddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        , wrapped_code);


// sss
#define V8_FUNC_sss(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        std::string _arg3 = *(v8::String::Utf8Value(args[2])); const char* arg3 = _arg3.c_str(); \
        , wrapped_code);


// oddd
#define V8_FUNC_oddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        , wrapped_code);


// dddd
#define V8_FUNC_dddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        , wrapped_code);


// iddd
#define V8_FUNC_iddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        , wrapped_code);


// iiss
#define V8_FUNC_iiss(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        std::string _arg3 = *(v8::String::Utf8Value(args[2])); const char* arg3 = _arg3.c_str(); \
        std::string _arg4 = *(v8::String::Utf8Value(args[3])); const char* arg4 = _arg4.c_str(); \
        , wrapped_code);


// iiis
#define V8_FUNC_iiis(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        std::string _arg4 = *(v8::String::Utf8Value(args[3])); const char* arg4 = _arg4.c_str(); \
        , wrapped_code);


// ssdd
#define V8_FUNC_ssdd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        , wrapped_code);


// sdddi
#define V8_FUNC_sdddi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        int arg5 = args[4]->IntegerValue(); \
        , wrapped_code);


// sssdd
#define V8_FUNC_sssdd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        std::string _arg3 = *(v8::String::Utf8Value(args[2])); const char* arg3 = _arg3.c_str(); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        , wrapped_code);


// ddddi
#define V8_FUNC_ddddi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        int arg5 = args[4]->IntegerValue(); \
        , wrapped_code);


// sdddd
#define V8_FUNC_sdddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        , wrapped_code);


// iiiss
#define V8_FUNC_iiiss(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        std::string _arg4 = *(v8::String::Utf8Value(args[3])); const char* arg4 = _arg4.c_str(); \
        std::string _arg5 = *(v8::String::Utf8Value(args[4])); const char* arg5 = _arg5.c_str(); \
        , wrapped_code);


// iiisi
#define V8_FUNC_iiisi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        std::string _arg4 = *(v8::String::Utf8Value(args[3])); const char* arg4 = _arg4.c_str(); \
        int arg5 = args[4]->IntegerValue(); \
        , wrapped_code);


// iiiii
#define V8_FUNC_iiiii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        int arg4 = args[3]->IntegerValue(); \
        int arg5 = args[4]->IntegerValue(); \
        , wrapped_code);


// idddd
#define V8_FUNC_idddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        , wrapped_code);


// dddddd
#define V8_FUNC_dddddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        , wrapped_code);


// iidddi
#define V8_FUNC_iidddi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        int arg6 = args[5]->IntegerValue(); \
        , wrapped_code);


// iiiddd
#define V8_FUNC_iiiddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        , wrapped_code);


// ddddii
#define V8_FUNC_ddddii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        int arg5 = args[4]->IntegerValue(); \
        int arg6 = args[5]->IntegerValue(); \
        , wrapped_code);


// idddsi
#define V8_FUNC_idddsi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        std::string _arg5 = *(v8::String::Utf8Value(args[4])); const char* arg5 = _arg5.c_str(); \
        int arg6 = args[5]->IntegerValue(); \
        , wrapped_code);


// ssiiid
#define V8_FUNC_ssiiid(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        int arg3 = args[2]->IntegerValue(); \
        int arg4 = args[3]->IntegerValue(); \
        int arg5 = args[4]->IntegerValue(); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        , wrapped_code);


// ddddddd
#define V8_FUNC_ddddddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        , wrapped_code);


// iiiiddd
#define V8_FUNC_iiiiddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        int arg4 = args[3]->IntegerValue(); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        , wrapped_code);


// iiddddd
#define V8_FUNC_iiddddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        , wrapped_code);


// ddddddii
#define V8_FUNC_ddddddii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        int arg7 = args[6]->IntegerValue(); \
        int arg8 = args[7]->IntegerValue(); \
        , wrapped_code);


// ddddiiid
#define V8_FUNC_ddddiiid(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        int arg5 = args[4]->IntegerValue(); \
        int arg6 = args[5]->IntegerValue(); \
        int arg7 = args[6]->IntegerValue(); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        , wrapped_code);


// ssiiidi
#define V8_FUNC_ssiiidi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        std::string _arg1 = *(v8::String::Utf8Value(args[0])); const char* arg1 = _arg1.c_str(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        int arg3 = args[2]->IntegerValue(); \
        int arg4 = args[3]->IntegerValue(); \
        int arg5 = args[4]->IntegerValue(); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        int arg7 = args[6]->IntegerValue(); \
        , wrapped_code);


// iidddddd
#define V8_FUNC_iidddddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        , wrapped_code);


// ddddddiii
#define V8_FUNC_ddddddiii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        int arg7 = args[6]->IntegerValue(); \
        int arg8 = args[7]->IntegerValue(); \
        int arg9 = args[8]->IntegerValue(); \
        , wrapped_code);


// oidddiiii
#define V8_FUNC_oidddiiii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        int arg2 = args[1]->IntegerValue(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        int arg6 = args[5]->IntegerValue(); \
        int arg7 = args[6]->IntegerValue(); \
        int arg8 = args[7]->IntegerValue(); \
        int arg9 = args[8]->IntegerValue(); \
        , wrapped_code);


// idddidddi
#define V8_FUNC_idddidddi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        int arg5 = args[4]->IntegerValue(); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        int arg9 = args[8]->IntegerValue(); \
        , wrapped_code);


// dddsiiidi
#define V8_FUNC_dddsiiidi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        std::string _arg4 = *(v8::String::Utf8Value(args[3])); const char* arg4 = _arg4.c_str(); \
        int arg5 = args[4]->IntegerValue(); \
        int arg6 = args[5]->IntegerValue(); \
        int arg7 = args[6]->IntegerValue(); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        int arg9 = args[8]->IntegerValue(); \
        , wrapped_code);


// iiidddidii
#define V8_FUNC_iiidddidii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        int arg3 = args[2]->IntegerValue(); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        int arg7 = args[6]->IntegerValue(); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        int arg9 = args[8]->IntegerValue(); \
        int arg10 = args[9]->IntegerValue(); \
        , wrapped_code);


// ddddddiiid
#define V8_FUNC_ddddddiiid(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        int arg7 = args[6]->IntegerValue(); \
        int arg8 = args[7]->IntegerValue(); \
        int arg9 = args[8]->IntegerValue(); \
        double arg10 = args[9]->NumberValue(); if (ISNAN(arg10)) RAISE_SCRIPT_ERROR(isNAN failed on argument 9 in #new_func); \
        , wrapped_code);


// osiddddddii
#define V8_FUNC_osiddddddii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        int arg3 = args[2]->IntegerValue(); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        double arg9 = args[8]->NumberValue(); if (ISNAN(arg9)) RAISE_SCRIPT_ERROR(isNAN failed on argument 8 in #new_func); \
        int arg10 = args[9]->IntegerValue(); \
        int arg11 = args[10]->IntegerValue(); \
        , wrapped_code);


// iissdddiiii
#define V8_FUNC_iissdddiiii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        std::string _arg3 = *(v8::String::Utf8Value(args[2])); const char* arg3 = _arg3.c_str(); \
        std::string _arg4 = *(v8::String::Utf8Value(args[3])); const char* arg4 = _arg4.c_str(); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        int arg8 = args[7]->IntegerValue(); \
        int arg9 = args[8]->IntegerValue(); \
        int arg10 = args[9]->IntegerValue(); \
        int arg11 = args[10]->IntegerValue(); \
        , wrapped_code);


// iiddddddidi
#define V8_FUNC_iiddddddidi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        int arg2 = args[1]->IntegerValue(); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        int arg9 = args[8]->IntegerValue(); \
        double arg10 = args[9]->NumberValue(); if (ISNAN(arg10)) RAISE_SCRIPT_ERROR(isNAN failed on argument 9 in #new_func); \
        int arg11 = args[10]->IntegerValue(); \
        , wrapped_code);


// idddddddiiii
#define V8_FUNC_idddddddiiii(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        int arg9 = args[8]->IntegerValue(); \
        int arg10 = args[9]->IntegerValue(); \
        int arg11 = args[10]->IntegerValue(); \
        int arg12 = args[11]->IntegerValue(); \
        , wrapped_code);


// idddddiidddi
#define V8_FUNC_idddddiidddi(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        int arg1 = args[0]->IntegerValue(); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        int arg7 = args[6]->IntegerValue(); \
        int arg8 = args[7]->IntegerValue(); \
        double arg9 = args[8]->NumberValue(); if (ISNAN(arg9)) RAISE_SCRIPT_ERROR(isNAN failed on argument 8 in #new_func); \
        double arg10 = args[9]->NumberValue(); if (ISNAN(arg10)) RAISE_SCRIPT_ERROR(isNAN failed on argument 9 in #new_func); \
        double arg11 = args[10]->NumberValue(); if (ISNAN(arg11)) RAISE_SCRIPT_ERROR(isNAN failed on argument 10 in #new_func); \
        int arg12 = args[11]->IntegerValue(); \
        , wrapped_code);


// dddddddiiidddd
#define V8_FUNC_dddddddiiidddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        double arg1 = args[0]->NumberValue(); if (ISNAN(arg1)) RAISE_SCRIPT_ERROR(isNAN failed on argument 0 in #new_func); \
        double arg2 = args[1]->NumberValue(); if (ISNAN(arg2)) RAISE_SCRIPT_ERROR(isNAN failed on argument 1 in #new_func); \
        double arg3 = args[2]->NumberValue(); if (ISNAN(arg3)) RAISE_SCRIPT_ERROR(isNAN failed on argument 2 in #new_func); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        int arg8 = args[7]->IntegerValue(); \
        int arg9 = args[8]->IntegerValue(); \
        int arg10 = args[9]->IntegerValue(); \
        double arg11 = args[10]->NumberValue(); if (ISNAN(arg11)) RAISE_SCRIPT_ERROR(isNAN failed on argument 10 in #new_func); \
        double arg12 = args[11]->NumberValue(); if (ISNAN(arg12)) RAISE_SCRIPT_ERROR(isNAN failed on argument 11 in #new_func); \
        double arg13 = args[12]->NumberValue(); if (ISNAN(arg13)) RAISE_SCRIPT_ERROR(isNAN failed on argument 12 in #new_func); \
        double arg14 = args[13]->NumberValue(); if (ISNAN(arg14)) RAISE_SCRIPT_ERROR(isNAN failed on argument 13 in #new_func); \
        , wrapped_code);


// osiddddddiidddd
#define V8_FUNC_osiddddddiidddd(new_func, wrapped_code) \
    V8_FUNC_GEN(new_func, \
        Handle<Object> arg1 = args[0]->ToObject(); \
        std::string _arg2 = *(v8::String::Utf8Value(args[1])); const char* arg2 = _arg2.c_str(); \
        int arg3 = args[2]->IntegerValue(); \
        double arg4 = args[3]->NumberValue(); if (ISNAN(arg4)) RAISE_SCRIPT_ERROR(isNAN failed on argument 3 in #new_func); \
        double arg5 = args[4]->NumberValue(); if (ISNAN(arg5)) RAISE_SCRIPT_ERROR(isNAN failed on argument 4 in #new_func); \
        double arg6 = args[5]->NumberValue(); if (ISNAN(arg6)) RAISE_SCRIPT_ERROR(isNAN failed on argument 5 in #new_func); \
        double arg7 = args[6]->NumberValue(); if (ISNAN(arg7)) RAISE_SCRIPT_ERROR(isNAN failed on argument 6 in #new_func); \
        double arg8 = args[7]->NumberValue(); if (ISNAN(arg8)) RAISE_SCRIPT_ERROR(isNAN failed on argument 7 in #new_func); \
        double arg9 = args[8]->NumberValue(); if (ISNAN(arg9)) RAISE_SCRIPT_ERROR(isNAN failed on argument 8 in #new_func); \
        int arg10 = args[9]->IntegerValue(); \
        int arg11 = args[10]->IntegerValue(); \
        double arg12 = args[11]->NumberValue(); if (ISNAN(arg12)) RAISE_SCRIPT_ERROR(isNAN failed on argument 11 in #new_func); \
        double arg13 = args[12]->NumberValue(); if (ISNAN(arg13)) RAISE_SCRIPT_ERROR(isNAN failed on argument 12 in #new_func); \
        double arg14 = args[13]->NumberValue(); if (ISNAN(arg14)) RAISE_SCRIPT_ERROR(isNAN failed on argument 13 in #new_func); \
        double arg15 = args[14]->NumberValue(); if (ISNAN(arg15)) RAISE_SCRIPT_ERROR(isNAN failed on argument 14 in #new_func); \
        , wrapped_code);

