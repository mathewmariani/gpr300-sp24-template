file(GLOB_RECURSE ASSIGNMENT0_INC CONFIGURE_DEPENDS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.h *.hpp)

file(GLOB_RECURSE ASSIGNMENT0_SRC CONFIGURE_DEPENDS
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    *.c *.cpp)

#Copies this assignment0's asset folder to bin when it is built
add_custom_target(copyAssetsW0
    ALL COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/
    ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE}/assets/)

install(FILES ${ASSIGNMENT0_INC} DESTINATION include/worksession0)
add_executable(worksession0 ${ASSIGNMENT0_SRC} ${ASSIGNMENT0_INC})
target_link_libraries(worksession0 PUBLIC core IMGUI assimp)
target_include_directories(worksession0 PUBLIC ${CORE_INC_DIR} ${stb_INCLUDE_DIR})

#Trigger asset copy when assignment0 is built
add_dependencies(worksession0 copyAssetsW0)