#include "../cmdsdl.hpp"
#include "emulators.cpp"

std::vector<std::shared_ptr<Emulator>> emulators;
std::shared_ptr<Emulator> emulator;

auto Emulator::enumeratePorts(string name) -> std::vector<InputPort>& {
  for(auto& emulator : emulators) {
    if(emulator->name == name && !emulator->ports.empty()) return emulator->ports;
  }
  static std::vector<InputPort> ports;
  if(ports.empty()) {
    for(auto id : range(5)) {
      InputPort port{string{"Controller Port ", 1 + id}};
      port.append(virtualPorts[id].pad);
      port.append(virtualPorts[id].mouse);
      ports.push_back(port);
    }
  }
  return ports;
}

auto Emulator::location() -> string {
  return {Path::userData(), "ares/Saves/", name, "/"};
}

auto Emulator::locate(const string& location, const string& suffix, const string& path, maybe<string> system) -> string {
  if(!system) system = root->name();

  //game path
  if(!path) return {Location::notsuffix(location), suffix};

  //path override
  string pathname = {path, *system, "/"};
  directory::create(pathname);
  return {pathname, Location::prefix(location), suffix};
}

//handles region selection when games support multiple regions
auto Emulator::region() -> string {
  auto preferredRegions = nall::split_and_strip(settings.boot.prefer, ",");
  if(game && game->pak) {
    auto regionList = game->pak->attribute("region");
    auto regions = nall::split_and_strip(regionList, ",");
    if(!regions.empty()) {
      for(auto& prefer : preferredRegions) {
        if(std::ranges::find(regions, prefer) != regions.end()) return prefer;
        if(prefer == "NTSC-U" || prefer == "NTSC-J") {
          if(std::ranges::find(regions, string("NTSC")) != regions.end()) return "NTSC";
        }
      }
      return regions.front();
    }
  }
  return {};
}

auto Emulator::load(const string& location) -> bool {
  if(inode::exists(location)) locationQueue.push_back(location);

  if(!load()) return false;
  setBoolean("Color Emulation", settings.video.colorEmulation);
  setBoolean("Deep Black Boost", settings.video.deepBlackBoost);
  setBoolean("Interframe Blending", settings.video.interframeBlending);
  setOverscan(settings.video.overscan);
  setColorBleed(settings.video.colorBleed);

  latch = {};
  root->power();
  return true;
}

auto Emulator::load(std::shared_ptr<mia::Pak> pak, string& path) -> string {
  string location;
  if(!locationQueue.empty()) {
    location = locationQueue.front();
    locationQueue.erase(locationQueue.begin());  //pull from the game queue if an entry is available
  } else if(!program.startGameLoad.empty()) {
    location = program.startGameLoad.front();
    program.startGameLoad.erase(program.startGameLoad.begin()); //pull from the command line if an entry is available
  }

  if(location) {
    path = Location::dir(location);
    return location;
  }
  return {};
}

auto Emulator::loadFirmware(const Firmware& firmware) -> std::shared_ptr<vfs::file> {
  if(firmware.location.iendsWith(".zip")) {
    Decode::ZIP archive;
    if(archive.open(firmware.location) && !archive.file.empty()) {
      auto image = archive.extract(archive.file.front());
      return vfs::memory::open(image);
    }
  } else {
    auto image = file::read(firmware.location);
    if(!image.empty()) {
      return vfs::memory::open(image);
    }
  }
  return {};
}

auto Emulator::unload() -> void {
  save();
  root->unload();
  game = {};
  system = {};
  root.reset();
  locationQueue.clear();
}

auto Emulator::load(mia::Pak& node, string name) -> bool {
  if(auto fp = node.pak->read(name)) {
    auto memory = file::read({node.location, name});
    if(!memory.empty()) {
      fp->read(memory);
      return true;
    }
  }
  return false;
}

auto Emulator::save(mia::Pak& node, string name) -> bool {
  if(auto memory = node.pak->write(name)) {
    return file::write({node.location, name}, {memory->data(), memory->size()});
  }
  return false;
}

auto Emulator::refresh() -> void {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->refresh();
  }
}

auto Emulator::setBoolean(const string& name, bool value) -> bool {
  if(auto node = root->scan<ares::Node::Setting::Boolean>(name)) {
    node->setValue(value);  //setValue() will not call modify() if value has not changed;
    node->modify(value);    //but that may prevent the initial setValue() from working
    return true;
  }
  return false;
}

auto Emulator::setOverscan(bool value) -> bool {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    if(auto overscan = screen->find<ares::Node::Setting::Boolean>("Overscan")) {
      overscan->setValue(value);
      return true;
    }
  }
  return false;
}

auto Emulator::setColorBleed(bool value) -> bool {
  if(auto screen = root->scan<ares::Node::Video::Screen>("Screen")) {
    screen->setColorBleed(screen->height() < 720 ? value : false);  //only apply to sub-HD content
    return true;
  }

  return false;
}

auto Emulator::error(const string& text) -> void {
}

auto Emulator::errorFirmware(const Firmware& firmware, string system) -> void {
  if(!system) system = emulator->name;
}

auto Emulator::input(ares::Node::Input::Input input) -> void {
  //looking up inputs is very time-consuming; skip call if input was called too recently
  auto thisPoll = chrono::millisecond();
  if(thisPoll - input->lastPoll < 5) return;
  input->lastPoll = thisPoll;

  auto device = ares::Node::parent(input);
  if(!device) return;

  auto port = ares::Node::parent(device);
  if(!port) return;
  
  for(auto& inputPort : ports) {
    if(inputPort.name != port->name()) continue;
    for(auto& inputDevice : inputPort.devices) {
      if(inputDevice.name != device->name()) continue;
      for(auto& inputNode : inputDevice.inputs) {
        if(inputNode.name != input->name()) continue;
        if(auto button = input->cast<ares::Node::Input::Button>()) {
          auto pressed = inputNode.mapping->pressed();
          return button->setValue(pressed);
        }
        if(auto axis = input->cast<ares::Node::Input::Axis>()) {
          auto value = inputNode.mapping->value();
          return axis->setValue(value);
        }
        if(auto rumble = input->cast<ares::Node::Input::Rumble>()) {
          if(auto target = dynamic_cast<InputRumble*>(inputNode.mapping)) {
            return target->rumble(rumble->strongValue(), rumble->weakValue());
          }
        }
      }
      for(auto& inputPair : inputDevice.pairs) {
        if(inputPair.name != input->name()) continue;
        if(auto axis = input->cast<ares::Node::Input::Axis>()) {
          auto value = inputPair.mappingHi->value() - inputPair.mappingLo->value();
          return axis->setValue(value);
        }
      }
    }
  }
}

auto Emulator::inputKeyboard(string name) -> bool {
  for (auto& device : inputManager.devices) {
    if (!device->isKeyboard()) continue;

    auto keyboard = std::dynamic_pointer_cast<HID::Keyboard>(device);

    auto key = keyboard->buttons().find(name);
    if (!key) return false;

    return keyboard->buttons().input(*key).value();
  }

  return false;
}

