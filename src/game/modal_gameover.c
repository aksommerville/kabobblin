#include "game.h"
#include <stdarg.h>

/* We have rows 3..5 of ASCII in rows 8..10 of the tilesheet, with some punctuation shuffled around.
 * The tiles are naturally 8x8 but all the graphics use 4x5 of that.
 * Add a pixel of spacing, so 5x6 per cell.
 * Maximum grid size 32,14.
 */
#define CELLW 5
#define CELLH 6
#define CELL_COLC 26
#define CELL_ROWC 8
#define TEXTX 16 /* Center of left column in output. */
#define TEXTY 10 /* '' top row. */

/* Each phase (for the most part) is a line of console text that we animate appearing.
 */
#define GAMEOVER_PHASE_COOLDOWN 0
#define GAMEOVER_PHASE_PARTICIPATION 1
#define GAMEOVER_PHASE_TIME 2
#define GAMEOVER_PHASE_TREASURE 3
#define GAMEOVER_PHASE_DEATH 4
#define GAMEOVER_PHASE_DONE 5

struct gameover {
  int phase;
  double clock; // Counts up, and resets at each phase transition.
  uint8_t text[CELL_COLC*CELL_ROWC]; // 0x80..0xaf or 0
  
  // Nonzero if we've rendered the final values for a given line:
  int done_time;
  int done_treasure;
  int done_death;
  int done_total;
  int done_participation;
  
  // Score, calculated at init. The raw inputs, you can read off (g).
  int score_time;
  int score_treasure;
  int score_death; // <=0
  int score_total;
};

/* Delete.
 */
 
void gameover_del(struct gameover *gameover) {
  if (!gameover) return;
  free(gameover);
}

/* Replace one row of text.
 * "%N" to insert one variadic int, using exactly (0..9) columns, right-aligned. "%0" variable-width.
 * "%t" to insert a variadic double as time, using exactly 9 columns "MM:SS.mmm".
 */
 
static void gameover_printf(struct gameover *gameover,int row,const char *fmt,...) {
  if ((row<0)||(row>=CELL_ROWC)) return;
  if (!fmt) fmt="";
  va_list vargs;
  va_start(vargs,fmt);
  uint8_t *dst=gameover->text+row*CELL_COLC;
  memset(dst,0,CELL_COLC);
  int x=0;
  while ((x<CELL_COLC)&&*fmt) {
    uint8_t ch=*(fmt++);
    if (ch=='%') {
    
      if (*fmt=='t') {
        fmt++;
        double sf=va_arg(vargs,double);
        if (x<=CELL_COLC-9) {
          if (sf<0.0) sf=0.0;
          int ms=(int)(sf*1000.0);
          int s=ms/1000; ms%=1000;
          int m=s/60; s%=60;
          if (m>99) { m=s=99; ms=999; }
          dst[x++]=0x80+(m/10);
          dst[x++]=0x80+(m%10);
          dst[x++]=0x8a; // colon
          dst[x++]=0x80+(s/10);
          dst[x++]=0x80+(s%10);
          dst[x++]=0x8b; // dot
          dst[x++]=0x80+(ms/100);
          dst[x++]=0x80+(ms/10)%10;
          dst[x++]=0x80+ms%10;
        }
        
      } else if ((*fmt>='0')&&(*fmt<='9')) {
        int v=va_arg(vargs,int);
        int digitc=(*fmt)-'0';
        int positive=1;
        if (v<0) {
          positive=0;
          v=-v;
        }
        fmt++;
        if (!digitc) {
               if (v>=1000000000) digitc=10;
          else if (v>= 100000000) digitc=9;
          else if (v>=  10000000) digitc=8;
          else if (v>=   1000000) digitc=7;
          else if (v>=    100000) digitc=6;
          else if (v>=     10000) digitc=5;
          else if (v>=      1000) digitc=4;
          else if (v>=       100) digitc=3;
          else if (v>=        10) digitc=2;
          else digitc=1;
          if (!positive) digitc++;
        }
        if (x<=CELL_COLC-digitc) {
          int i=digitc;
          for (;i-->0;v/=10) dst[x+i]=0x80+v%10;
          // Eliminate leading zeroes.
          for (i=0;i<digitc-1;i++) {
            if (dst[x+i]==0x80) dst[x+i]=0;
            else if (!positive&&(i>0)) {
              dst[x+i-1]=0x8c;
              break;
            } else break;
          }
        }
        x+=digitc;
        
      } else {
        // Unknown format char, emit a literal "%".
        // Except we don't actually have a glyph for that. So skip it.
        x++;
      }
    } else if ((ch>='A')&&(ch<='Z')) {
      dst[x++]=ch+0x50;
    } else if ((ch>='0')&&(ch<=':')) {
      dst[x++]=ch+0x50;
    } else if ((ch>='a')&&(ch<='z')) {
      dst[x++]=ch+0x30;
    } else if (ch=='.') {
      dst[x++]=0x8b;
    } else if (ch=='-') {
      dst[x++]=0x8c;
    } else {
      // Unknown glyph, eg space, leave it blank.
      x++;
    }
  }
}

