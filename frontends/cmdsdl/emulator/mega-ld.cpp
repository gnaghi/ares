struct MegaLD : Emulator {
  MegaLD();
  auto load() -> bool override;
  auto load(Menu) -> void;
  auto unload() -> void override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;

  u32 regionID = 0;
  sTimer discTrayTimer;
};

MegaLD::MegaLD() {
  manufacturer = "Pioneer";
  name = "LaserActive (SEGA PAC)";

  firmware.push_back({"BIOS", "US"});
  firmware.push_back({"BIOS", "Japan"});

  for(auto id : range(2)) {
    InputPort port{string{"Controller Port ", 1 + id}};

  { InputDevice device{"Fighting Pad"};
    device.digital("Up",    virtualPorts[id].pad.up);
    device.digital("Down",  virtualPorts[id].pad.down);
    device.digital("Left",  virtualPorts[id].pad.left);
    device.digital("Right", virtualPorts[id].pad.right);
    device.digital("A",     virtualPorts[id].pad.west);
    device.digital("B",     virtualPorts[id].pad.south);
    device.digital("C",     virtualPorts[id].pad.east);
    device.digital("X",     virtualPorts[id].pad.l_bumper);
    device.digital("Y",     virtualPorts[id].pad.north);
    device.digital("Z",     virtualPorts[id].pad.r_bumper);
    device.digital("Mode",  virtualPorts[id].pad.select);
    device.digital("Start", virtualPorts[id].pad.start);
    port.append(device); }

    ports.push_back(port);
  }
}

auto MegaLD::load() -> bool {
  game = mia::Medium::create("Mega LD");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  auto region = Emulator::region();
  if(region == "NTSC-J") regionID = 1;
  if(region == "NTSC-U") regionID = 0;

  system = mia::System::create("Mega LD");
  if(!system->load(firmware[regionID].location)) {
    errorFirmware(firmware[regionID], "Mega LD");
    return false;
  }

  if(!ares::MegaDrive::load(root, {"[Pioneer] LaserActive (SEGA PAC) (", region, ")"})) return false;

  if(auto port = root->find<ares::Node::Port>("Mega CD/Disc Tray")) {
    port->allocate();
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 1")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  if(auto port = root->find<ares::Node::Port>("Controller Port 2")) {
    port->allocate("Fighting Pad");
    port->connect();
  }

  discTrayTimer = Timer{};
  return true;
}

auto MegaLD::load(Menu menu) -> void {
}

auto MegaLD::unload() -> void {
  Emulator::unload();
  discTrayTimer.reset();
}

auto MegaLD::save() -> bool {
  root->save();
  system->save(game->location);
  game->save(game->location);
  return true;
}

auto MegaLD::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Mega Drive") return system->pak;
  if(node->name() == "Mega CD Disc") return game->pak;
  return {};
}
