#include "game.h"

/* Load a new map, reset state, begin play.
 */
 
int begin_level(int mapid) {
  kill_all_sprites();
  g.victory=0;
  g.mapid=mapid;

  const void *serial=0;
  int serialc=res_get(&serial,EGG_TID_map,mapid);
  struct rom_map map;
  if (rom_map_decode(&map,serial,serialc)<0) return -1;
  if ((map.w!=COLC)||(map.h!=ROWC)) {
    fprintf(stderr,"map:%d dimensions %dx%d, must be %dx%d\n",mapid,map.w,map.h,COLC,ROWC);
    return -1;
  }
  memcpy(g.map,map.v,COLC*ROWC);
  struct rom_command_reader command_reader={.v=map.cmdv,.c=map.cmdc};
  struct rom_command cmd;
  while (rom_command_reader_next(&cmd,&command_reader)>0) {
    switch (cmd.opcode) {
      case CMD_map_sprite: { // u8:x u8:y u16:spriteid u32:reserved
          int x=cmd.argv[0],y=cmd.argv[1];
          int spriteid=(cmd.argv[2]<<8)|cmd.argv[3];
          if ((x<0)||(x>=COLC)||(y<0)||(y>=ROWC)) return -1;
          spawn_sprite(spriteid,0,x+0.5,y+0.5);
        } break;
    }
  }
  
  //TODO restart music
  
  return 0;
}
