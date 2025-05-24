#include "game/game.h"

#define HERO_WALK_SPEED 6.0 /* m/s */
#define HERO_GRAVITY_RATE 18.0 /* m/s**2 */
#define HERO_GRAVITY_LIMIT 10.0 /* m/s */
#define HERO_GRAVITY_LIMIT_DRAG 2.0 /* m/s */
#define HERO_JUMP_DEFAULT 14.0 /* m/s, maximum (initial) jump velocity */
#define HERO_JUMP_DECAY 78.0 /* m/s**2 */

/* Instance definition.
 */
 
struct sprite_hero {
  struct sprite hdr;
  int grounded;
  double gravity; // m/s, volatile
  double jump_power; // m/s, volatile
  int jumping;
  int jump_blackout; // Waiting for SOUTH to release.
  int walking;
  int dragging;
  double animclock;
  int animframe;
};

#define SPRITE ((struct sprite_hero*)sprite)

/* Cleanup.
 */
 
static void _hero_del(struct sprite *sprite) {
}

/* Init.
 */
 
static int _hero_init(struct sprite *sprite) {
  if (!g.hero) g.hero=sprite;
  sprite->pht=-0.45;
  sprite->phl=-0.45;
  sprite->phr=0.45;
  SPRITE->grounded=1;
  SPRITE->jump_power=HERO_JUMP_DEFAULT;
  return 0;
}

/* Begin jumping.
 */
 
static void hero_begin_jump(struct sprite *sprite) {
  if (SPRITE->dragging) {
    SPRITE->jump_power=HERO_JUMP_DEFAULT;
  } else {
    if (SPRITE->jump_power<=0.0) return;
  }
  //TODO sound effect
  SPRITE->jumping=1;
  SPRITE->grounded=0;
  SPRITE->jump_blackout=1;
}

/* Terminate jump.
 */
 
static void hero_end_jump(struct sprite *sprite) {
  SPRITE->jumping=0;
  SPRITE->jump_power=0.0;
}

/* Continue jumping.
 */
 
static void hero_update_jump(struct sprite *sprite,double elapsed) {
  sprite->y-=SPRITE->jump_power*elapsed;
  physics_rectify_sprite(sprite,0.0,1.0);
  SPRITE->jump_power-=elapsed*HERO_JUMP_DECAY;
  if (SPRITE->jump_power<=0.0) {
    SPRITE->jumping=0;
  }
}

/* Begin falling.
 */
 
static void hero_fall(struct sprite *sprite) {
  SPRITE->grounded=0;
  SPRITE->jump_power=0.0;
}

/* End a fall.
 */
 
static void hero_land(struct sprite *sprite) {
  SPRITE->grounded=1;
  SPRITE->gravity=0.0;
  SPRITE->jump_power=HERO_JUMP_DEFAULT;
}

/* Start the walking animation if we were still.
 */
 
static void hero_is_walking(struct sprite *sprite,double elapsed) {
  if (!SPRITE->walking) {
    SPRITE->walking=1;
    SPRITE->animclock=0.250;
    SPRITE->animframe=1;
  }
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.250;
    if (++(SPRITE->animframe)>=4) SPRITE->animframe=0;
  }
}

static void hero_not_walking(struct sprite *sprite) {
  if (!SPRITE->walking) return;
  SPRITE->walking=0;
  SPRITE->animframe=0;
}

/* Update.
 */
 
static void _hero_update(struct sprite *sprite,double elapsed) {

  /* Walk horizontally.
   */
  SPRITE->dragging=0;
  switch (g.input&(EGG_BTN_LEFT|EGG_BTN_RIGHT)) {
    case EGG_BTN_LEFT: {
        hero_is_walking(sprite,elapsed);
        sprite->xform=EGG_XFORM_XREV;
        sprite->x-=HERO_WALK_SPEED*elapsed;
        if (physics_rectify_sprite(sprite,1.0,0.0)) {
          if (!SPRITE->grounded) SPRITE->dragging=1;
        }
      } break;
    case EGG_BTN_RIGHT: {
        hero_is_walking(sprite,elapsed);
        sprite->xform=0;
        sprite->x+=HERO_WALK_SPEED*elapsed;
        if (physics_rectify_sprite(sprite,-1.0,0.0)) {
          if (!SPRITE->grounded) SPRITE->dragging=1;
        }
      } break;
    default: hero_not_walking(sprite);
  }
  
  /* Jump or gravity.
   */
  if (!(g.input&EGG_BTN_SOUTH)) SPRITE->jump_blackout=0;
  if (SPRITE->jumping) {
    if (!(g.input&EGG_BTN_SOUTH)) {
      hero_end_jump(sprite);
    } else {
      hero_update_jump(sprite,elapsed);
    }
  } else if ((g.input&EGG_BTN_SOUTH)&&((SPRITE->jump_power>0.0)||SPRITE->dragging)&&!SPRITE->jump_blackout) {
    hero_begin_jump(sprite);
  } else {
    SPRITE->gravity+=HERO_GRAVITY_RATE*elapsed;
    if (SPRITE->dragging) {
      if (SPRITE->gravity>HERO_GRAVITY_LIMIT_DRAG) SPRITE->gravity=HERO_GRAVITY_LIMIT_DRAG;
    } else {
      if (SPRITE->gravity>HERO_GRAVITY_LIMIT) SPRITE->gravity=HERO_GRAVITY_LIMIT;
    }
    sprite->y+=SPRITE->gravity*elapsed;
    if (physics_rectify_sprite(sprite,0.0,-1.0)) {
      SPRITE->gravity=0.0;
      if (!SPRITE->grounded) hero_land(sprite);
    } else {
      if (SPRITE->grounded) hero_fall(sprite);
    }
  }
  
  /* Fire arrow.
   */
  if (!SPRITE->dragging&&g.arrows_remaining) {
    if ((g.input&EGG_BTN_WEST)&&!(g.pvinput&EGG_BTN_WEST)) {
      struct sprite *arrow=spawn_sprite(0,&sprite_type_arrow,sprite->x,sprite->y);
      if (arrow) {
        arrow_setup(arrow,(sprite->xform&EGG_XFORM_XREV)?-1.0:1.0);
        g.arrows_remaining--;
      }
    }
  }
  
  //TODO hazards
  
  /* Fallen into a pit?
   */
  if (sprite->y>ROWC+3.0) {
    sprite->defunct=1;
    return;
  }
}

/* Render.
 */
 
static void _hero_render(struct sprite *sprite,int x,int y) {
  uint8_t tileid=sprite->tileid;
  uint8_t xform=sprite->xform;
  if (SPRITE->dragging) {
    tileid+=3;
  } else if (SPRITE->walking) {
    switch (SPRITE->animframe) {
      case 1: tileid+=1; break;
      case 3: tileid+=2; break;
    }
  }
  graf_draw_tile(&g.graf,g.texid_tiles,x,y,tileid,xform);
}

/* Type definition.
 */

const struct sprite_type sprite_type_hero={
  .name="hero",
  .objlen=sizeof(struct sprite_hero),
  .del=_hero_del,
  .init=_hero_init,
  .update=_hero_update,
  .render=_hero_render,
};
