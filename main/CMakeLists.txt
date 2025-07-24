# main/CMakeLists.txt

idf_component_register(
    SRCS 
        "main.c"
        "max30102.c" 
        "i2c_config.c"
    INCLUDE_DIRS 
        "."
    REQUIRES 
        "driver"
        "nvs_flash"
        "log"
        "freertos"
)