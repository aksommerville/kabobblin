#include "game.h"

/* Rectify sprite position.
 * For now:
 *  - Only sprite-on-map collisions count.
 *  - No one-way collisions.
 * Return nonzero if modified.
 */
 
int physics_rectify_sprite(struct sprite *sprite,double corrx,double corry) {
  double fl=sprite->x+sprite->phl;
  double fr=sprite->x+sprite->phr;
  double ft=sprite->y+sprite->pht;
  double fb=sprite->y+sprite->phb;
  int cola=(int)fl; if (cola<0) cola=0;
  int colz=(int)(fr-0.001); if (colz>=COLC) colz=COLC-1;
  if (cola>colz) return 0;
  int rowa=(int)ft; if (rowa<0) rowa=0;
  int rowz=(int)(fb-0.001); if (rowz>=ROWC) rowz=ROWC-1;
  if (rowa>rowz) return 0;
  // Correct against the first collision we find. If there's more than one, we should catch the next one next time around.
  const uint8_t *maprow=g.map+rowa*COLC+cola;
  int row=rowa; for (;row<=rowz;row++,maprow+=COLC) {
    const uint8_t *mapp=maprow;
    int col=cola; for (;col<=colz;col++,mapp++) {
      uint8_t physics=g.physics[*mapp];
      if (physics==NS_physics_solid) {
        if (corrx>0.0) {
          sprite->x=col+1.001-sprite->phl;
        } else if (corrx<0.0) {
          sprite->x=col-0.001-sprite->phr;
        } else if (corry>0.0) {
          sprite->y=row+1.001-sprite->pht;
        } else if (corry<0.0) {
          sprite->y=row-0.001-sprite->phb;
        }
        return 1;
      }
    }
  }
  return 0;
}
