idf_component_register(SRCS "storage_little.cpp" "storage.cpp"
                       PRIV_REQUIRES spiffs vfs littlefs
                       INCLUDE_DIRS ".")
