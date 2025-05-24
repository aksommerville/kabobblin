#include "game/game.h"

#define TREASURE_COLLECT_DISTANCE 0.250

struct sprite_treasure {
  struct sprite hdr;
  uint8_t tileid0;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_treasure*)sprite)

static int _treasure_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  return 0;
}

static void _treasure_update(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.250;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
    sprite->tileid=SPRITE->tileid0+SPRITE->animframe;
  }
  if (g.hero) {
    double dx=g.hero->x-sprite->x;
    if ((dx<-TREASURE_COLLECT_DISTANCE)||(dx>TREASURE_COLLECT_DISTANCE)) return;
    double dy=g.hero->y-sprite->y;
    if ((dy<-TREASURE_COLLECT_DISTANCE)||(dy>TREASURE_COLLECT_DISTANCE)) return;
    sprite->defunct=1;
    g.got_treasure=1;
    //TODO sound effect
  }
}

const struct sprite_type sprite_type_treasure={
  .name="treasure",
  .objlen=sizeof(struct sprite_treasure),
  .init=_treasure_init,
  .update=_treasure_update,
};
