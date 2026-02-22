struct FamicomDiskSystem : Emulator {
  FamicomDiskSystem();
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto notify(const string& message) -> void override;

  std::shared_ptr<mia::Pak> bios;
};

FamicomDiskSystem::FamicomDiskSystem() {
  manufacturer = "Nintendo";
  name = "Famicom Disk System";

  firmware.push_back({"BIOS", "Japan", "fdc1a76e654feea993fcb38366e05ee5f4eb641f86fe6bebaeefd412e112dd72"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Gamepad"};
    device.digital("Up",         virtualPorts[id].pad.up);
    device.digital("Down",       virtualPorts[id].pad.down);
    device.digital("Left",       virtualPorts[id].pad.left);
    device.digital("Right",      virtualPorts[id].pad.right);
    device.digital("B",          virtualPorts[id].pad.west);
    device.digital("A",          virtualPorts[id].pad.south);
    device.digital("Select",     virtualPorts[id].pad.select);
    device.digital("Start",      virtualPorts[id].pad.start);
    device.digital("Microphone", virtualPorts[id].pad.north);
    port.append(device); }

    ports.push_back(port);
  }
}

auto FamicomDiskSystem::load() -> bool {
  game = mia::Medium::create("Famicom Disk System");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  bios = mia::Medium::create("Famicom");
  if(!bios->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  system = mia::System::create("Famicom");
  if(!system->load()) return false;

  if(!ares::Famicom::load(root, "[Nintendo] Famicom (NTSC-J)")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->scan<ares::Node::Port>("Disk Slot")) {
    port->allocate();
    port->connect();
  }

  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue("Disk 1: Side A");
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Gamepad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Gamepad");
    port->connect();
  }

  return true;
}

auto FamicomDiskSystem::save() -> bool {
  root->save();
  system->save(system->location);
  bios->save(bios->location);
  game->save(game->location);
  return true;
}

auto FamicomDiskSystem::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Famicom") return system->pak;
  if(node->name() == "Famicom Cartridge") return bios->pak;
  if(node->name() == "Famicom Disk System") return game->pak;
  return {};
}

auto FamicomDiskSystem::notify(const string& message) -> void {
  if(auto node = root->scan<ares::Node::Setting::String>("State")) {
    node->setValue(message);
  }
}
