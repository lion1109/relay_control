idf_component_register(SRCS "test_relay_control.cpp"
                       INCLUDE_DIRS "../main"
                       REQUIRES unity build)

#if ( UNIT_TEST )
#    add_compile_definitions(UNIT_TEST)
#endif()