auto Program::updateMessage() -> void {
}

auto Program::showMessage(const string& text) -> void {
  messages.push_back({chrono::millisecond(), text});
  printf("%s\n", (const char*)text);
}
