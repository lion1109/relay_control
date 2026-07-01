idf_component_register(
	SRCS "relay_control.c"
	INCLUDE_DIRS "."
        REQUIRES build_utils driver nvs_flash esp_wifi esp_http_server
)

if(UNIT_TEST)
    add_compile_definitions(UNIT_TEST)
endif()
