#include "../cmdsdl.hpp"
#include "platform.cpp"
#include "load.cpp"
#include "states.cpp"
#include "rewind.cpp"
#include "status.cpp"
#include "utility.cpp"
#include "drivers.cpp"

Program program;

auto Program::create() -> void {
  ares::platform = this;

  if(!startGameLoad.empty()) {
  	print("\nthere is a game to load : ");
    auto gameToLoad = startGameLoad.front();
    startGameLoad.erase(startGameLoad.begin());
    print(gameToLoad);
    if(startSystem) {
    	print("\nThere is a startSystem : ");
    	print(startSystem);
      for(auto &emulator: emulators) {
        if(emulator->name == startSystem) {
          if(load(emulator, gameToLoad))
          {
          	print("\ngame loaded.\n");
          }
          else
          {
          	print("Error loading game.");
          }

          return;
        }
      }
    }
    if(auto emulator = identify(gameToLoad))
    {
    	print("\nemulator detected : ", emulator->name, "\n");
    	if(load(emulator, gameToLoad)) {
    		print("\ngame loaded");
    	}

    }
    else
    {
    	print("\ncannot identify system.");
    }
//    print("Detected emulator : ");
//    print(emulator.name);
  }
}

auto Program::main() -> void {
  if(!emulator || !emulator->root) return;
//  inputManager.poll();

  emulator->root->run();
}

auto Program::quit() -> void {
  unload();
//Todo : replace this  
//  Application::processEvents();
//  Application::quit();
}
