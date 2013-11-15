#include "../config.h"

typedef struct _setting_entry {
	const char *name;
	const char *info;
	const std::string option;
	void (*update)(unsigned long);
} SettingEntry;

#include "main_settings.cpp"
#include "video_settings.cpp"
#include "sound_settings.cpp"

static int cmd_main_settings() {
	return RunMainSettings();
}

static int cmd_video_settings() {
	return RunVideoSettings();
}

static int cmd_sound_settings() {
	return RunSoundSettings();
}

static int cmd_config_save() {
	extern Config *g_config;
	g_config->save();
}

static MenuEntry
	settings_menu[] = {
		{ "Main Setup", "Change fceux main config", cmd_main_settings },
		{ "Video Setup", "Change video config", cmd_video_settings },
		{ "Sound Setup", "Change sound config", cmd_sound_settings },
		{ "Save config as default",	"Override default config", cmd_config_save } };

int RunSettingsMenu() {
	static int index = 0;
	static int spy = 80;
	int done = 0, y, i;

	g_dirty = 1;
	while (!done) {
		// Get time and battery every second
		if (update_time()) {
			update_battery();
			g_dirty = 1;
		}

		// Parse input
		readkey();
		if (parsekey(DINGOO_B))
			done = 1;

		if (parsekey(DINGOO_UP, 1)) {
			if (index > 0) {
				index--;
				spy -= 16;
			} else {
				index = 3;
				spy = 80 + 16*index;
			}
		}

		if (parsekey(DINGOO_DOWN, 1)) {
			if (index < 3) {
				index++;
				spy += 16;
			} else {
				index = 0;
				spy = 80;
			}
		}

		if (parsekey(DINGOO_A)) {
			done = settings_menu[index].command();
		}

		// Must draw bg only when needed
		// Draw stuff
		if (g_dirty) {
			draw_bg(vbuffer, g_bg);

			// Draw time and battery every minute
			DrawText(gui_screen, g_time, 148, 5);
			DrawText(gui_screen, g_battery, 214, 5);

			DrawChar(gui_screen, SP_SETTINGS, 40, 38);

			// Draw menu
			for (i = 0, y = 76; i < 4; i++, y += 16) {
				DrawText(gui_screen, settings_menu[i].name, 50, y);
			}

			// Draw info
			DrawText(gui_screen, settings_menu[index].info, 16, 225);

			// Draw selector
			DrawChar(gui_screen, SP_SELECTOR, 34, spy);

			g_dirty = 0;
		}

		SDL_Delay(16);

		// Update real screen
		FCEUGUI_Flip();
	}

	// Must update emulation core and drivers
	UpdateEMUCore(g_config);
	FCEUD_DriverReset();

	// Clear screen
	dingoo_clear_video();

	g_dirty = 1;
}
