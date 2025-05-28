#include "game/game.h"

#define GOBLIN_PHASE_IDLE 0
#define GOBLIN_PHASE_WALK 1
#define GOBLIN_PHASE_ATTACK 2
#define GOBLIN_PHASE_EAT 3
#define GOBLIN_PHASE_FALL 4
#define GOBLIN_PHASE_HUNGRY 5

#define GOBLIN_WALK_SPEED 2.0
#define GOBLIN_ATTACK_SPEED 7.0 /* Hero's walk speed is 6. */
#define GOBLIN_ATTACK_DISTANCE 1.500
#define GOBLIN_RETAIN_DISTANCE 4.000 /* Once triggered, he'll lose interest when you get so far away. */
#define GOBLIN_OVERSHOOT_DISTANCE 2.000 /* Stop attack if I've gone so far in the wrong direction. */
#define GOBLIN_EAT_DISTANCE 0.100 /* This can be low, should end roughly centered on the hero. But too low and we could miss it. */
#define GOBLIN_GRAVITY_RATE 18.0
#define GOBLIN_GRAVITY_LIMIT 10.0
#define GOBLIN_HUNGRY_TIME 0.200

struct sprite_goblin {
  struct sprite hdr;
  double animclock;
  int animframe;
  uint8_t tileid0;
  int phase;
  double phaseclock;
  double dx; // WALK
  double gravity;
};

#define SPRITE ((struct sprite_goblin*)sprite)

static int _goblin_init(struct sprite *sprite) {
  SPRITE->tileid0=sprite->tileid;
  return 0;
}

/* Phase selection.
 *****************************************************************************/

static void goblin_decide_phase(struct sprite *sprite) {

  // If we're hungry and the hero still looks reachable, attack.
  if ((SPRITE->phase==GOBLIN_PHASE_HUNGRY)&&g.hero) {
    double dy=g.hero->y-sprite->y;
    if ((dy>-0.500)&&(dy<0.500)) {
      double dx=g.hero->x-sprite->x;
      if (
        ((SPRITE->dx<0.0)&&(dx>-GOBLIN_RETAIN_DISTANCE)&&(dx<GOBLIN_ATTACK_DISTANCE))||
        ((SPRITE->dx>0.0)&&(dx>-GOBLIN_ATTACK_DISTANCE)&&(dx<GOBLIN_RETAIN_DISTANCE))
      ) {
        egg_play_sound(RID_sound_attack);
        SPRITE->phase=GOBLIN_PHASE_ATTACK;
        SPRITE->phaseclock=999.0;
        sprite->tileid=SPRITE->tileid0+3;
        return;
      }
    }
    sprite->tileid=SPRITE->tileid0;
  }

  // If we're off the map, or the cell below me is not solid, fall.
  int col=(int)sprite->x;
  int row=(int)sprite->y;
  if ((col<0)||(col>=COLC)||(row<0)||(row>=COLC-1)||(g.physics[g.map[(row+1)*COLC+col]]!=NS_physics_solid)) {
    SPRITE->phase=GOBLIN_PHASE_FALL;
    SPRITE->phaseclock=999.0;
    SPRITE->gravity=0.0;
    sprite->tileid=SPRITE->tileid0;
    return;
  }

  // Determine my horizontal range of travel from the map only.
  double xlo=sprite->x,xhi=sprite->x;
  if ((col>=0)&&(col<COLC)&&(row>=0)&&(row<ROWC-1)) { // Note that we need a row below too.
    const uint8_t *upperp=g.map+row*COLC+col;
    const uint8_t *lowerp=upperp+COLC;
    if ((g.physics[*upperp]==NS_physics_vacant)&&(g.physics[*lowerp]==NS_physics_solid)) {
      xlo=(double)(int)xlo;
      xhi=xlo+1.0;
      int qcol=col;
      while (qcol>0) {
        qcol--;
        upperp--;
        lowerp--;
        if ((g.physics[*upperp]==NS_physics_vacant)&&(g.physics[*lowerp]==NS_physics_solid)) {
          xlo=qcol;
        } else {
          break;
        }
      }
      qcol=col;
      upperp=g.map+row*COLC+col;
      lowerp=upperp+COLC;
      while (qcol<COLC-1) {
        qcol++;
        upperp++;
        lowerp++;
        if ((g.physics[*upperp]==NS_physics_vacant)&&(g.physics[*lowerp]==NS_physics_solid)) {
          xhi=qcol+1.0;
        } else {
          break;
        }
      }
    }
  }
  
  // Shorten range if any goblins or heroes are within it.
  const double YTHRESH=0.750;
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (other->defunct) continue;
    if (sprite==other) continue;
    if (
      (sprite->type!=&sprite_type_goblin)&&
      (sprite->type!=&sprite_type_hero)
    ) continue;
    double dy=other->y-sprite->y;
    if ((dy<=-YTHRESH)||(dy>=YTHRESH)) continue;
    if (other->x<sprite->x) {
      if (other->x+other->phr>xlo) xlo=other->x+other->phr;
    } else {
      if (other->x+other->phl<xhi) xhi=other->x+other->phl;
    }
  }
  
  // If we've poked outside the valid range, force back into it.
  double xrange=xhi-xlo;
  if (xrange>=sprite->phr-sprite->phl) {
    if (sprite->x+sprite->phl<xlo) {
      sprite->x=xlo-sprite->phl;
    } else if (sprite->x+sprite->phr>xhi) {
      sprite->x=xhi-sprite->phr;
    }
  }
  
  // If there's at least two meters of freedom, walk.
  if (xrange>=1.5) {
    SPRITE->phase=GOBLIN_PHASE_WALK;
    SPRITE->phaseclock=7.0;
    double xmid=(xlo+xhi)/2.0;
    if (sprite->x>xmid) {
      SPRITE->dx=-1.0;
      sprite->xform=EGG_XFORM_XREV;
    } else {
      SPRITE->dx=1.0;
      sprite->xform=0;
    }
    return;
  }

  // Final fallback: Sit idle for one second, then look again.
  SPRITE->phase=GOBLIN_PHASE_IDLE;
  SPRITE->phaseclock=1.0;
}

