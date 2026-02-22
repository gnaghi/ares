auto Program::attach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Video::Screen>()) {
    screens = emulator->root->find<ares::Node::Video::Screen>();
  }

  if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
    streams = emulator->root->find<ares::Node::Audio::Stream>();
    stream->setResamplerFrequency(48000.0);
  }
}

auto Program::detach(ares::Node::Object node) -> void {
  if(auto screen = node->cast<ares::Node::Video::Screen>()) {
    screens = emulator->root->find<ares::Node::Video::Screen>();
    std::erase(screens, screen);
  }

  if(auto stream = node->cast<ares::Node::Audio::Stream>()) {
    streams = emulator->root->find<ares::Node::Audio::Stream>();
    std::erase(streams, stream);
  }
}

auto Program::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  return emulator->pak(node);
}

auto Program::event(ares::Event event) -> void {
}

auto Program::log(ares::Node::Debugger::Tracer::Tracer node, string_view message) -> void {
  string channel = string{node->component(), " ", node->name()};

  string modifiedMessage;
  if(node->prefix()) { message = modifiedMessage = string{channel, ": ", message}; }
  if(node->autoLineBreak()) message = modifiedMessage = string{message, "\n"};

  if(node->terminal()) {
    print(message);
  }
}

auto Program::status(string_view message) -> void {
  showMessage(message);
}

auto Program::video(ares::Node::Video::Screen node, const u32* data, u32 pitch, u32 width, u32 height) -> void {
  ScreenRefresh(data, pitch, width, height);
}

auto Program::audio(ares::Node::Audio::Stream node) -> void {
  if(streams.empty()) return;

  while(true) {
    for(auto& stream : streams) {
      if(!stream->pending()) return;
    }

    f64 samples[2] = {};
    for(auto& stream : streams) {
      f64 buffer[2] = {};
      u32 channels = stream->read(buffer);
      if(channels == 1) buffer[1] = buffer[0];
      samples[0] += buffer[0];
      samples[1] += buffer[1];
    }

    if(sdlAudioDevice) {
      float output[2] = {(float)samples[0], (float)samples[1]};
      SDL_QueueAudio(sdlAudioDevice, output, sizeof(output));
    }
  }
}

// ── Gamepad helpers ─────────────────────────────────────────────────────────

static auto padButton(u32 port, SDL_GameControllerButton btn) -> bool {
  if(port >= MaxControllers) return false;
  auto pad = gameControllers[port];
  if(!pad) return false;
  return SDL_GameControllerGetButton(pad, btn);
}

static auto padAxis(u32 port, SDL_GameControllerAxis axis) -> s16 {
  if(port >= MaxControllers) return 0;
  auto pad = gameControllers[port];
  if(!pad) return 0;
  return SDL_GameControllerGetAxis(pad, axis);
}

static constexpr s16 DEADZONE = 8000;

//detect which virtual port (0..4) an input node belongs to
static auto detectPort(ares::Node::Input::Input node) -> u32 {
  auto parent = node->parent();
  if(parent.expired()) return 0;
  auto p = parent.lock();
  auto pp = p->parent();
  if(pp.expired()) return 0;
  auto portNode = pp.lock();
  auto portName = portNode->name();
  for(u32 i = 0; i < 5; i++) {
    char digit = '1' + i;
    string needle{&digit, 1};
    if(portName.find(needle)) return i;
  }
  return 0;
}

