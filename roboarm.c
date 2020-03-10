#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL_image.h>

#define ARM_WIDTH   10
#define ARM_LENGTH  100

SDL_Window* window;
SDL_Renderer* renderer;

void clean_after_init() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

void draw_arm(SDL_Texture* texture, double deg, int pivot_x, int pivot_y) {
  SDL_Rect drect;
  drect.x = pivot_x - ARM_LENGTH; drect.y = pivot_y - ARM_WIDTH;
  drect.w = ARM_LENGTH; drect.h = ARM_WIDTH;
  SDL_Point rel_pivot;
  rel_pivot.x = drect.w; rel_pivot.y = drect.h / 2;
  SDL_RenderCopyEx(renderer, texture, NULL, &drect, deg, &rel_pivot, SDL_FLIP_NONE);
}

int main() {
  if (SDL_Init(SDL_INIT_VIDEO)) {
    fprintf(stderr, "error: could not initialize SDL: %s\n", SDL_GetError());
    return 1;
  }
  if (IMG_Init(IMG_INIT_PNG) != IMG_INIT_PNG) {
    fprintf(stderr, "error: could not initialize SDL_image: %s\n", IMG_GetError());
    return 1;
  }

  window = SDL_CreateWindow(
      "roboarm", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, 0
      );
  renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC
      );

  SDL_Surface* pixel_surface = IMG_Load("pixel.png");
  if (pixel_surface == NULL) {
    fprintf(stderr, "error: could not load pixel.png: %s\n", IMG_GetError());
    clean_after_init();
    return 1;
  }

  SDL_Texture* arm_texture;
  arm_texture = SDL_CreateTextureFromSurface(renderer, pixel_surface);

  SDL_FreeSurface(pixel_surface);

  uint8_t running = true;
  SDL_Event e;
  while (running) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          if (e.key.keysym.sym == SDLK_q) running = false;
          break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    draw_arm(arm_texture, 45, 400, 300);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(arm_texture);

  clean_after_init();
  return 0;
}
