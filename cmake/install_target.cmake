function(install_target_and_headers ns name)
if(NOT CBUILDKIT_NOINSTALL)
set(target_name ${ns}-${name})

install(FILES
${ARGN}
DESTINATION include)

install(TARGETS ${target_name}
    EXPORT ${target_name}-targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin)

install(EXPORT ${target_name}-targets
    FILE ${target_name}-targets.cmake
    NAMESPACE ${ns}::
    DESTINATION lib/cmake/${ns})
endif()
endfunction()

function(install_cfgpkg ns content)
    if(NOT CBUILDKIT_NOINSTALL)
    set(config_file "${CMAKE_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
    file(WRITE "${config_file}" ${content})
    install(FILES "${config_file}" DESTINATION lib/cmake/${ns})
    endif()
endfunction()