auto Program::input(ares::Node::Input::Input node) -> void {
  const u8* keys = SDL_GetKeyboardState(NULL);
  auto name = node->name();
  u32 port = detectPort(node);

  if(auto button = node->cast<ares::Node::Input::Button>()) {
    s16 pressed = 0;

    //keyboard mapping (port 0 only)
    if(port == 0) {
      if     (name == "Up")      pressed = keys[SDL_SCANCODE_UP];
      else if(name == "Down")    pressed = keys[SDL_SCANCODE_DOWN];
      else if(name == "Left")    pressed = keys[SDL_SCANCODE_LEFT];
      else if(name == "Right")   pressed = keys[SDL_SCANCODE_RIGHT];
      else if(name == "B")       pressed = keys[SDL_SCANCODE_Z];
      else if(name == "A")       pressed = keys[SDL_SCANCODE_X];
      else if(name == "Y")       pressed = keys[SDL_SCANCODE_A];
      else if(name == "X")       pressed = keys[SDL_SCANCODE_S];
      else if(name == "L")       pressed = keys[SDL_SCANCODE_Q];
      else if(name == "R")       pressed = keys[SDL_SCANCODE_W];
      else if(name == "Select")  pressed = keys[SDL_SCANCODE_RSHIFT];
      else if(name == "Start")   pressed = keys[SDL_SCANCODE_RETURN];
      else if(name == "C")       pressed = keys[SDL_SCANCODE_C];
      else if(name == "Mode")    pressed = keys[SDL_SCANCODE_TAB];
      else if(name == "Z")       pressed = keys[SDL_SCANCODE_D];
      else if(name == "C-Up")    pressed = keys[SDL_SCANCODE_I];
      else if(name == "C-Down")  pressed = keys[SDL_SCANCODE_K];
      else if(name == "C-Left")  pressed = keys[SDL_SCANCODE_J];
      else if(name == "C-Right") pressed = keys[SDL_SCANCODE_L];
    }

    //gamepad mapping (all ports, positional: south=B, east=A, west=Y, north=X)
    if     (name == "Up")      pressed |= padButton(port, SDL_CONTROLLER_BUTTON_DPAD_UP);
    else if(name == "Down")    pressed |= padButton(port, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    else if(name == "Left")    pressed |= padButton(port, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
    else if(name == "Right")   pressed |= padButton(port, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    else if(name == "B")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_A);
    else if(name == "A")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_B);
    else if(name == "Y")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_X);
    else if(name == "X")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_Y);
    else if(name == "L")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    else if(name == "R")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    else if(name == "Select")  pressed |= padButton(port, SDL_CONTROLLER_BUTTON_BACK);
    else if(name == "Start")   pressed |= padButton(port, SDL_CONTROLLER_BUTTON_START);
    else if(name == "C")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    else if(name == "Mode")    pressed |= padButton(port, SDL_CONTROLLER_BUTTON_BACK);
    else if(name == "Z")       pressed |= padButton(port, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
    else if(name == "L-Stick") pressed |= padButton(port, SDL_CONTROLLER_BUTTON_LEFTSTICK);
    else if(name == "R-Stick") pressed |= padButton(port, SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    else if(name == "C-Up")    { s16 v = padAxis(port, SDL_CONTROLLER_AXIS_RIGHTY); pressed |= (v < -DEADZONE); }
    else if(name == "C-Down")  { s16 v = padAxis(port, SDL_CONTROLLER_AXIS_RIGHTY); pressed |= (v >  DEADZONE); }
    else if(name == "C-Left")  { s16 v = padAxis(port, SDL_CONTROLLER_AXIS_RIGHTX); pressed |= (v < -DEADZONE); }
    else if(name == "C-Right") { s16 v = padAxis(port, SDL_CONTROLLER_AXIS_RIGHTX); pressed |= (v >  DEADZONE); }

    //left analog stick also drives d-pad
    if(name == "Up" || name == "Down" || name == "Left" || name == "Right") {
      s16 lx = padAxis(port, SDL_CONTROLLER_AXIS_LEFTX);
      s16 ly = padAxis(port, SDL_CONTROLLER_AXIS_LEFTY);
      if(name == "Up")    pressed |= (ly < -DEADZONE);
      if(name == "Down")  pressed |= (ly >  DEADZONE);
      if(name == "Left")  pressed |= (lx < -DEADZONE);
      if(name == "Right") pressed |= (lx >  DEADZONE);
    }

    button->setValue(pressed);
    return;
  }

  if(auto axis = node->cast<ares::Node::Input::Axis>()) {
    s16 value = 0;

    if(port == 0) {
      if(name == "X-Axis") {
        if(keys[SDL_SCANCODE_LEFT])  value -= 32767;
        if(keys[SDL_SCANCODE_RIGHT]) value += 32767;
      } else if(name == "Y-Axis") {
        if(keys[SDL_SCANCODE_UP])   value -= 32767;
        if(keys[SDL_SCANCODE_DOWN]) value += 32767;
      }
    }

    if(name == "X-Axis") {
      s16 ax = padAxis(port, SDL_CONTROLLER_AXIS_LEFTX);
      if(ax > DEADZONE || ax < -DEADZONE) value = ax;
    } else if(name == "Y-Axis") {
      s16 ay = padAxis(port, SDL_CONTROLLER_AXIS_LEFTY);
      if(ay > DEADZONE || ay < -DEADZONE) value = ay;
    }

    axis->setValue(value);
    return;
  }
}
