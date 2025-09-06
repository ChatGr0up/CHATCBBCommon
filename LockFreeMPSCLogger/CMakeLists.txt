file(GLOB_RECURSE ALL_SRC *.cpp)
message(STATUS "${ALL_SRC}")
add_library(LockFreeMPSCLogger STATIC ${ALL_SRC})

target_include_directories(LockFreeMPSCLogger
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}  
        ${PROJECT_ROOT}/CHATCBBCommon/include          
        ${PROJECT_ROOT}/CHATCBBThirdPartyDepends/include
)

target_link_libraries(LockFreeMPSCLogger
    PRIVATE
        CHATCBBThirdPartyDepends  
)
