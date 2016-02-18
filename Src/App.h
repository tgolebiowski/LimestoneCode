#ifndef APP_H

/* -------------------------------
	STUFF THE OS PROVIDES THE GAME
----------------------------------*/
int16_t ReadShaderSrcFileFromDisk(const char* fileName, GLchar* buffer, uint16_t bufferLen);

/* --------------------------------
	STUFF THE GAME PROVIDES THE OS
 ----------------------------------*/
bool Update();
void Render();
void GameInit();

#define APP_H
#endif