auto Program::identify(const string& filename) -> std::shared_ptr<Emulator> {
  if(auto system = mia::identify(filename)) {
    for(auto& emulator : emulators) {
      if(emulator->name == system) return emulator;
    }
  }
  print("\nUnable to identify what type of game this file is.");
  return {};
}

//location is an optional game to load automatically (for command-line loading)
auto Program::load(std::shared_ptr<Emulator> emulator, string location) -> bool {
  unload();

  ::emulator = emulator;

  // For arcade systems, show the game browser dialog as we're using MAME-compatible roms
  if(emulator->arcade() && !location) {
//    gameBrowserWindow.show(emulator);

    // Temporarily pretend that the load failed to prevent UI hang
    // The browser dialog will call load() again when necessary
    ::emulator.reset();
    return false;
  }

  return load(location);
}

auto Program::load(string location) -> bool {
  if(settings.debugServer.enabled) {
    nall::GDB::server.reset();
  }

  if(!emulator->load(location)) {
    emulator.reset();
    return false;
  }
  location = emulator->game->location;

  pause(false);


  return true;
}

auto Program::unload() -> void {
  if(!emulator) return;

  nall::GDB::server.close();
  nall::GDB::server.reset();

  settings.save();
  clearUndoStates();
  showMessage({"Unloaded ", Location::prefix(emulator->game->location)});
  emulator->unload();
  screens.clear();
  streams.clear();
  emulator.reset();
  rewindReset();
  message.text = "";
}
