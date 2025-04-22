file(GLOB RECK_SOURCES "*.cpp")

add_compile_options("-fPIC")
add_library(reck STATIC ${RECK_SOURCES})

add_library(reck_shared SHARED $<TARGET_OBJECTS:reck>)
set_target_properties(reck_shared PROPERTIES OUTPUT_NAME reck)