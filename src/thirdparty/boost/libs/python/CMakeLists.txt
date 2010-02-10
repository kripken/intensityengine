
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")
    set(CMAKE_CXX_FLAGS "-O2")
else(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(CMAKE_CXX_FLAGS "-O2")
else(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(STATUS "Windows for boost.python")
    set(CMAKE_CXX_FLAGS "/DWIN32 /O2 /EHsc")
endif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

endif(${CMAKE_SYSTEM_NAME} MATCHES "Linux" OR ${CMAKE_SYSTEM_NAME} MATCHES "BSD")

#set(Boost_Python_Library boost_python)
#if(${WIN32})
#    set(Boost_Python_Library libboost_python-vc90-mt-1_41) # Must match what Boost invents for its own name, see auto_link.hpp
#endif(${WIN32})
message(STATUS "Boost.Python libname(2): ${Boost_Python_Library}")

add_library(boost_python src/converter/builtin_converters.cpp src/converter/type_id.cpp src/converter/from_python.cpp src/converter/registry.cpp src/converter/arg_to_python_base.cpp src/tuple.cpp src/module.cpp src/str.cpp src/object_operators.cpp src/slice.cpp src/wrapper.cpp src/exec.cpp src/list.cpp src/object_protocol.cpp src/numeric.cpp src/long.cpp src/dict.cpp src/import.cpp src/object/inheritance.cpp src/object/pickle_support.cpp src/object/function_doc_signature.cpp src/object/iterator.cpp src/object/function.cpp src/object/class.cpp src/object/life_support.cpp src/object/stl_iterator.cpp src/object/enum.cpp src/errors.cpp)
target_link_libraries(boost_python ${PYTHON_LIBRARIES})

