add_library(Concurrency INTERFACE)

target_include_directories(Concurrency
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}  
        ${PROJECT_ROOT}/CHATCBBCommon/LockFreeMPSCLogger      
        ${PROJECT_ROOT}/CHATCBBThirdPartyDepends/include
)

target_link_libraries(Concurrency
    INTERFACE
        CHATCBBThirdPartyDepends  
        LockFreeMPSCLogger
)