/* Routine update.
 *************************************************************************/
 
static struct sprite *goblin_bumpeth(struct sprite *sprite) {
  double leadx=sprite->x;
  if (SPRITE->dx<0.0) leadx+=sprite->phl;
  else leadx+=sprite->phr;
  struct sprite **otherp=g.spritev;
  int i=g.spritec;
  for (;i-->0;otherp++) {
    struct sprite *other=*otherp;
    if (other->defunct) continue;
    if (sprite==other) continue;
    if (
      (sprite->type!=&sprite_type_goblin)&&
      (sprite->type!=&sprite_type_hero)
    ) continue;
    double dy=other->y-sprite->y;
    if ((dy<-0.500)||(dy>0.500)) continue;
    if (leadx<=other->x+other->phl) continue;
    if (leadx>=other->x+other->phr) continue;
    return other;
  }
  return 0;
}

static int goblin_unbefloored(struct sprite *sprite) {
  int col=(int)sprite->x;
  int row=(int)sprite->y;
  if ((col<0)||(col>=COLC)||(row<0)||(row>=COLC-1)||(g.physics[g.map[(row+1)*COLC+col]]!=NS_physics_solid)) {
    return 1;
  }
  return 0;
}
 
static void goblin_animate_marching(struct sprite *sprite,double elapsed) {
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
}
 
static void goblin_update_IDLE(struct sprite *sprite,double elapsed) {
  sprite->tileid=SPRITE->tileid0;
  SPRITE->animclock=0.0;
}

