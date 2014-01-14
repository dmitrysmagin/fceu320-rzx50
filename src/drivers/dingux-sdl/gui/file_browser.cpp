#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#include "file_list.h"

// Externals
extern SDL_Surface* screen;
extern Config *g_config;

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

	spy = 74;

	size = list->Size();

	index = 0;
	offset_start = 0;
	offset_end = size > max_entries ? max_entries : size;

	g_dirty = 1;
	while (1) {
		// Parse input
		readkey();
		// TODO - put exit keys

		// Go to previous folder or return ...
		if (parsekey(DINGOO_B) || parsekey(DINGOO_LEFT)) {
			list->Enter(-1);
			goto RESTART;
		}

		// Enter folder or select rom ...
		if (parsekey(DINGOO_A) || parsekey(DINGOO_RIGHT)) {
			if (list->GetSize(index) == -1) {
				list->Enter(index);
				goto RESTART;
			} else {
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
					} else {
						index = size - 1;
						offset_end = size;
						offset_start = size <= max_entries ? 0 : offset_end - max_entries;
						spy = 74 + 15*(index - offset_start);
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
				} else {
					index = 0;
					offset_start = 0;
					offset_end = size <= max_entries ? size : max_entries;
					spy = 74;
				}
			}

			if (parsekey(DINGOO_L, 1)) {
				if (index > offset_start) {
					index = offset_start;

					spy = 74;

				} else if (index - max_entries >= 0){
						index -= max_entries;
						offset_start -= max_entries;
						offset_end = offset_start + max_entries;
				} else
					goto RESTART;
			}

			if (parsekey(DINGOO_R, 1)) {
				if (index < offset_end-1) {
					index = offset_end-1;

					spy = 74 + 15*(index-offset_start);

				} else if (offset_end + max_entries <= size) {
						index += max_entries;
						offset_end += max_entries;
						offset_start += max_entries;
				} else {
					index = size - 1;
					spy = 74 + 15*(max_entries-1);
					offset_end = size;
					offset_start = offset_end - max_entries;
				}
			}
		}

		// Draw stuff
		if (g_dirty) {
			draw_bg(g_bg);

			DrawChar(gui_screen, SP_BROWSER, 40, 38);

			// Draw file list
			for (i = offset_start, y = 70; i < offset_end; i++, y += 15) {
				DrawText(gui_screen, list->GetName(i), 36, y);
			}

			// Draw info
			if (info)
				DrawText(gui_screen, info, 16, 225);
			else {
				if (justsavedromdir == 1){
					DrawText(gui_screen, "ROM Dir Saved!", 16, 225);
				} else {
					if (list->GetSize(index) == -1)
						DrawText(gui_screen, "Open folder?", 16, 225);
					else
						DrawText(gui_screen, "Open file?", 16, 225);
				}
				justsavedromdir = 0;
			}

			// Draw selector
			DrawChar(gui_screen, SP_SELECTOR, 20, spy);

			// Draw offset marks
			if (offset_start > 0)
				DrawChar(gui_screen, SP_UPARROW, 274, 62);
			if (offset_end < list->Size())
				DrawChar(gui_screen, SP_DOWNARROW, 274, 203);

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
