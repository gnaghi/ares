#include "cmdsdl.hpp"
#include <iostream>

// ── Global SDL state ────────────────────────────────────────────────────────

SDL_AudioDeviceID sdlAudioDevice = 0;
SDL_Renderer* renderer = nullptr;
SDL_Texture* texture = nullptr;
SDL_Window* sdlWindow = nullptr;
SDL_GameController* gameControllers[MaxControllers] = {};

static u32 textureWidth = 0;
static u32 textureHeight = 0;

//intermediate framebuffer (filled by emulator callback, consumed by main loop)
static std::vector<u32> framebuffer;
static u32 fbWidth = 0;
static u32 fbHeight = 0;
static u32 fbPitch = 0;
static bool fbReady = false;

//fullscreen & scaling
static bool isFullscreen = false;
static int scaleMultiplier = -1;  //-1=stretch (default), 0=aspect fit, 1-4=window multiplier
static SDL_Rect destRect = {};
static bool useDestRect = false;
static int windowWidth = 640;
static int windowHeight = 480;

//hotkey edge detection (previous frame state)
static struct {
  bool l, r, dpadUp, dpadDown, start, x, y;
} prevBtn = {};

// ── OSD: embedded 8x8 bitmap font (ASCII 0x20..0x7E) ───────────────────────

