#include "game/game.h"

struct sprite_goblin {
  struct sprite hdr;
  double animclock;
  int animframe;
  uint8_t tileid0;
};

#define SPRITE ((struct sprite_goblin*)sprite)

static int _goblin_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  return 0;
}

static void _goblin_update(struct sprite *sprite,double elapsed) {

  // Marching animation, even if we're standing still.
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.200;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    switch (SPRITE->animframe) {
      case 0: sprite->tileid=SPRITE->tileid0+1; break;
      case 1: sprite->tileid=SPRITE->tileid0; break;
      case 2: sprite->tileid=SPRITE->tileid0+2; break;
      case 3: sprite->tileid=SPRITE->tileid0; break;
    }
  }
  
  //TODO Determine legal extents of motion, walk back and forth.
}

const struct sprite_type sprite_type_goblin={
  .name="goblin",
  .objlen=sizeof(struct sprite_goblin),
  .init=_goblin_init,
  .update=_goblin_update,
};
