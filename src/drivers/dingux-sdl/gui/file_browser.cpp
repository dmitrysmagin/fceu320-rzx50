#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#include "file_list.h"

// Externals
extern SDL_Surface* screen;
extern Config *g_config;

s32 LoadLastSelectedRomPos() // Try to get the last selected rom position from a config file
{
	char* home = getenv("HOME");
	char lastselfile [128];
	s32 savedval = 0;
	sprintf (lastselfile, "%s/.fceux/lastselected.cfg", home);
	FILE * pFile;
	pFile = fopen (lastselfile,"r+");
	if (pFile != NULL) {
		fscanf (pFile, "%i", &savedval);
		fclose (pFile);
	}
	return savedval;
}

void SaveLastSelectedRomPos(s32 pospointer) // Save the last selected rom position in a config file
{
	char* home = getenv("HOME");
	char lastselfile [128];
	sprintf (lastselfile, "%s/.fceux/lastselected.cfg", home);
	FILE * pFile;
	pFile = fopen (lastselfile,"w+");
	fprintf (pFile, "%i", pospointer);
	fclose (pFile);
}

void DelLastSelectedRomPos() // Remove the last selected rom position config file
{
	char* home = getenv("HOME");
	char lastselfile [128];
	sprintf (lastselfile, "%s/.fceux/lastselected.cfg", home);
	remove (lastselfile);
}