static const u8 osdFont[95][8] = {
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
  {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, // !
  {0x6C,0x6C,0x24,0x00,0x00,0x00,0x00,0x00}, // "
  {0x6C,0x6C,0xFE,0x6C,0xFE,0x6C,0x6C,0x00}, // #
  {0x18,0x7E,0xC0,0x7C,0x06,0xFC,0x18,0x00}, // $
  {0x00,0xC6,0xCC,0x18,0x30,0x66,0xC6,0x00}, // %
  {0x38,0x6C,0x38,0x76,0xDC,0xCC,0x76,0x00}, // &
  {0x18,0x18,0x30,0x00,0x00,0x00,0x00,0x00}, // '
  {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // (
  {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // )
  {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // *
  {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // +
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // ,
  {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // -
  {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // .
  {0x06,0x0C,0x18,0x30,0x60,0xC0,0x80,0x00}, // /
  {0x7C,0xC6,0xCE,0xDE,0xF6,0xE6,0x7C,0x00}, // 0
  {0x18,0x38,0x78,0x18,0x18,0x18,0x7E,0x00}, // 1
  {0x7C,0xC6,0x06,0x1C,0x30,0x66,0xFE,0x00}, // 2
  {0x7C,0xC6,0x06,0x3C,0x06,0xC6,0x7C,0x00}, // 3
  {0x1C,0x3C,0x6C,0xCC,0xFE,0x0C,0x1E,0x00}, // 4
  {0xFE,0xC0,0xFC,0x06,0x06,0xC6,0x7C,0x00}, // 5
  {0x38,0x60,0xC0,0xFC,0xC6,0xC6,0x7C,0x00}, // 6
  {0xFE,0xC6,0x0C,0x18,0x30,0x30,0x30,0x00}, // 7
  {0x7C,0xC6,0xC6,0x7C,0xC6,0xC6,0x7C,0x00}, // 8
  {0x7C,0xC6,0xC6,0x7E,0x06,0x0C,0x78,0x00}, // 9
  {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x00}, // :
  {0x00,0x18,0x18,0x00,0x00,0x18,0x18,0x30}, // ;
  {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // <
  {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00}, // =
  {0x60,0x30,0x18,0x0C,0x18,0x30,0x60,0x00}, // >
  {0x7C,0xC6,0x0C,0x18,0x18,0x00,0x18,0x00}, // ?
  {0x7C,0xC6,0xDE,0xDE,0xDE,0xC0,0x78,0x00}, // @
  {0x38,0x6C,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, // A
  {0xFC,0x66,0x66,0x7C,0x66,0x66,0xFC,0x00}, // B
  {0x3C,0x66,0xC0,0xC0,0xC0,0x66,0x3C,0x00}, // C
  {0xF8,0x6C,0x66,0x66,0x66,0x6C,0xF8,0x00}, // D
  {0xFE,0x62,0x68,0x78,0x68,0x62,0xFE,0x00}, // E
  {0xFE,0x62,0x68,0x78,0x68,0x60,0xF0,0x00}, // F
  {0x3C,0x66,0xC0,0xC0,0xCE,0x66,0x3E,0x00}, // G
  {0xC6,0xC6,0xC6,0xFE,0xC6,0xC6,0xC6,0x00}, // H
  {0x3C,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // I
  {0x1E,0x0C,0x0C,0x0C,0xCC,0xCC,0x78,0x00}, // J
  {0xE6,0x66,0x6C,0x78,0x6C,0x66,0xE6,0x00}, // K
  {0xF0,0x60,0x60,0x60,0x62,0x66,0xFE,0x00}, // L
  {0xC6,0xEE,0xFE,0xD6,0xC6,0xC6,0xC6,0x00}, // M
  {0xC6,0xE6,0xF6,0xDE,0xCE,0xC6,0xC6,0x00}, // N
  {0x7C,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, // O
  {0xFC,0x66,0x66,0x7C,0x60,0x60,0xF0,0x00}, // P
  {0x7C,0xC6,0xC6,0xC6,0xD6,0xDE,0x7C,0x0E}, // Q
  {0xFC,0x66,0x66,0x7C,0x6C,0x66,0xE6,0x00}, // R
  {0x7C,0xC6,0x60,0x38,0x0C,0xC6,0x7C,0x00}, // S
  {0x7E,0x5A,0x18,0x18,0x18,0x18,0x3C,0x00}, // T
  {0xC6,0xC6,0xC6,0xC6,0xC6,0xC6,0x7C,0x00}, // U
  {0xC6,0xC6,0xC6,0xC6,0x6C,0x38,0x10,0x00}, // V
  {0xC6,0xC6,0xC6,0xD6,0xFE,0xEE,0xC6,0x00}, // W
  {0xC6,0xC6,0x6C,0x38,0x6C,0xC6,0xC6,0x00}, // X
  {0x66,0x66,0x66,0x3C,0x18,0x18,0x3C,0x00}, // Y
  {0xFE,0xC6,0x8C,0x18,0x32,0x66,0xFE,0x00}, // Z
  {0x3C,0x30,0x30,0x30,0x30,0x30,0x3C,0x00}, // [
  {0xC0,0x60,0x30,0x18,0x0C,0x06,0x02,0x00}, // backslash
  {0x3C,0x0C,0x0C,0x0C,0x0C,0x0C,0x3C,0x00}, // ]
  {0x10,0x38,0x6C,0xC6,0x00,0x00,0x00,0x00}, // ^
  {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFE}, // _
  {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // `
  {0x00,0x00,0x78,0x0C,0x7C,0xCC,0x76,0x00}, // a
  {0xE0,0x60,0x7C,0x66,0x66,0x66,0xDC,0x00}, // b
  {0x00,0x00,0x7C,0xC6,0xC0,0xC6,0x7C,0x00}, // c
  {0x1C,0x0C,0x7C,0xCC,0xCC,0xCC,0x76,0x00}, // d
  {0x00,0x00,0x7C,0xC6,0xFE,0xC0,0x7C,0x00}, // e
  {0x38,0x6C,0x60,0xF0,0x60,0x60,0xF0,0x00}, // f
  {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0xF8}, // g
  {0xE0,0x60,0x6C,0x76,0x66,0x66,0xE6,0x00}, // h
  {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // i
  {0x06,0x00,0x0E,0x06,0x06,0x66,0x66,0x3C}, // j
  {0xE0,0x60,0x66,0x6C,0x78,0x6C,0xE6,0x00}, // k
  {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // l
  {0x00,0x00,0xEC,0xFE,0xD6,0xC6,0xC6,0x00}, // m
  {0x00,0x00,0xDC,0x66,0x66,0x66,0x66,0x00}, // n
  {0x00,0x00,0x7C,0xC6,0xC6,0xC6,0x7C,0x00}, // o
  {0x00,0x00,0xDC,0x66,0x66,0x7C,0x60,0xF0}, // p
  {0x00,0x00,0x76,0xCC,0xCC,0x7C,0x0C,0x1E}, // q
  {0x00,0x00,0xDC,0x76,0x60,0x60,0xF0,0x00}, // r
  {0x00,0x00,0x7C,0xC0,0x7C,0x06,0xFC,0x00}, // s
  {0x30,0x30,0xFC,0x30,0x30,0x36,0x1C,0x00}, // t
  {0x00,0x00,0xCC,0xCC,0xCC,0xCC,0x76,0x00}, // u
  {0x00,0x00,0xC6,0xC6,0xC6,0x6C,0x38,0x00}, // v
  {0x00,0x00,0xC6,0xC6,0xD6,0xFE,0x6C,0x00}, // w
  {0x00,0x00,0xC6,0x6C,0x38,0x6C,0xC6,0x00}, // x
  {0x00,0x00,0xC6,0xC6,0xCE,0x76,0x06,0xFC}, // y
  {0x00,0x00,0xFE,0x0C,0x38,0x60,0xFE,0x00}, // z
  {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // {
  {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // |
  {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // }
  {0x76,0xDC,0x00,0x00,0x00,0x00,0x00,0x00}, // ~
};

static constexpr u64 OSD_DISPLAY_MS = 3000;
static constexpr int OSD_SCALE = 2;
static constexpr int OSD_MARGIN = 8;

#include <nall/main.hpp>

void ScreenRefresh(const u32* data, u32 pitch, u32 width, u32 height) {
        u32 size = height * (pitch / sizeof(u32));
        if(framebuffer.size() < size) framebuffer.resize(size);
        memcpy(framebuffer.data(), data, height * pitch);
        fbWidth = width;
        fbHeight = height;
        fbPitch = pitch;
        fbReady = true;
}

// ── Scaling ─────────────────────────────────────────────────────────────────

static void applyScaleResize() {
        if(!sdlWindow || !textureHeight || scaleMultiplier < 1) return;
        if(isFullscreen) return;

        int h = (int)textureHeight * scaleMultiplier;
        int w = h * 4 / 3;
        SDL_SetWindowSize(sdlWindow, w, h);
        SDL_SetWindowPosition(sdlWindow, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        windowWidth = w;
        windowHeight = h;
}

static void recalcDestRect() {
        if(!textureWidth || !textureHeight) return;

        if(scaleMultiplier != 0) {
                useDestRect = false;
                return;
        }

        //--scale 0: aspect-correct fit with letterboxing
        useDestRect = true;
        float scaleW = (float)windowWidth  / (float)textureWidth;
        float scaleH = (float)windowHeight / (float)textureHeight;
        float scale = scaleW < scaleH ? scaleW : scaleH;
        destRect.w = (int)((float)textureWidth  * scale);
        destRect.h = (int)((float)textureHeight * scale);
        destRect.x = (windowWidth  - destRect.w) / 2;
        destRect.y = (windowHeight - destRect.h) / 2;
}

static void toggleFullscreen() {
        if(!sdlWindow) return;
        isFullscreen = !isFullscreen;
        SDL_SetWindowFullscreen(sdlWindow, isFullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
        SDL_GetWindowSize(sdlWindow, &windowWidth, &windowHeight);
        recalcDestRect();
}

// ── Gamepad management ──────────────────────────────────────────────────────

static void openController(int deviceIndex) {
        if(!SDL_IsGameController(deviceIndex)) return;
        for(int i = 0; i < MaxControllers; i++) {
                if(!gameControllers[i]) {
                        gameControllers[i] = SDL_GameControllerOpen(deviceIndex);
                        break;
                }
        }
}

static void closeController(SDL_JoystickID instanceId) {
        for(int i = 0; i < MaxControllers; i++) {
                if(!gameControllers[i]) continue;
                SDL_Joystick* joy = SDL_GameControllerGetJoystick(gameControllers[i]);
                if(SDL_JoystickInstanceID(joy) == instanceId) {
                        SDL_GameControllerClose(gameControllers[i]);
                        gameControllers[i] = nullptr;
                        return;
                }
        }
}

static SDL_GameController* firstController() {
        for(int i = 0; i < MaxControllers; i++) {
                if(gameControllers[i]) return gameControllers[i];
        }
        return nullptr;
}

// ── OSD rendering ───────────────────────────────────────────────────────────

static void osdRenderText(SDL_Renderer* r, const char* text, int x, int y) {
        if(!text || !text[0]) return;

        int len = 0;
        for(const char* p = text; *p; p++) len++;

        int charW = 8 * OSD_SCALE;
        int charH = 8 * OSD_SCALE;
        int pad = 4;

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_Rect bg = {x - pad, y - pad, len * charW + pad * 2, charH + pad * 2};
        SDL_SetRenderDrawColor(r, 0, 0, 0, 180);
        SDL_RenderFillRect(r, &bg);

        SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
        for(int ci = 0; text[ci]; ci++) {
                int ch = (u8)text[ci];
                if(ch < 32 || ch > 126) ch = '?';
                const u8* glyph = osdFont[ch - 32];
                int ox = x + ci * charW;
                for(int row = 0; row < 8; row++) {
                        u8 bits = glyph[row];
                        for(int col = 0; col < 8; col++) {
                                if(bits & (0x80 >> col)) {
                                        SDL_Rect px = {ox + col * OSD_SCALE, y + row * OSD_SCALE, OSD_SCALE, OSD_SCALE};
                                        SDL_RenderFillRect(r, &px);
                                }
                        }
                }
        }
}

// ── CLI ─────────────────────────────────────────────────────────────────────

static void printUsage() {
        std::cerr <<
        "Usage: aresdl [options] <rom> [rom2 ...]\n"
        "\n"
        "Options:\n"
        "  --system <name>    Force emulator (e.g. \"Super Famicom\", \"Game Boy\")\n"
        "  --fullscreen       Start in fullscreen\n"
        "  --scale <N>        Window size: 1-4 = height multiplier (4:3 ratio),\n"
        "                     0 = aspect-correct fit with letterboxing\n"
        "                     Default: stretch to fill 640x480 window\n"
        "  --hotkeys          Show gamepad hotkeys and button layout\n"
        << std::endl;
}

static void printHotkeys() {
        std::cerr <<
        "Gamepad hotkeys (hold Select/Back as modifier):\n"
        "\n"
        "  Select + L ............. Save state\n"
        "  Select + R ............. Load state\n"
        "  Select + DPad Up ....... Next save slot (1-9)\n"
        "  Select + DPad Down ..... Previous save slot (1-9)\n"
        "  Select + X (west) ...... Toggle fullscreen\n"
        "  Select + Y (north) ..... Screenshot\n"
        "  Select + Start ......... Quit\n"
        "\n"
        "Button layout (mapped by position, not label):\n"
        "\n"
        "  Gamepad            SNES    Genesis    N64\n"
        "  -------            ----    -------    ---\n"
        "  A (south)    ->    B       A          A\n"
        "  B (east)     ->    A       B          B\n"
        "  X (west)     ->    Y       X          -\n"
        "  Y (north)    ->    X       Y          -\n"
        "  LB           ->    L       -          L\n"
        "  RB           ->    R       C          R\n"
        "  Back/Select  ->    Select  Mode       -\n"
        "  Start        ->    Start   Start      Start\n"
        "  Left stick   ->    D-Pad   D-Pad      Analog\n"
        "  Right stick  ->    -       -          C-Buttons\n"
        "\n"
        "              [LB]                [RB]\n"
        "          .-----------------------------.\n"
        "          |                             |\n"
        "          |    .---.          [Y]       |\n"
        "          |    | U |       [X]   [B]   |\n"
        "          |  .-+---+-. .     [A]       |\n"
        "          |  |L|   |R|  Sel  Start     |\n"
        "          |  '-+---+-'                  |\n"
        "          |    | D |      (o)    (o)    |\n"
        "          |    '---'     L-Stk  R-Stk  |\n"
        "          |                             |\n"
        "          '-----------------------------'\n"
        "           D-Pad      Face buttons: SNES layout\n"
        << std::endl;
}

// ── Hotkey processing ───────────────────────────────────────────────────────

static bool processHotkeys(bool& running) {
        auto pad = firstController();
        if(!pad) return false;

        bool selectHeld = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_BACK);
        if(!selectHeld) {
                prevBtn = {};
                return false;
        }

        bool curL      = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        bool curR      = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        bool curDpadUp = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_UP);
        bool curDpadDn = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        bool curStart  = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_START);
        bool curX      = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_X);
        bool curY      = SDL_GameControllerGetButton(pad, SDL_CONTROLLER_BUTTON_Y);

        if(curStart && !prevBtn.start) running = false;
        if(curL && !prevBtn.l)         program.stateSave(program.state.slot);
        if(curR && !prevBtn.r)         program.stateLoad(program.state.slot);

        if(curDpadUp && !prevBtn.dpadUp) {
                program.state.slot = program.state.slot < 9 ? program.state.slot + 1 : 1;
                program.showMessage({"Save slot: ", program.state.slot});
        }
        if(curDpadDn && !prevBtn.dpadDown) {
                program.state.slot = program.state.slot > 1 ? program.state.slot - 1 : 9;
                program.showMessage({"Save slot: ", program.state.slot});
        }
        if(curX && !prevBtn.x) {
                toggleFullscreen();
                program.showMessage(isFullscreen ? "Fullscreen" : "Windowed");
        }
        if(curY && !prevBtn.y) {
                if(textureWidth && textureHeight && !framebuffer.empty()) {
                        program.captureScreenshot(framebuffer.data(), fbPitch, fbWidth, fbHeight);
                }
        }

        prevBtn = {curL, curR, curDpadUp, curDpadDn, curStart, curX, curY};
        return true;
}

// ── Main ────────────────────────────────────────────────────────────────────

auto nall::main(Arguments arguments) -> void {

        if(arguments.find("--hotkeys")) {
                printHotkeys();
                return;
        }

        if(string system; arguments.take("--system", system)) {
                program.startSystem = system;
        }

        if(string scaleStr; arguments.take("--scale", scaleStr)) {
                scaleMultiplier = scaleStr.integer();
                if(scaleMultiplier < 0 || scaleMultiplier > 4) scaleMultiplier = 0;
        }

        if(arguments.find("--fullscreen")) {
                arguments.take("--fullscreen");
                isFullscreen = true;
        }

        std::vector<string> gameFiles;
        for(auto argument : arguments) {
                if(file::exists(argument)) gameFiles.push_back(argument);
        }

        if(gameFiles.empty()) {
                printUsage();
                return;
        }

        if(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) < 0) {
                std::cerr << "SDL init failed: " << SDL_GetError() << std::endl;
                return;
        }

        //audio: 48kHz stereo float32, push model
        SDL_AudioSpec desired = {};
        desired.freq = 48000;
        desired.format = AUDIO_F32SYS;
        desired.channels = 2;
        desired.samples = 1024;
        desired.callback = NULL;
        SDL_AudioSpec obtained;
        sdlAudioDevice = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
        if(sdlAudioDevice) {
                SDL_PauseAudioDevice(sdlAudioDevice, 0);
        } else {
                std::cerr << "SDL audio failed: " << SDL_GetError() << std::endl;
        }

        ares::Memory::FixedAllocator::get();
        inputManager.create();
        Emulator::construct();

        print("Systems: ");
        for(auto& emulator : emulators) print(emulator->name, ", ");
        print("\n");

        program.startGameLoad = gameFiles;
        program.create();

        for(int i = 0; i < SDL_NumJoysticks(); i++) openController(i);

        u32 windowFlags = SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE;
        if(isFullscreen) windowFlags |= SDL_WINDOW_FULLSCREEN_DESKTOP;

        sdlWindow = SDL_CreateWindow("areSDL",
                SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                640, 480, windowFlags);
        if(!sdlWindow) {
                std::cerr << "Window creation failed: " << SDL_GetError() << std::endl;
                SDL_Quit();
                return;
        }
        SDL_GetWindowSize(sdlWindow, &windowWidth, &windowHeight);

        renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_ACCELERATED);
        if(!renderer) renderer = SDL_CreateRenderer(sdlWindow, -1, SDL_RENDERER_SOFTWARE);
        if(!renderer) {
                std::cerr << "Renderer creation failed: " << SDL_GetError() << std::endl;
                SDL_DestroyWindow(sdlWindow);
                SDL_Quit();
                return;
        }

        bool running = true;
        SDL_Event ev;

        while(running) {
                while(SDL_PollEvent(&ev)) {
                        switch(ev.type) {
                        case SDL_QUIT:
                                running = false;
                                break;
                        case SDL_WINDOWEVENT:
                                if(ev.window.event == SDL_WINDOWEVENT_CLOSE) running = false;
                                if(ev.window.event == SDL_WINDOWEVENT_RESIZED ||
                                   ev.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                                        windowWidth = ev.window.data1;
                                        windowHeight = ev.window.data2;
                                        recalcDestRect();
                                }
                                break;
                        case SDL_CONTROLLERDEVICEADDED:
                                openController(ev.cdevice.which);
                                break;
                        case SDL_CONTROLLERDEVICEREMOVED:
                                closeController(ev.cdevice.which);
                                break;
                        }
                }
                if(!running) break;

                processHotkeys(running);
                if(!running) break;

                //audio-driven frame pacing (~50ms buffer target)
                if(sdlAudioDevice) {
                        constexpr u32 maxQueue = 48000 * 2 * sizeof(float) / 20;
                        while(SDL_GetQueuedAudioSize(sdlAudioDevice) > maxQueue) SDL_Delay(1);
                } else {
                        SDL_Delay(16);
                }

                program.main();

                //update texture from framebuffer
                if(fbReady) {
                        if(fbWidth != textureWidth || fbHeight != textureHeight) {
                                if(texture) SDL_DestroyTexture(texture);
                                texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888,
                                        SDL_TEXTUREACCESS_STREAMING, fbWidth, fbHeight);
                                textureWidth = fbWidth;
                                textureHeight = fbHeight;
                                applyScaleResize();
                                recalcDestRect();
                        }
                        if(texture) SDL_UpdateTexture(texture, NULL, framebuffer.data(), fbPitch);
                        fbReady = false;
                }

                //render
                SDL_RenderClear(renderer);
                if(texture) SDL_RenderCopy(renderer, texture, NULL, useDestRect ? &destRect : NULL);

                //OSD overlay
                if(!program.messages.empty()) {
                        auto& msg = program.messages.back();
                        u64 age = chrono::millisecond() - msg.timestamp;
                        if(age < OSD_DISPLAY_MS) {
                                int ax = useDestRect ? destRect.x : 0;
                                int ay = useDestRect ? destRect.y : 0;
                                int ah = useDestRect ? destRect.h : windowHeight;
                                int osdY = ay + ah - OSD_MARGIN - 8 * OSD_SCALE;
                                if(osdY < 0) osdY = OSD_MARGIN;
                                osdRenderText(renderer, (const char*)msg.text, ax + OSD_MARGIN, osdY);
                        }
                }

                SDL_RenderPresent(renderer);
        }

        //cleanup
        for(int i = 0; i < MaxControllers; i++) {
                if(gameControllers[i]) { SDL_GameControllerClose(gameControllers[i]); gameControllers[i] = nullptr; }
        }
        if(texture) SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer); renderer = nullptr;
        if(sdlAudioDevice) SDL_CloseAudioDevice(sdlAudioDevice);
        SDL_DestroyWindow(sdlWindow); sdlWindow = nullptr;
        SDL_Quit();
}
