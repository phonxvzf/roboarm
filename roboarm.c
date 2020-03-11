#include <stdio.h>
#include <stdbool.h>
#include <math.h>

#include <SDL2/SDL_image.h>

#define ARM_WIDTH   10
#define ARM_LENGTH  283
#define ANIM_SPEED  200 // pixels per sec

SDL_Window* window;
SDL_Renderer* renderer;

typedef struct {
  double x, y;
} Point2f;

void clean_after_init() {
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

double rad_to_deg(double rad) {
  return 180 / M_PI * rad;
}

double pow2(double x) {
  return x * x;
}

double distance(Point2f* a, Point2f* b) {
  return sqrt(pow2(a->x - b->x) + pow2(a->y - b->y));
}

double distance2(Point2f* a, Point2f* b) {
  return pow2(a->x - b->x) + pow2(a->y - b->y);
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

void solve_ik(double* angle0, double* angle1, SDL_Point* p0, Point2f* target) {
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

  SDL_Surface* cross_surface = IMG_Load("cross.png");
  if (cross_surface == NULL) {
    fprintf(stderr, "error: could not load cross.png: %s\n", IMG_GetError());
    SDL_FreeSurface(pixel_surface);
    clean_after_init();
    return 1;
  }

  SDL_Texture* arm_texture[2];
  arm_texture[0] = SDL_CreateTextureFromSurface(renderer, pixel_surface);

  SDL_FillRect(pixel_surface, NULL, SDL_MapRGBA(pixel_surface->format, 0xaa, 0xaa, 0xaa, 0xff));
  arm_texture[1] = SDL_CreateTextureFromSurface(renderer, pixel_surface);

  SDL_FreeSurface(pixel_surface);

  SDL_Texture* cross_texture = SDL_CreateTextureFromSurface(renderer, cross_surface);

  uint8_t running = true;
  uint8_t animating = false;
  uint8_t recv_mouse = true;
  SDL_Event e;
  SDL_Point mouse;
  SDL_Point pivot0, pivot1;
  pivot0.x = 400; pivot0.y = 400;

  Point2f keys[32];
  Point2f current_target;
  int n_keys = 0;
  int current_key = 0;
  double dt;
  double anim_dist;

  uint32_t tick = SDL_GetTicks();

  while (running) {
    tick = SDL_GetTicks();

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0xff);
    SDL_RenderClear(renderer);

    while (SDL_PollEvent(&e)) {
      switch (e.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          if (e.key.keysym.sym == SDLK_q) running = false;
          if (e.key.keysym.sym == SDLK_r) {
            recv_mouse = true;
            animating = false;
            n_keys = 0;
          }
          if (e.key.keysym.sym == SDLK_SPACE) {
            if (n_keys > 0) {
              animating = true;
              recv_mouse = false;
              current_key = 0;
              current_target.x = keys[0].x;
              current_target.y = keys[0].y;
              anim_dist = 0;
            }
          }
          break;
        case SDL_MOUSEMOTION:
          mouse.x = e.motion.x;
          mouse.y = e.motion.y;
          break;
        case SDL_MOUSEBUTTONDOWN:
          if (n_keys < 32) {
            int mx, my;
            SDL_GetMouseState(&mx, &my);
            keys[n_keys].x = mx;
            keys[n_keys].y = my;
            ++n_keys;
          }
          break;
      }
    }

    if (animating) {
      if (current_key + 1 < n_keys) {
        double move_dist =
          (ANIM_SPEED + 0.5 * distance(&keys[current_key], &keys[current_key+1])) * dt;
        double dir_x = (keys[current_key + 1].x - keys[current_key].x);
        double dir_y = (keys[current_key + 1].y - keys[current_key].y);
        double theta = atan2(dir_y, dir_x);
        current_target.x += move_dist * cos(theta);
        current_target.y += move_dist * sin(theta);
        anim_dist += move_dist;
        if (pow2(anim_dist) >= distance2(&keys[current_key], &keys[current_key+1])) {
          anim_dist = 0;
          ++current_key;
        }
      } else {
        animating = false;
        recv_mouse = false;
      }
    }

    Point2f target;
    target.x = recv_mouse ? mouse.x : current_target.x;
    target.y = recv_mouse ? mouse.y : current_target.y;

    double angle0, angle1;
    solve_ik(&angle0, &angle1, &pivot0, &target);
    draw_arm(arm_texture[0], angle0, pivot0.x, pivot0.y);

    pivot1 = pivot_position(angle0, &pivot0);
    draw_arm(arm_texture[1], angle1, pivot1.x, pivot1.y);

    SDL_Rect cross_rect;
    cross_rect.w = 16; cross_rect.h = 16;
    for (int i = 0; i < n_keys; ++i) {
      cross_rect.x = keys[i].x - 8; cross_rect.y = keys[i].y - 8;
      SDL_RenderCopy(renderer, cross_texture, NULL, &cross_rect);
    }

    SDL_RenderPresent(renderer);
    dt = (SDL_GetTicks() - tick) * 0.001f;
  }

  SDL_DestroyTexture(arm_texture[0]);
  SDL_DestroyTexture(arm_texture[1]);
  SDL_DestroyTexture(cross_texture);

  clean_after_init();
  return 0;
}
