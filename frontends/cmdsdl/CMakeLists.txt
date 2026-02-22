find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

add_executable(
  cmdsdl
  cmdsdl.cpp
  emulator/emulator.cpp
  input/input.cpp
  program/program.cpp
  settings/settings.cpp
)

target_include_directories(cmdsdl PRIVATE ${SDL2_INCLUDE_DIRS} ${ARES_LIBS_SOURCE_DIR})
target_compile_definitions(cmdsdl PRIVATE MIA_LIBRARY)

target_link_libraries(
  cmdsdl
  PRIVATE ares::ares ares::mia ares::nall sljit
          ${SDL2_STATIC_LIBRARIES}
)

set_target_properties(cmdsdl PROPERTIES OUTPUT_NAME aresdl)
