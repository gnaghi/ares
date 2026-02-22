auto Program::videoDriverUpdate() -> void {
  videoMonitorUpdate();
  videoFormatUpdate();
}

auto Program::videoMonitorUpdate() -> void {
}

auto Program::videoFormatUpdate() -> void {
}

auto Program::videoFullScreenToggle() -> void {
}

//

auto Program::audioDriverUpdate() -> void {
}

auto Program::audioDeviceUpdate() -> void {
}

auto Program::audioFrequencyUpdate() -> void {
}

auto Program::audioLatencyUpdate() -> void {
}

//

auto Program::inputDriverUpdate() -> void {
  inputManager.poll(true);
}
