#include "game/game.h"

#define ARROW_SPEED 12.0

struct sprite_arrow {
  struct sprite hdr;
  double dx;
  int goblinc;
  int stuck;
};

#define SPRITE ((struct sprite_arrow*)sprite)

static int _arrow_init(struct sprite *sprite) {
  sprite->tileid=0x60;
  return 0;
}

void arrow_setup(struct sprite *sprite,double dx) {
  if (!sprite||(sprite->type!=&sprite_type_arrow)) return;
  SPRITE->dx=dx;
  if (dx<0.0) {
    sprite->xform=EGG_XFORM_XREV;
  } else {
    sprite->xform=0;
  }
}

static void arrow_check_skewer(struct sprite *sprite) {
  int i=g.spritec;
  while (i-->0) {
    struct sprite *dumpling=g.spritev[i];
    if (dumpling->defunct) continue;
    if (dumpling->type==&sprite_type_goblin) {
      if (sprite->x<dumpling->x+dumpling->phl) continue;
      if (sprite->x>dumpling->x+dumpling->phr) continue;
      if (sprite->y<dumpling->y+dumpling->pht) continue;
      if (sprite->y>dumpling->y+dumpling->phb) continue;
      dumpling->defunct=1;
      SPRITE->goblinc++;
      if (SPRITE->dx<0.0) {
        if (dumpling->x<sprite->x) {
          sprite->x=dumpling->x;
        }
      } else {
        if (dumpling->x>sprite->x) {
          sprite->x=dumpling->x;
        }
      }
    }
  }
}

static void _arrow_update(struct sprite *sprite,double elapsed) {
  if (SPRITE->stuck) return;
  sprite->x+=SPRITE->dx*ARROW_SPEED*elapsed;
  if ((sprite->x<-5.0)||(sprite->x>COLC+5.0)) {
    sprite->defunct=1;
  } else {
    arrow_check_skewer(sprite);
    int col=(int)sprite->x,row=(int)sprite->y;
    if ((col>=0)&&(row>=0)&&(col<COLC)&&(row<ROWC)) {
      uint8_t physics=g.physics[g.map[row*COLC+col]];
      if (physics==NS_physics_solid) {
        SPRITE->stuck=1;
        if (SPRITE->dx<0.0) {
          sprite->x=col+0.875;
        } else {
          sprite->x=col+0.125;
        }
      }
    }
  }
}

static void _arrow_render(struct sprite *sprite,int x,int y) {
  // (x,y) is the tip of the arrow.
  if (sprite->xform) { // facing left
    x+=2;
    if (SPRITE->goblinc) {
      int i=SPRITE->goblinc;
      for (;i-->0;x+=4) {
        graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid+2,sprite->xform);
      }
    } else {
      graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid+1,sprite->xform);
      x+=4;
    }
    graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid,sprite->xform);
  } else { // facing right
    x-=2;
    if (SPRITE->goblinc) {
      int i=SPRITE->goblinc;
      for (;i-->0;x-=4) {
        graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid+2,sprite->xform);
      }
    } else {
      graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid+1,sprite->xform);
      x-=4;
    }
    graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid,sprite->xform);
  }
}

const struct sprite_type sprite_type_arrow={
  .name="arrow",
  .objlen=sizeof(struct sprite_arrow),
  .init=_arrow_init,
  .update=_arrow_update,
  .render=_arrow_render,
};

int arrow_finished(const struct sprite *sprite) {
  if (!sprite||(sprite->type!=&sprite_type_arrow)) return 0;
  return SPRITE->stuck;
}
