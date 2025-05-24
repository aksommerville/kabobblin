#include "game.h"

#define HELLO_MSG_TIME          6.000
#define HELLO_MSG_FADE_IN_TIME  0.500
#define HELLO_MSG_FADE_OUT_TIME 0.500

struct hello {
  int hiscoreid,hiscorex,hiscorey,hiscorew,hiscoreh;
  int msgid,msgx,msgy,msgw,msgh;
  int msgp; // index in strings:1, 4..6
  double msgclock;
};

/* Delete.
 */
 
void hello_del(struct hello *hello) {
  if (!hello) return;
  egg_texture_del(hello->hiscoreid);
  egg_texture_del(hello->msgid);
  free(hello);
}

/* Load the next message.
 */
 
static void hello_next_message(struct hello *hello) {
  hello->msgp++;
  if ((hello->msgp<4)||(hello->msgp>6)) hello->msgp=4;
  egg_texture_del(hello->msgid);
  hello->msgid=font_texres_oneline(g.font,1,hello->msgp,FBW,0x804020ff);
  egg_texture_get_status(&hello->msgw,&hello->msgh,hello->msgid);
  hello->msgx=(FBW>>1)-(hello->msgw>>1);
  hello->msgy=FBH-5-hello->msgh;
  hello->msgclock=0.0;
}

/* New.
 */
 
struct hello *hello_new() {
  struct hello *hello=calloc(1,sizeof(struct hello));
  if (!hello) return 0;
  
  { // High score won't change while we're open, capture it just once.
    char tmp[256];
    struct strings_insertion ins={'i',.i=g.hiscore};
    int tmpc=strings_format(tmp,sizeof(tmp),1,3,&ins,1);
    hello->hiscoreid=font_tex_oneline(g.font,tmp,tmpc,FBW,0xffffffff);
    egg_texture_get_status(&hello->hiscorew,&hello->hiscoreh,hello->hiscoreid);
    hello->hiscorex=(FBW>>1)-(hello->hiscorew>>1);
    hello->hiscorey=58;
  }
  
  hello_next_message(hello);
  
  return hello;
}

/* Update.
 */
 
void hello_update(struct hello *hello,double elapsed) {
  if ((g.input&EGG_BTN_SOUTH)&&!(g.pvinput&EGG_BTN_SOUTH)) {
    begin_play(); // deletes me
    return;
  }
  hello->msgclock+=elapsed;
  if (hello->msgclock>=HELLO_MSG_TIME) hello_next_message(hello);
}

/* Render.
 */
 
void hello_render(struct hello *hello) {
  graf_draw_rect(&g.graf,0,0,FBW,FBH,0x100802ff);
  
  { // Big logo.
    int srcx=0,srcy=92,w=91,h=36;
    int dstx=(FBW>>1)-(w>>1);
    int dsty=14;
    graf_draw_decal(&g.graf,g.texid_tiles,dstx,dsty,srcx,srcy,w,h,0);
  }
  
  // Two smaller text labels. (msg) fades in and out according to (msgclock).
  graf_draw_decal(&g.graf,hello->hiscoreid,hello->hiscorex,hello->hiscorey,0,0,hello->hiscorew,hello->hiscoreh,0);
  int msgalpha=0xff;
  if (hello->msgclock<HELLO_MSG_FADE_IN_TIME) msgalpha=(int)((hello->msgclock*255.0)/HELLO_MSG_FADE_IN_TIME);
  else if (hello->msgclock>HELLO_MSG_TIME-HELLO_MSG_FADE_OUT_TIME) msgalpha=(int)(((HELLO_MSG_TIME-hello->msgclock)*255.0)/HELLO_MSG_FADE_OUT_TIME);
  if (msgalpha>0) {
    if (msgalpha<0xff) graf_set_alpha(&g.graf,msgalpha);
    graf_draw_decal(&g.graf,hello->msgid,hello->msgx,hello->msgy,0,0,hello->msgw,hello->msgh,0);
    if (msgalpha<0xff) graf_set_alpha(&g.graf,0xff);
  }
}