static void goblin_update_WALK(struct sprite *sprite,double elapsed) {
  goblin_animate_marching(sprite,elapsed);
  double pvx=sprite->x;
  sprite->x+=GOBLIN_WALK_SPEED*SPRITE->dx*elapsed;
  
  // Physics takes care of walking into walls; reverse direction when that happens.
  if (physics_rectify_sprite(sprite,-SPRITE->dx,0.0)) {
    SPRITE->dx=-SPRITE->dx;
    sprite->xform^=EGG_XFORM_XREV;
  } else {
  
    // If our toe stepped over a cliff, turn around. Don't back out the move.
    // Same if the toe goes offscreen, call that a cliff.
    double toex=sprite->x;
    if (SPRITE->dx<0.0) toex+=sprite->phl;
    else toex+=sprite->phr;
    int col=(int)toex;
    int row=(int)(sprite->y+1.0);
    if ((col<0)||(col>=COLC)||(row<0)||(row>=ROWC)||(g.physics[g.map[row*COLC+col]]==NS_physics_vacant)) {
      SPRITE->dx=-SPRITE->dx;
      sprite->xform^=EGG_XFORM_XREV;
      
    // If we bump into another goblin, back out the move and stop walking.
    } else if (goblin_bumpeth(sprite)) {
      sprite->x=pvx;
      SPRITE->phaseclock=0.0;
    }
  }
}

static void goblin_update_ATTACK(struct sprite *sprite,double elapsed) {
  // Hero gone? That can happen. Reset.
  if (!g.hero) {
    SPRITE->phaseclock=0.0;
    return;
  }
  // If the hero escapes vertically, reset. This is likely; a canny hero would jump.
  // It's not the -1/2..1/2 trigger range, it's wider.
  double dy=g.hero->y-sprite->y;
  if ((dy<-1.750)||(dy>1.750)) {
    SPRITE->phaseclock=0.0;
    return;
  }
  // If we're within some horizontal threshold, begin the feast.
  // But! We never start eating if the floor has dropped out. This is critical, it's fairly easy for the hero to lead goblins off a cliff.
  double dx=g.hero->x-sprite->x;
  if ((dx>=-GOBLIN_EAT_DISTANCE)&&(dx<=GOBLIN_EAT_DISTANCE)&&(dy>=-0.5)&&(dy<=0.5)) {
    if (goblin_unbefloored(sprite)) {
      SPRITE->phase=GOBLIN_PHASE_FALL;
      SPRITE->phaseclock=999.0;
      SPRITE->gravity=0.0;
    } else {
      g.hero->defunct=1;
      g.hero=0;
      SPRITE->phase=GOBLIN_PHASE_EAT;
      SPRITE->phaseclock=999.0;
      egg_play_sound(RID_sound_eat);
    }
    return;
  }
  // If we're too far beyond the target horizontally, d'oh, give up.
  if (
    ((SPRITE->dx<0.0)&&(dx>GOBLIN_OVERSHOOT_DISTANCE))||
    ((SPRITE->dx>0.0)&&(dx<-GOBLIN_OVERSHOOT_DISTANCE))
  ) {
    SPRITE->phaseclock=0.0;
    return;
  }
  // Advance at a constant speed. NB (SPRITE->dx) not (dx). He keeps going if you jump over him, the fool.
  sprite->tileid=SPRITE->tileid0+3;
  if (SPRITE->dx>0.0) {
    sprite->xform=0;
    sprite->x+=GOBLIN_ATTACK_SPEED*elapsed;
  } else {
    sprite->xform=EGG_XFORM_XREV;
    sprite->x-=GOBLIN_ATTACK_SPEED*elapsed;
  }
  // If I struck a wall, stop.
  if (physics_rectify_sprite(sprite,-SPRITE->dx,0.0)) {
    SPRITE->phaseclock=0.0;
  }
}

static void goblin_update_EAT(struct sprite *sprite,double elapsed) {
  if ((SPRITE->animclock-=elapsed)<=0.0) {
    SPRITE->animclock+=0.250;
    if (++(SPRITE->animframe)>=2) SPRITE->animframe=0;
    sprite->tileid=SPRITE->tileid0+4+SPRITE->animframe;
  }
}