/* New.
 */
 
struct gameover *gameover_new() {
  struct gameover *gameover=calloc(1,sizeof(struct gameover));
  if (!gameover) return 0;
  
  gameover->phase=GAMEOVER_PHASE_COOLDOWN;
  gameover->clock=0.0;
  
  /* Calculate score.
   */
  if (g.playtime<=TIME_MINIMUM) gameover->score_time=TIME_BONUS_MAX;
  else if (g.playtime>=TIME_MAXIMUM) gameover->score_time=0;
  else gameover->score_time=((TIME_MAXIMUM-g.playtime)*TIME_BONUS_MAX)/(TIME_MAXIMUM-TIME_MINIMUM);
  gameover->score_treasure=g.treasurec*TREASURE_BONUS;
  gameover->score_death=g.deathc*DEATH_BONUS;
  gameover->score_total=PARTICIPATION_AWARD+gameover->score_time+gameover->score_treasure+gameover->score_death;
  if (gameover->score_total<0) gameover->score_total=0; // Can be negative due to death; clamp it.
  
  /* Since we're in charge of calculating the score, we also manage (g.hiscore).
   */
  if (gameover->score_total>g.hiscore) {
    g.hiscore=gameover->score_total;
    hiscore_save();
  }
  
  return gameover;
}

/* Print bits in the text console.
 */
 
static void gameover_partial_participation(struct gameover *gameover,double t) {
  int score=(int)(PARTICIPATION_AWARD*t);
  gameover_printf(gameover,0,"Complete: %9 %6",PARTICIPATION_AWARD,score);
}

static void gameover_final_participation(struct gameover *gameover) {
  if (gameover->done_participation) return;
  gameover->done_participation=1;
  gameover_printf(gameover,0,"Complete: %9 %6",PARTICIPATION_AWARD,PARTICIPATION_AWARD);
}
 
static void gameover_partial_time(struct gameover *gameover,double t) {
  int score=(int)(gameover->score_time*t);
  gameover_printf(gameover,1,"    Time: %t %6",g.playtime,score);
}

static void gameover_final_time(struct gameover *gameover) {
  if (gameover->done_time) return;
  gameover->done_time=1;
  gameover_printf(gameover,1,"    Time: %t %6",g.playtime,gameover->score_time);
}
 
static void gameover_partial_treasure(struct gameover *gameover,double t) {
  int score=(int)(gameover->score_treasure*t);
  gameover_printf(gameover,2,"Treasure: %9 %6",g.treasurec,score);
}

static void gameover_final_treasure(struct gameover *gameover) {
  if (gameover->done_treasure) return;
  gameover->done_treasure=1;
  gameover_printf(gameover,2,"Treasure: %9 %6",g.treasurec,gameover->score_treasure);
}
 
static void gameover_partial_death(struct gameover *gameover,double t) {
  int score=(int)(gameover->score_death*t);
  gameover_printf(gameover,3,"   Death: %9 %6",g.deathc,score);
}

