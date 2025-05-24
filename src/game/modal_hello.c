#include "game.h"

struct hello {
  double cooldown_clock;
};

/* Delete.
 */
 
void hello_del(struct hello *hello) {
  if (!hello) return;
  free(hello);
}

/* New.
 */
 
struct hello *hello_new() {
  struct hello *hello=calloc(1,sizeof(struct hello));
  if (!hello) return 0;
  
  hello->cooldown_clock=1.0;
  
  return hello;
}

/* Update.
 */
 
void hello_update(struct hello *hello,double elapsed) {
  if (hello->cooldown_clock>0.0) {
    hello->cooldown_clock-=elapsed;
  } else {
    if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
      begin_play(); // deletes me
      return;
    }
  }
}

/* Render.
 */
 
void hello_render(struct hello *hello) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x100802ff);
  
  { // Big logo.
    int srcx=0,srcy=92,w=91,h=36;
    int dstx=(FBW>>1)-(w>>1);
    int dsty=20;
    graf_draw_decal(&g.graf,g.texid_tiles,dstx,dsty,srcx,srcy,w,h,0);
  }
}
