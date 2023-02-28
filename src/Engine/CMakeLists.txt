set(public_files
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Engine/Input/ShortcutManager.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Engine/Types.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Engine/Ui/Point.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Engine/Ui/Rect.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Engine/Ui/Size.hpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/OpenLoco/Engine/World.hpp"
)

set(private_files
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Engine.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/Input/ShortcutManager.cpp"
)

loco_add_library(Engine STATIC
    PUBLIC_FILES
        ${public_files}
    PRIVATE_FILES
        ${private_files}
)

target_link_libraries(Engine 
    PUBLIC
        Core
        Console
        Utility
        Interop
        Math
)
