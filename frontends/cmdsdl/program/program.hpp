struct Program : ares::Platform {
  auto create() -> void;
  auto main() -> void;
  auto quit() -> void;

  //platform.cpp
  auto attach(ares::Node::Object) -> void override;
  auto detach(ares::Node::Object) -> void override;
  auto pak(ares::Node::Object) -> std::shared_ptr<vfs::directory> override;
  auto event(ares::Event) -> void override;
  auto log(ares::Node::Debugger::Tracer::Tracer tracer, string_view message) -> void override;
  auto status(string_view message) -> void override;
  auto video(ares::Node::Video::Screen, const u32* data, u32 pitch, u32 width, u32 height) -> void override;
  auto audio(ares::Node::Audio::Stream) -> void override;
  auto input(ares::Node::Input::Input) -> void override;

  //load.cpp
  auto identify(const string& filename) -> std::shared_ptr<Emulator>;
  auto load(std::shared_ptr<Emulator> emulator, string location = {}) -> bool;
  auto load(string location) -> bool;
  auto unload() -> void;

  //states.cpp
  auto stateSave(u32 slot) -> bool;
  auto stateLoad(u32 slot) -> bool;
  auto undoStateSave() -> bool;
  auto undoStateLoad() -> bool;
  auto clearUndoStates() -> void;

  //status.cpp
  auto updateMessage() -> void;
  auto showMessage(const string&) -> void;

  //utility.cpp
  auto pause(bool) -> void;
  auto mute() -> void;
  auto paletteUpdate() -> void;
  auto runAheadUpdate() -> void;
  auto captureScreenshot(const u32* data, u32 pitch, u32 width, u32 height) -> void;


  //drivers.cpp
  auto videoDriverUpdate() -> void;
  auto videoMonitorUpdate() -> void;
  auto videoFormatUpdate() -> void;
  auto videoFullScreenToggle() -> void;

  auto audioDriverUpdate() -> void;
  auto audioDeviceUpdate() -> void;
  auto audioFrequencyUpdate() -> void;
  auto audioLatencyUpdate() -> void;

  auto inputDriverUpdate() -> void;

  bool startFullScreen = false;
  std::vector<string> startGameLoad;
  bool noFilePrompt = false;

  string startSystem;
  string startShader;

  std::vector<ares::Node::Video::Screen> screens;
  std::vector<ares::Node::Audio::Stream> streams;

  bool paused = false;
  bool fastForwarding = false;
  bool rewinding = false;
  bool runAhead = false;
  bool requestFrameAdvance = false;
  bool requestScreenshot = false;
  bool keyboardCaptured = false;

  struct State {
    u32 slot = 1;
    u32 undoSlot = 1;
  } state;

  //rewind.cpp
  struct Rewind {
    enum class Mode : u32 { Playing, Rewinding } mode = Mode::Playing;
    std::vector<serializer> history;
    u32 length = 0;
    u32 frequency = 0;
    u32 counter = 0;
  } rewind;
  auto rewindSetMode(Rewind::Mode) -> void;
  auto rewindReset() -> void;
  auto rewindRun() -> void;

  struct Message {
    u64 timestamp = 0;
    string text;
  } message;

  std::vector<Message> messages;
  maybe<u64> vblanksPerSecond;
};

extern Program program;
