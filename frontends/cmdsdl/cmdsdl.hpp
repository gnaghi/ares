#include <ares/ares.hpp>
#include <nall/gdb/server.hpp>
#include <mia/mia.hpp>

inline auto operator!(const LoadResult& r) -> bool { return r.result != successful; }

//stubs for hiro types (cmdsdl has no GUI)
struct Menu {};
struct Timer {};
struct sTimer {
  sTimer() = default;
  sTimer(Timer) {}
  auto reset() -> void {}
  auto operator->() -> Timer* { static Timer t; return &t; }
};

#include <nall/instance.hpp>
#include <nall/encode/png.hpp>
#include <nall/hash/crc16.hpp>

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>

#include "input/input.hpp"
#include "emulator/emulator.hpp"
#include "program/program.hpp"
#include "settings/settings.hpp"

auto locate(const string& name) -> string;
void ScreenRefresh(const u32* data, u32 pitch, u32 width, u32 height);

//SDL globals shared across translation units
extern SDL_AudioDeviceID sdlAudioDevice;
extern SDL_Window* sdlWindow;
extern SDL_GameController* gameControllers[5];

inline constexpr int MaxControllers = 5;
