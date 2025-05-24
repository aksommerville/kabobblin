#include "game/game.h"

#define MMM_TTL 0.750
#define MMM_RADIUS_A 0.500
#define MMM_RADIUS_Z 2.000
// The angle of the two outside tiles doesn't change, might as well precalculate.
#define MMM_COST 0.5403023058681398
#define MMM_SINT -0.8414709848078965

struct sprite_mmm {
  struct sprite hdr;
  double clock;
};

#define SPRITE ((struct sprite_mmm*)sprite)

static void _mmm_update(struct sprite *sprite,double elapsed) {
  SPRITE->clock+=elapsed;
  if (SPRITE->clock>=MMM_TTL) sprite->defunct=1;
}

static void _mmm_render(struct sprite *sprite,int x,int y) {
  double radius=MMM_RADIUS_A+((SPRITE->clock*(MMM_RADIUS_Z-MMM_RADIUS_A))/MMM_TTL);
  radius*=NS_sys_tilesize;
  graf_draw_tile(&g.graf,g.texid_tiles,x,y-(int)radius,0x77,0);
  int dx=(int)(MMM_COST*radius);
  int dy=(int)(MMM_SINT*radius);
  graf_draw_tile(&g.graf,g.texid_tiles,x+dx,y+dy,0x77,0);
  graf_draw_tile(&g.graf,g.texid_tiles,x-dx,y+dy,0x77,0);
}

const struct sprite_type sprite_type_moneymoneymoney={
  .name="moneymoneymoney",
  .objlen=sizeof(struct sprite_mmm),
  .update=_mmm_update,
  .render=_mmm_render,
};
