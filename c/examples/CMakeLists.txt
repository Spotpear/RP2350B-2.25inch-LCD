# Search for all source files in the current directory
# and save the names to the DIR_examples_SRCS variable
# NOTE: do NOT hardcode PICO_SDK_PATH here. The original author's path
# ("E:/pico/pico-sdk") does not exist on this machine and would break a clean
# reconfigure. The SDK path comes from the VS Code extension / environment.
aux_source_directory(. DIR_examples_SRCS)

include_directories(../lib/Config)
include_directories(../lib/GUI)
include_directories(../lib/Infrared)
include_directories(../lib/LCD)
include_directories(../lib/Icm20948)
include_directories(../lib/QMI8658)
include_directories(../lib/RGB)
include_directories(../lib/Fonts)
include_directories(../lib/FatFs/include)
include_directories(../lib/FatFs/sd_driver)
include_directories(../lib/FatFs/ff15/source)


# Generate link library
add_library(examples ${DIR_examples_SRCS})
target_link_libraries(examples PUBLIC Config LCD Infrared  Icm20948 QMI8658 GUI Fonts FatFs RGB)