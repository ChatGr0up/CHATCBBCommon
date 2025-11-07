file(GLOB_RECURSE ALL_SRC *.cpp)
message(STATUS "${ALL_SRC}")
add_library(Async STATIC ${ALL_SRC})

target_include_directories(Async
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}  
        ${PROJECT_ROOT}/CHATCBBCommon/LockFreeMPSCLogger          
        ${PROJECT_ROOT}/CHATCBBCommon/CHATCommonDef          
        ${PROJECT_ROOT}/CHATCBBThirdPartyDepends/include
)

target_link_libraries(Async
    PRIVATE
        CHATCBBThirdPartyDepends  
        LockFreeMPSCLogger
)
