add_subdirectory(FreeRTOSTasks)

add_library(app_main STATIC)

include_directories("../")

target_include_directories(app_main PUBLIC .)
target_sources(app_main PRIVATE app_main.cpp)

target_link_libraries(app_main PRIVATE freertos_kernel freertos_tasks platform stm32cubemx log_common)
