# file(GLOB EXTERN_FILES ../assets/binary/*)



idf_component_register(
    SRC_DIRS
        "."
        "task"
        "device"
        "resources"

    INCLUDE_DIRS
        "."
        "task"
        "device"
        "resources"

    EMBED_FILES ${EXTERN_FILES}
)

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format" "-Wno-error=narrowing")
target_compile_definitions(${COMPONENT_LIB} PUBLIC "-DLOG_LOCAL_LEVEL=ESP_LOG_VERBOSE")