static char s_LastDir[128] = "/";
int RunFileBrowser(char *source, char *outname, const char *types[],
		const char *info) {

	int size = 0;
	int index;
	int offset_start, offset_end;
	static int max_entries = 8;
	int justsavedromdir = 0;

	static int spy;
	int y, i;
	
	// Try to get a saved romdir from a config file
	char* home = getenv("HOME");
	char romcfgfile [128];
	sprintf (romcfgfile, "%s/.fceux/romdir.cfg", home);
	FILE * pFile;
	pFile = fopen (romcfgfile,"r+");
	if (pFile != NULL) {
		fgets (s_LastDir , 128 , pFile);
		fclose (pFile);
	}

	// Create file list
	FileList *list = new FileList(source ? source : s_LastDir, types);
	if (list == NULL)
		return 0;

	RESTART:

	spy = 72;

	size = list->Size();

	index = 0;
	index = LoadLastSelectedRomPos();
	offset_start = 0;
	offset_end = size > max_entries ? max_entries : size;
	if (index >= (size - 1)) { //go to end of list
		index = size - 1;
		offset_end = size;
		offset_start = size <= max_entries ? 0 : offset_end - max_entries;
		spy = 72 + 15*(index - offset_start);
	} else if (index > (max_entries - 2)) {
		offset_start = index - (max_entries - 1);
		offset_end = offset_start + max_entries;
		spy = 72 + 15*(max_entries - 1);
	} else {
		spy = 72 + (15*index);
	}

	g_dirty = 1;
	while (1) {
		// Parse input
		readkey();
		// TODO - put exit keys

		// Go to previous folder or return ...
		if (parsekey(DINGOO_B)) {
			DelLastSelectedRomPos();
			list->Enter(-1);
			goto RESTART;
		}

		// Enter folder or select rom ...
		if (parsekey(DINGOO_A)) {
			if (list->GetSize(index) == -1) { //enter folder
				DelLastSelectedRomPos();
				list->Enter(index);
				goto RESTART;
			} else { //select rom
				SaveLastSelectedRomPos(index);
				strncpy(outname, list->GetPath(index), 128);
				break;
			}
		}

		if (parsekey(DINGOO_X)) {
			return 0;
		}
		
		if (parsekey(DINGOO_SELECT)) {
			// Save the current romdir in a config file
			char* home = getenv("HOME");
			char romcfgfile [128];
			strncpy(s_LastDir, list->GetCurDir(), 128);
			sprintf (romcfgfile, "%s/.fceux/romdir.cfg", home);
			FILE * pFile;
			pFile = fopen (romcfgfile,"w+");
			fputs (s_LastDir,pFile);
			fclose (pFile);
			justsavedromdir = 1;
			DelLastSelectedRomPos();
		}

		if (size > 0) {
			// Move through file list
			if (parsekey(DINGOO_UP, 1)) {
					if (index > offset_start){
						index--;
						spy -= 15;
					} else if (offset_start > 0) {
						index--;
						offset_start--;
						offset_end--;
					} else {                          //go to end of list
						index = size - 1;
						offset_end = size;
						offset_start = size <= max_entries ? 0 : offset_end - max_entries;
						spy = 72 + 15*(index - offset_start);
					}
			}

			if (parsekey(DINGOO_DOWN, 1)) {
				if (index < offset_end - 1){
					index++;
					spy += 15;
				} else if (offset_end < size) {
					index++;
					offset_start++;
					offset_end++;
				} else {                    //go to beginning of list
					index = 0;
					offset_start = 0;
					offset_end = size <= max_entries ? size : max_entries;
					spy = 72;
				}
			}

			if (parsekey(DINGOO_LEFT, 1)) {
				if (index > offset_start) {
					index = offset_start;

					spy = 72;

				} else if (index - max_entries >= 0){
						index -= max_entries;
						offset_start -= max_entries;
						offset_end = offset_start + max_entries;
				} else if (index <= 0){                    //go to end of list
						index = size - 1;
						offset_end = size;
						offset_start = size <= max_entries ? 0 : offset_end - max_entries;
						spy = 72 + 15*(index - offset_start);
				} else {                                  //go to beginning of list
					index = 0;
					offset_start = 0;
					offset_end = size <= max_entries ? size : max_entries;
					spy = 72;
				}
			}

			if (parsekey(DINGOO_RIGHT, 1)) {
				if (index < offset_end-1) {
					index = offset_end-1;

					spy = 72 + 15*(index-offset_start);

				} else if (offset_end + max_entries <= size) {
						index += max_entries;
						offset_end += max_entries;
						offset_start += max_entries;
				} else if (index >= size -1) {         //go to beginning of list
						index = 0;
						offset_start = 0;
						offset_end = size <= max_entries ? size : max_entries;
						spy = 72;
				} else {                               //go to end of list
					index = size - 1;
					spy = 72 + 15*(max_entries-1);
					offset_end = size;
					offset_start = offset_end - max_entries;
				}
			}
		}

		// Draw stuff
		if (g_dirty) {
			draw_bg(g_bg);
			
			//Draw Top and Bottom Bars
			DrawChar(gui_screen, SP_SELECTOR, 0, 37);
			DrawChar(gui_screen, SP_SELECTOR, 81, 37);
			DrawChar(gui_screen, SP_SELECTOR, 0, 225);
			DrawChar(gui_screen, SP_SELECTOR, 81, 225);
			DrawText(gui_screen, "B - Go Back", 235, 225);
			DrawChar(gui_screen, SP_LOGO, 12, 9);
			
			// Draw selector
			DrawChar(gui_screen, SP_SELECTOR, 4, spy);
			DrawChar(gui_screen, SP_SELECTOR, 81, spy);

			DrawText(gui_screen, "ROM Browser", 8, 37);

			// Draw file list
			for (i = offset_start, y = 72; i < offset_end; i++, y += 15) {
				DrawText(gui_screen, list->GetName(i), 8, y);
			}

			// Draw info
			if (info)
				DrawText(gui_screen, info, 8, 225);
			else {
				if (justsavedromdir == 1){
					DrawText(gui_screen, "ROM dir successfully saved!", 8, 225);
				} else {
					if (list->GetSize(index) == -1)
						DrawText(gui_screen, "SELECT - Save ROM dir", 8, 225);
					else
						DrawText(gui_screen, "SELECT - Save ROM dir", 8, 225);
				}
				justsavedromdir = 0;
			}

			// Draw offset marks
			if (offset_start > 0)
				DrawChar(gui_screen, SP_UPARROW, 157, 57);
			if (offset_end < list->Size())
				DrawChar(gui_screen, SP_DOWNARROW, 157, 197);

			g_dirty = 0;
		}

		SDL_Delay(16);

		// Update real screen
		FCEUGUI_Flip();
		
	}

	if (source == NULL)
		strncpy(s_LastDir, list->GetCurDir(), 128);
	delete list;

	return 1;
}
