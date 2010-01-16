
set(CMAKE_CXX_FLAGS "-O2")

add_library(boost_python src/converter/builtin_converters.cpp src/converter/type_id.cpp src/converter/from_python.cpp src/converter/registry.cpp src/converter/arg_to_python_base.cpp src/tuple.cpp src/module.cpp src/str.cpp src/object_operators.cpp src/slice.cpp src/wrapper.cpp src/exec.cpp src/list.cpp src/object_protocol.cpp src/numeric.cpp src/long.cpp src/dict.cpp src/import.cpp src/object/inheritance.cpp src/object/pickle_support.cpp src/object/function_doc_signature.cpp src/object/iterator.cpp src/object/function.cpp src/object/class.cpp src/object/life_support.cpp src/object/stl_iterator.cpp src/object/enum.cpp src/errors.cpp)

target_link_libraries(boost_python ${PYTHON_LIBRARIES})

