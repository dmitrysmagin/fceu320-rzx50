#ifndef __FCEU_DINGOO_H
#define __FCEU_DINGOO_H

#include <SDL/SDL.h>
#include "main.h"
#include "dface.h"
#include "input.h"

static void DoFun(int frameskip);
static int isloaded = 0;

int LoadGame(const char *path);
int CloseGame(void);

int FCEUD_LoadMovie(const char *name, char *romname);
int FCEUD_DriverReset();

#endif
