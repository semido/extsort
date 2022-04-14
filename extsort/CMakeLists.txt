file(GLOB_RECURSE MY_H CONFIGURE_DEPENDS "*.hpp")
file(GLOB_RECURSE MY_SRC CONFIGURE_DEPENDS "*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Headers Files" FILES ${MY_H})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${MY_SRC})

target_sources(extsort PRIVATE ${MY_SRC} ${MY_H})
