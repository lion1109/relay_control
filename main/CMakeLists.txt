idf_component_register(
	SRCS "relay_control.cpp"
	INCLUDE_DIRS "."
        REQUIRES build driver esp_wifi esp_http_server
)

if(UNIT_TEST)
    add_compile_definitions(UNIT_TEST)
endif()
