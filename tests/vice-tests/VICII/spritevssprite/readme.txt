
This test examines the case when two (identical) sprites collide at the exact
same vertical and horizontal position.

Per Frame,
- the (both) sprites are moved one pixel horizontally
- the sprite/sprite collision register is read twice per rasterline, for the
  height of the sprite. the second value read is the one we investigate here.
