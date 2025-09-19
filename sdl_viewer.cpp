#include "sdl_viewer.h"

#include <SDL.h>
#include <stdio.h>

#include <cstdint>
#include <cstdlib>
#include <vector>

StatusOr<SDLViewer> SDLViewer::open(std::shared_ptr<BounceDeskClient> client) {
  SDLViewer viewer;
  viewer.client_ = client;
  viewer.app_loop_ = std::thread(&SDLViewer::app_loop, &viewer);
  return viewer;
}

SDLViewer::SDLViewer(SDLViewer&& other) {
  exit_loop_ = other.exit_loop_.load();
  was_closed_ = other.was_closed_.load();
  client_ = std::move(other.client_);
  app_loop_ = std::move(other.app_loop_);
}

SDLViewer::~SDLViewer() { close(); }

void SDLViewer::close() {
  exit_loop_ = true;
  if (app_loop_.joinable()) {
    app_loop_.join();
  }
}

void SDLViewer::app_loop() {
  SDL_Window* window = nullptr;
  SDL_Renderer* renderer = nullptr;
  SDL_Texture* texture = nullptr;

  auto check = [&](auto v, std::string msg) {
    if (v == 0) {
      fprintf(stderr, "%s failed: %s\n", msg.c_str(), SDL_GetError());
      fprintf(stderr, "SDLViewer is exiting.\n");
      if (renderer) SDL_DestroyRenderer(renderer);
      if (window) SDL_DestroyWindow(window);
      SDL_Quit();
    }
  };

  check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER), "SDL_Init");

  int w = 1, h = 1;
  window = SDL_CreateWindow("Bounce Viewer", SDL_WINDOWPOS_CENTERED,
                            SDL_WINDOWPOS_CENTERED, w, h, SDL_WINDOW_SHOWN);
  check(window, "SDL_CreateWindow");
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  check(renderer, "SDL_CreateRenderer");
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                              SDL_TEXTUREACCESS_STREAMING, w, h);
  check(texture, "SDL_CreateTexture");

  const int FPS = 30;
  const uint32_t frame_ms = 1000 / FPS;
  while (!exit_loop_) {
    const uint32_t frame_start = SDL_GetTicks();
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
      if (e.type == SDL_QUIT) {
        exit_loop_ = true;
      }
    }

    Frame f = client_->get_frame();
    if (f.width != w || f.height != h) {
      w = f.width;
      h = f.height;
      SDL_SetWindowSize(window, w, h);
      if (texture) SDL_DestroyTexture(texture);
      texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                  SDL_TEXTUREACCESS_STREAMING, w, h);
      check(texture, "SDL_CreateTexture (resize)");
    }

    const int pitch = f.width * 4;
    check(SDL_UpdateTexture(texture, nullptr, f.pixels.get(), pitch),
          "SDL_UpdateTexture");
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, nullptr, nullptr);
    SDL_RenderPresent(renderer);
    const uint32_t elapsed = SDL_GetTicks() - frame_start;
    if (elapsed < frame_ms) {
      SDL_Delay(frame_ms - elapsed);
    }
  }
  if (texture) SDL_DestroyTexture(texture);
  if (renderer) SDL_DestroyRenderer(renderer);
  if (window) SDL_DestroyWindow(window);
  SDL_Quit();
  was_closed_ = true;
}