static void goblin_update_FALL(struct sprite *sprite,double elapsed) {

  // Force my horizontal position to align with the map.
  sprite->x=(double)((int)sprite->x+0.5);

  SPRITE->gravity+=GOBLIN_GRAVITY_RATE*elapsed;
  if (SPRITE->gravity>GOBLIN_GRAVITY_LIMIT) SPRITE->gravity=GOBLIN_GRAVITY_LIMIT;
  sprite->y+=SPRITE->gravity*elapsed;
  if (sprite->y>ROWC+1.0) {
    sprite->defunct=1;
    return;
  }
  if (physics_rectify_sprite(sprite,0.0,-1.0)) {
    // Don't just reset the phase. Go IDLE explicitly for a brief interval.
    SPRITE->phase=GOBLIN_PHASE_IDLE;
    SPRITE->phaseclock=1.0;
  }
}

static void goblin_update_HUNGRY(struct sprite *sprite,double elapsed) {
  if (g.hero) {
    double dy=g.hero->y-sprite->y;
    if ((dy>-0.500)&&(dy<0.500)) {
      double dx=g.hero->x-sprite->x;
      if (
        ((SPRITE->dx<0.0)&&(dx>-GOBLIN_RETAIN_DISTANCE)&&(dx<=GOBLIN_ATTACK_DISTANCE))||
        ((SPRITE->dx>0.0)&&(dx>=-GOBLIN_ATTACK_DISTANCE)&&(dx<GOBLIN_RETAIN_DISTANCE))
      ) {
        // Still in range, cool.
      } else {
        SPRITE->phaseclock=0.0;
      }
    }
  }
}

static void _goblin_update(struct sprite *sprite,double elapsed) {

  // IDLE and WALK phases can be interrupted to attack the hero.
  if (g.hero&&((SPRITE->phase==GOBLIN_PHASE_IDLE)||(SPRITE->phase==GOBLIN_PHASE_WALK))) {
    double dy=g.hero->y-sprite->y;
    if ((dy>-0.500)&&(dy<0.500)) {
      double dx=g.hero->x-sprite->x;
      if ((dx>-GOBLIN_ATTACK_DISTANCE)&&(dx<GOBLIN_ATTACK_DISTANCE)) {
        egg_play_sound(RID_sound_hungry);
        SPRITE->phase=GOBLIN_PHASE_HUNGRY;
        SPRITE->phaseclock=GOBLIN_HUNGRY_TIME;
        sprite->tileid=SPRITE->tileid0+3;
        if (dx>0.0) {
          sprite->xform=0;
          SPRITE->dx=1.0;
        } else {
          sprite->xform=EGG_XFORM_XREV;
          SPRITE->dx=-1.0;
        }
      }
    }
  }

  // Change phase when the timer runs out. This will happen on our first update.
  if ((SPRITE->phaseclock-=elapsed)<=0.0) {
    goblin_decide_phase(sprite);
  }
  
  switch (SPRITE->phase) {
    case GOBLIN_PHASE_IDLE: goblin_update_IDLE(sprite,elapsed); break;
    case GOBLIN_PHASE_WALK: goblin_update_WALK(sprite,elapsed); break;
    case GOBLIN_PHASE_ATTACK: goblin_update_ATTACK(sprite,elapsed); break;
    case GOBLIN_PHASE_EAT: goblin_update_EAT(sprite,elapsed); break;
    case GOBLIN_PHASE_FALL: goblin_update_FALL(sprite,elapsed); break;
    case GOBLIN_PHASE_HUNGRY: goblin_update_HUNGRY(sprite,elapsed); break;
  }
}

static void _goblin_render(struct sprite *sprite,int x,int y) {
  if (SPRITE->phase==GOBLIN_PHASE_HUNGRY) {
    // HUNGRY takes two tiles. Also we don't bother setting (tileid) in this case.
    graf_draw_tile(&g.graf,g.texid_tiles,x,y,0x56,sprite->xform);
    graf_draw_tile(&g.graf,g.texid_tiles,x,y-NS_sys_tilesize,0x46,sprite->xform);
  } else {
    graf_draw_tile(&g.graf,g.texid_tiles,x,y,sprite->tileid,sprite->xform);
  }
}

const struct sprite_type sprite_type_goblin={
  .name="goblin",
  .objlen=sizeof(struct sprite_goblin),
  .init=_goblin_init,
  .update=_goblin_update,
  .render=_goblin_render,
};
