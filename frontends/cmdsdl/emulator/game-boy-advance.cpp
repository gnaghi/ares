struct GameBoyAdvance : Emulator {
  GameBoyAdvance();
  auto load(Menu) -> void;
  auto load() -> bool override;
  auto save() -> bool override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
};

GameBoyAdvance::GameBoyAdvance() {
  manufacturer = "Nintendo";
  name = "Game Boy Advance";

  firmware.push_back({"BIOS", "World", "fd2547724b505f487e6dcb29ec2ecff3af35a841a77ab2e85fd87350abd36570"});

  { InputPort port{string{"Game Boy Advance"}};

  { InputDevice device{"Controls"};
    device.digital("Up",     virtualPorts[0].pad.up);
    device.digital("Down",   virtualPorts[0].pad.down);
    device.digital("Left",   virtualPorts[0].pad.left);
    device.digital("Right",  virtualPorts[0].pad.right);
    device.digital("B",      virtualPorts[0].pad.south);
    device.digital("A",      virtualPorts[0].pad.east);
    device.digital("L",      virtualPorts[0].pad.l_bumper);
    device.digital("R",      virtualPorts[0].pad.r_bumper);
    device.digital("Select", virtualPorts[0].pad.select);
    device.digital("Start",  virtualPorts[0].pad.start);
    device.rumble ("Rumble", virtualPorts[0].pad.rumble);
    port.append(device); }

    ports.push_back(port);
  }
}

auto GameBoyAdvance::load(Menu menu) -> void {
}

auto GameBoyAdvance::load() -> bool {
  game = mia::Medium::create("Game Boy Advance");
  if(!game->load(Emulator::load(game, configuration.game))) return false;

  system = mia::System::create("Game Boy Advance");
  if(!system->load(firmware[0].location)) return errorFirmware(firmware[0]), false;

  if(!ares::GameBoyAdvance::load(root, "[Nintendo] Game Boy Advance")) return false;

  if(auto port = root->find<ares::Node::Port>("Cartridge Slot")) {
    port->allocate();
    port->connect();
  }

  return true;
}

auto GameBoyAdvance::save() -> bool {
  root->save();
  system->save(system->location);
  game->save(game->location);
  return true;
}

auto GameBoyAdvance::pak(ares::Node::Object node) -> std::shared_ptr<vfs::directory> {
  if(node->name() == "Game Boy Advance") return system->pak;
  if(node->name() == "Game Boy Advance Cartridge") return game->pak;
  return {};
}