static void gameover_final_death(struct gameover *gameover) {
  if (gameover->done_death) return;
  gameover->done_death=1;
  gameover_printf(gameover,3,"   Death: %9 %6",g.deathc,gameover->score_death);
}

// There's no "partial_total"; it appears finished as soon as it's ready.
static void gameover_final_total(struct gameover *gameover) {
  if (gameover->done_total) return;
  gameover->done_total=1;
  gameover_printf(gameover,5,"   Score:        %9",gameover->score_total);
  gameover_printf(gameover,6,"    Best:        %9",g.hiscore);
}

/* Update.
 */
 
void gameover_update(struct gameover *gameover,double elapsed) {
  gameover->clock+=elapsed;
  switch (gameover->phase) {
  
    case GAMEOVER_PHASE_COOLDOWN: {
        // Input suppressed.
        if (gameover->clock>=0.5) {
          gameover->clock-=0.5;
          gameover->phase=GAMEOVER_PHASE_PARTICIPATION;
        }
      } break;
      
    case GAMEOVER_PHASE_PARTICIPATION: {
        if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
          gameover->phase=GAMEOVER_PHASE_DONE;
        } else if (gameover->clock>=1.0) {
          gameover->clock-=1.0;
          gameover->phase=GAMEOVER_PHASE_TIME;
        } else {
          gameover_partial_participation(gameover,gameover->clock/1.0);
        }
      } break;
      
    case GAMEOVER_PHASE_TIME: {
        if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
          gameover->phase=GAMEOVER_PHASE_DONE;
        } else if (gameover->clock>=1.0) {
          gameover->clock-=1.0;
          gameover->phase=GAMEOVER_PHASE_TREASURE;
        } else {
          gameover_final_participation(gameover);
          gameover_partial_time(gameover,gameover->clock/1.0);
        }
      } break;
      
    case GAMEOVER_PHASE_TREASURE: {
        if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
          gameover->phase=GAMEOVER_PHASE_DONE;
        } else if (gameover->clock>=1.0) {
          gameover->clock-=1.0;
          gameover->phase=GAMEOVER_PHASE_DEATH;
        } else {
          gameover_final_participation(gameover);
          gameover_final_time(gameover);
          gameover_partial_treasure(gameover,gameover->clock/1.0);
        }
      } break;
      
    case GAMEOVER_PHASE_DEATH: {
        if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
          gameover->phase=GAMEOVER_PHASE_DONE;
        } else if (gameover->clock>=1.0) {
          gameover->clock-=1.0;
          gameover->phase=GAMEOVER_PHASE_DONE;
        } else {
          gameover_final_participation(gameover);
          gameover_final_time(gameover);
          gameover_final_treasure(gameover);
          gameover_partial_death(gameover,gameover->clock/1.0);
        }
      } break;
      
    case GAMEOVER_PHASE_DONE: {
        if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
          begin_hello(); // deletes me
          return;
        } else {
          gameover_final_participation(gameover);
          gameover_final_time(gameover);
          gameover_final_treasure(gameover);
          gameover_final_death(gameover);
          gameover_final_total(gameover);
        }
      } break;
  }
}

/* Render.
 */
 
void gameover_render(struct gameover *gameover) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x100802ff);
  
  // Floor along the bottom.
  int dstx=NS_sys_tilesize>>1;
  int dsty=FBH-(NS_sys_tilesize>>1);
  for (;dstx<FBW;dstx+=NS_sys_tilesize) graf_draw_tile(&g.graf,g.texid_tiles,dstx,dsty,0x11,0);
  
  //TODO Animated sequence of the hero rescuing the princess.
  
  // Text console.
  const uint8_t *src=gameover->text;
  int rowi=CELL_ROWC;
  for (dsty=TEXTY;rowi-->0;dsty+=CELLH) {
    int coli=CELL_COLC;
    for (dstx=TEXTX;coli-->0;dstx+=CELLW,src++) {
      if (!*src) continue;
      graf_draw_tile(&g.graf,g.texid_tiles,dstx,dsty,*src,0);
    }
  }
}
