#ifndef GAME_H_
#define GAME_H_

void gameSetup(void);
void systemLoop(void);
void gameFrame(void);
void gotoSleep(void);

//YOUR OTHER FUNCTION PROTOTYPES HERE

void fillTiles(uint8_t location, uint8_t startingTile, uint8_t howMany);

#endif /* GAME_H_ */