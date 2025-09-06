add_library(ModuleController INTERFACE)

target_include_directories(ModuleController
    INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}  
        ${PROJECT_ROOT}/CHATCBBCommon/include          
        ${PROJECT_ROOT}/CHATCBBThirdPartyDepends/include
)

target_link_libraries(ModuleController
    INTERFACE
        CHATCBBThirdPartyDepends  
)
