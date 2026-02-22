struct Settings : Markup::Node {
  using string = nall::string;

  auto load() -> void;
  auto save() -> void;
  auto process(bool load) -> void;

  struct Video {
    string driver;
    string monitor;
    string format;
    bool exclusive = false;
    bool blocking = false;
    bool flush = false;
    string shader = "None";
    u32 multiplier = 2;
    string output = "Scale";
    bool aspectCorrection = true;
    bool adaptiveSizing = true;
    bool autoCentering = false;

    f64 luminance = 1.0;
    f64 saturation = 1.0;
    f64 gamma = 1.0;
    bool colorBleed = false;
    bool colorEmulation = true;
    bool deepBlackBoost = false;
    bool interframeBlending = true;
    bool overscan = false;
    bool pixelAccuracy = false;

    string quality = "SD";
    bool supersampling = false;
    bool disableVideoInterfaceProcessing = false;
    bool weaveDeinterlacing = true;
  } video;

  struct Audio {
    string driver;
    string device;
    u32 frequency = 0;
    u32 latency = 0;
    bool exclusive = false;
    bool blocking = true;
    bool dynamic = false;
    bool mute = false;

    f64 volume = 1.0;
    f64 balance = 0.0;
  } audio;

  struct Input {
    string driver;
    string defocus = "Pause";
  } input;

  struct Boot {
    bool fast = false;
    bool debugger = false;
    string prefer = "NTSC-U";
  } boot;

  struct General {
    bool showStatusBar = true;
    bool rewind = false;
    bool runAhead = false;
    bool autoSaveMemory = true;
    bool homebrewMode = false;
  } general;

  struct Rewind {
    u32 length = 100;
    u32 frequency = 10;
  } rewind;

  struct Paths {
    string home;
    string firmware;
    string saves;
    string screenshots;
    string debugging;
    string arcadeRoms;
    struct SuperFamicom {
      string gameBoy;
      string bsMemory;
      string sufamiTurbo;
    } superFamicom;
  } paths;

  struct Recent {
    string game[9];
  } recent;

  struct DebugServer {
    u32 port = 9123;
    bool enabled = false; // if enabled, server starts with ares
    bool useIPv4 = false; // forces IPv4 over IPv6
  } debugServer;

  struct Nintendo64 {
    bool expansionPak = true;
  } nintendo64;
};


extern Settings settings;
