#pragma once
// The Codex Linux image contains the SDL2 runtime but not the development
// headers. This file declares only the stable SDL2 ABI used by the validation
// shell. Gameplay and presentation state remain in px_core.
#include <cstdint>

using Uint8 = std::uint8_t;
using Uint16 = std::uint16_t;
using Uint32 = std::uint32_t;
using Sint16 = std::int16_t;
using Sint32 = std::int32_t;

struct SDL_Window;
struct SDL_Renderer;
struct SDL_GameController;
struct SDL_Texture;

struct SDL_Rect { int x; int y; int w; int h; };
struct SDL_FPoint { float x; float y; };
struct SDL_Color { Uint8 r; Uint8 g; Uint8 b; Uint8 a; };
struct SDL_Vertex { SDL_FPoint position; SDL_Color color; SDL_FPoint tex_coord; };
struct SDL_Keysym { Sint32 scancode; Sint32 sym; Uint16 mod; Uint32 unused; };
struct SDL_KeyboardEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint8 state;
    Uint8 repeat;
    Uint8 padding2;
    Uint8 padding3;
    SDL_Keysym keysym;
};
struct SDL_ControllerButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 padding1;
    Uint8 padding2;
};
struct SDL_ControllerDeviceEvent {
    Uint32 type;
    Uint32 timestamp;
    Sint32 which;
};
struct SDL_MouseButtonEvent {
    Uint32 type;
    Uint32 timestamp;
    Uint32 windowID;
    Uint32 which;
    Uint8 button;
    Uint8 state;
    Uint8 clicks;
    Uint8 padding1;
    Sint32 x;
    Sint32 y;
};
union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_ControllerButtonEvent cbutton;
    SDL_ControllerDeviceEvent cdevice;
    SDL_MouseButtonEvent button;
    Uint8 padding[56];
};

extern "C" {
int SDL_Init(Uint32 flags);
void SDL_Quit();
const char* SDL_GetError();
int SDL_SetHint(const char* name, const char* value);
SDL_Window* SDL_CreateWindow(const char* title, int x, int y, int w, int h, Uint32 flags);
void SDL_DestroyWindow(SDL_Window* window);
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, int index, Uint32 flags);
void SDL_DestroyRenderer(SDL_Renderer* renderer);
int SDL_RenderSetLogicalSize(SDL_Renderer* renderer, int w, int h);
int SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, int blendMode);
int SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
int SDL_RenderClear(SDL_Renderer* renderer);
int SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_Rect* rect);
int SDL_RenderDrawRect(SDL_Renderer* renderer, const SDL_Rect* rect);
int SDL_RenderDrawLine(SDL_Renderer* renderer, int x1, int y1, int x2, int y2);
int SDL_RenderDrawPoint(SDL_Renderer* renderer, int x, int y);
int SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture,
                       const SDL_Vertex* vertices, int vertexCount,
                       const int* indices, int indexCount);
void SDL_RenderPresent(SDL_Renderer* renderer);
int SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect, Uint32 format, void* pixels, int pitch);
int SDL_PollEvent(SDL_Event* event);
Uint32 SDL_GetTicks();
void SDL_Delay(Uint32 milliseconds);
int SDL_NumJoysticks();
int SDL_IsGameController(int joystickIndex);
SDL_GameController* SDL_GameControllerOpen(int joystickIndex);
void SDL_GameControllerClose(SDL_GameController* controller);
Sint16 SDL_GameControllerGetAxis(SDL_GameController* controller, int axis);
Uint8 SDL_GameControllerGetButton(SDL_GameController* controller, int button);
}

constexpr Uint32 SDL_INIT_VIDEO = 0x00000020u;
constexpr Uint32 SDL_INIT_GAMECONTROLLER = 0x00002000u;
constexpr Uint32 SDL_WINDOW_SHOWN = 0x00000004u;
constexpr Uint32 SDL_WINDOW_HIDDEN = 0x00000008u;
constexpr Uint32 SDL_WINDOW_RESIZABLE = 0x00000020u;
constexpr int SDL_WINDOWPOS_CENTERED = 0x2FFF0000;
constexpr Uint32 SDL_RENDERER_SOFTWARE = 0x00000001u;
constexpr Uint32 SDL_RENDERER_ACCELERATED = 0x00000002u;
constexpr Uint32 SDL_RENDERER_PRESENTVSYNC = 0x00000004u;
constexpr int SDL_BLENDMODE_BLEND = 1;
constexpr Uint32 SDL_PIXELFORMAT_ARGB8888 = 0x16362004u;

constexpr Uint32 SDL_QUIT = 0x100u;
constexpr Uint32 SDL_KEYDOWN = 0x300u;
constexpr Uint32 SDL_KEYUP = 0x301u;
constexpr Uint32 SDL_MOUSEBUTTONDOWN = 0x401u;
constexpr Uint32 SDL_MOUSEBUTTONUP = 0x402u;
constexpr Uint32 SDL_CONTROLLERBUTTONDOWN = 0x651u;
constexpr Uint32 SDL_CONTROLLERBUTTONUP = 0x652u;
constexpr Uint32 SDL_CONTROLLERDEVICEADDED = 0x653u;
constexpr Uint32 SDL_CONTROLLERDEVICEREMOVED = 0x654u;

constexpr Sint32 SDLK_RETURN = 13;
constexpr Sint32 SDLK_ESCAPE = 27;
constexpr Sint32 SDLK_SPACE = 32;
constexpr Sint32 SDLK_BACKSPACE = 8;
constexpr Sint32 SDLK_LEFT = 1073741904;
constexpr Sint32 SDLK_RIGHT = 1073741903;
constexpr Sint32 SDLK_DOWN = 1073741905;
constexpr Sint32 SDLK_UP = 1073741906;
constexpr Sint32 SDLK_LSHIFT = 1073742049;
constexpr Sint32 SDLK_RSHIFT = 1073742053;
constexpr Uint8 SDL_BUTTON_LEFT = 1;
constexpr Uint8 SDL_BUTTON_RIGHT = 3;

constexpr Uint8 SDL_CONTROLLER_BUTTON_A = 0;
constexpr Uint8 SDL_CONTROLLER_BUTTON_B = 1;
constexpr Uint8 SDL_CONTROLLER_BUTTON_X = 2;
constexpr Uint8 SDL_CONTROLLER_BUTTON_Y = 3;
constexpr Uint8 SDL_CONTROLLER_BUTTON_START = 6;
constexpr Uint8 SDL_CONTROLLER_BUTTON_LEFTSHOULDER = 9;
constexpr Uint8 SDL_CONTROLLER_BUTTON_RIGHTSHOULDER = 10;
constexpr Uint8 SDL_CONTROLLER_BUTTON_DPAD_UP = 11;
constexpr Uint8 SDL_CONTROLLER_BUTTON_DPAD_DOWN = 12;
constexpr Uint8 SDL_CONTROLLER_BUTTON_DPAD_LEFT = 13;
constexpr Uint8 SDL_CONTROLLER_BUTTON_DPAD_RIGHT = 14;

constexpr int SDL_CONTROLLER_AXIS_LEFTX = 0;
constexpr int SDL_CONTROLLER_AXIS_LEFTY = 1;
constexpr int SDL_CONTROLLER_AXIS_RIGHTX = 2;
constexpr int SDL_CONTROLLER_AXIS_RIGHTY = 3;
constexpr int SDL_CONTROLLER_AXIS_TRIGGERLEFT = 4;
constexpr int SDL_CONTROLLER_AXIS_TRIGGERRIGHT = 5;
