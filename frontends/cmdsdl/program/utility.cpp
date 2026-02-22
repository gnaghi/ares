auto Program::pause(bool state) -> void {
  if(paused == state) return;
  paused = state;

}

auto Program::mute() -> void {
  settings.audio.mute = !settings.audio.mute;
}

auto Program::paletteUpdate() -> void {
  if(!emulator) return;
  for(auto& screen : emulator->root->find<ares::Node::Video::Screen>()) {
    screen->setLuminance(settings.video.luminance);
    screen->setSaturation(settings.video.saturation);
    screen->setGamma(settings.video.gamma);
  }
}

auto Program::runAheadUpdate() -> void {
  runAhead = settings.general.runAhead;
  if(!emulator) return;
  if(emulator->name == "Game Boy Advance") runAhead = false;  //crashes immediately
  if(emulator->name == "Nintendo 64") runAhead = false;  //too demanding
  if(emulator->name == "PlayStation") runAhead = false;  //too demanding
}

auto Program::captureScreenshot(const u32* data, u32 pitch, u32 width, u32 height) -> void {
  string filename{emulator->locate({Location::notsuffix(emulator->game->location), " ", chrono::local::datetime().transform(":", "-"), ".png"}, ".png", settings.paths.screenshots)};
  if(Encode::PNG::RGB8(filename, data, pitch, width, height)) {
    showMessage("Captured screenshot");
  } else {
    showMessage("Failed to capture screenshot");
  }
}
