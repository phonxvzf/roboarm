#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL_image.h>

#define ARM_WIDTH   10
#define ARM_LENGTH  283

SDL_Window* window;
SDL_Renderer* renderer;

void clean_after_init() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

double rad_to_deg(double rad) {
  return 180.f / M_PI * rad;
}

double pow2(double x) {
  return x * x;
}

void draw_arm(SDL_Texture* texture, double rad, int pivot_x, int pivot_y) {
  SDL_Rect drect;
  drect.x = pivot_x - ARM_LENGTH; drect.y = pivot_y - ARM_WIDTH;
  drect.w = ARM_LENGTH; drect.h = ARM_WIDTH;
  SDL_Point rel_pivot;
  rel_pivot.x = drect.w; rel_pivot.y = drect.h / 2;
  SDL_RenderCopyEx(
      renderer, texture, NULL, &drect, rad_to_deg(M_PI + rad), &rel_pivot, SDL_FLIP_NONE
      );
}

SDL_Point pivot_position(double rad0, SDL_Point* p0) {
  SDL_Point ret;
  ret.x = p0->x + cos(rad0) * ARM_LENGTH; ret.y = p0->y + sin(rad0) * ARM_LENGTH;
  return ret;
}

void solve_ik(double* angle0, double* angle1, SDL_Point* p0, SDL_Point* target) {
  double dist2 = pow2(target->x - p0->x) + pow2(target->y - p0->y);
  double beta = acos((dist2 - 2 * pow2(ARM_LENGTH)) / (-2 * pow2(ARM_LENGTH)));
  double a = (M_PI - beta) / 2;
  double a_prime = atan2(target->y - p0->y, target->x - p0->x);
  double alpha = a + a_prime;

  *angle0 = alpha;
  *angle1 = (beta + alpha) - M_PI;
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
      "roboarm", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, 0
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

  SDL_Texture* arm_texture[2];
  arm_texture[0] = SDL_CreateTextureFromSurface(renderer, pixel_surface);

  SDL_FillRect(pixel_surface, NULL, SDL_MapRGBA(pixel_surface->format, 0xaa, 0xaa, 0xaa, 0xff));
  arm_texture[1] = SDL_CreateTextureFromSurface(renderer, pixel_surface);

  SDL_FreeSurface(pixel_surface);

  uint8_t running = true;
  SDL_Event e;
  SDL_Point mouse;
  SDL_Point pivot0, pivot1;
  pivot0.x = 400; pivot0.y = 400;

  while (running) {
    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          if (e.key.keysym.sym == SDLK_q) running = false;
          break;
        case SDL_MOUSEMOTION:
          mouse.x = e.motion.x;
          mouse.y = e.motion.y;
          break;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);

    double angle0, angle1;
    solve_ik(&angle0, &angle1, &pivot0, &mouse);
    draw_arm(arm_texture[0], angle0, pivot0.x, pivot0.y);

    pivot1 = pivot_position(angle0, &pivot0);
    draw_arm(arm_texture[1], angle1, pivot1.x, pivot1.y);

    SDL_RenderPresent(renderer);
  }

  SDL_DestroyTexture(arm_texture[0]);
  SDL_DestroyTexture(arm_texture[1]);

  clean_after_init();
  return 0;
}
