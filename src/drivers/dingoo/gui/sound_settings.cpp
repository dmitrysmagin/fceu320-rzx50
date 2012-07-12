
// Externals
extern Config *g_config;

/* MENU COMMANDS */

// Custom palette
static void sound_update(unsigned long key) {
	int val;

	if (key & DINGOO_RIGHT)
		val = 1;
	if (key & DINGOO_LEFT)
		val = 0;

	g_config->setOption("SDL.Sound", val);
}

// Sound rate
static void soundrate_update(unsigned long key) {
	const int rates[] = { 8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000 };
	int i = 0, val;
	g_config->getOption("SDL.Sound.Rate", &val);

	for (i = 0; i < 8; i++)
		if (val == rates[i])
			break;

	if (key & DINGOO_RIGHT)
		i = i < 7 ? i + 1 : 0;
	if (key & DINGOO_LEFT)
		i = i > 0 ? i - 1 : 7;

	g_config->setOption("SDL.Sound.Rate", rates[i]);
}

// Sound thread priority
static void priority_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.Priority", &val);

	if (key & DINGOO_RIGHT)
		val = val < 19 ? val + 1 : -20;
	if (key & DINGOO_LEFT)
		val = val > -20 ? val - 1 : 19;

	g_config->setOption("SDL.Sound.Priority", val);
}

// Custom palette
static void lowpass_update(unsigned long key) {
	int val, tmp;

	if (key & DINGOO_RIGHT)
		val = 1;
	if (key & DINGOO_LEFT)
		val = 0;

	g_config->setOption("SDL.Sound.LowPass", val);
}

// Sound volume
static void volume_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.Volume", &val);

	if (key & DINGOO_RIGHT)
		val = val < 256 ? val + 1 : 0;
	if (key & DINGOO_LEFT)
		val = val > 0 ? val - 1 : 256;

	g_config->setOption("SDL.Sound.Volume", val);
}

// Triangle volume
static void triangle_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.TriangleVolume", &val);

	if (key & DINGOO_RIGHT)
		val = val < 256 ? val + 1 : 0;
	if (key & DINGOO_LEFT)
		val = val > 0 ? val - 1 : 256;

	g_config->setOption("SDL.Sound.TriangleVolume", val);
}

// Square 1 volume
static void square1_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.Square1Volume", &val);

	if (key & DINGOO_RIGHT)
		val = val < 256 ? val + 1 : 0;
	if (key & DINGOO_LEFT)
		val = val > 0 ? val - 1 : 256;

	g_config->setOption("SDL.Sound.Square1Volume", val);
}

// Square 2 volume
static void square2_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.Square2Volume", &val);

	if (key & DINGOO_RIGHT)
		val = val < 256 ? val + 1 : 0;
	if (key & DINGOO_LEFT)
		val = val > 0 ? val - 1 : 256;

	g_config->setOption("SDL.Sound.Square2Volume", val);
}

// Noise volume
static void noise_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.NoiseVolume", &val);

	if (key & DINGOO_RIGHT)
		val = val < 256 ? val + 1 : 0;
	if (key & DINGOO_LEFT)
		val = val > 0 ? val - 1 : 256;

	g_config->setOption("SDL.Sound.NoiseVolume", val);
}

// PCM volume
static void pcm_update(unsigned long key) {
	int val;
	g_config->getOption("SDL.Sound.PCMVolume", &val);

	if (key & DINGOO_RIGHT)
		val = val < 256 ? val + 1 : 0;
	if (key & DINGOO_LEFT)
		val = val > 0 ? val - 1 : 256;

	g_config->setOption("SDL.Sound.PCMVolume", val);
}

/* SOUND SETTINGS MENU */
static SettingEntry sd_menu[] = {
	{ "Toggle sound", "Enable sound", "SDL.Sound", sound_update },
	//{ "Sound rate",	"Sound playback rate (Hz)", "SDL.Sound.Rate", soundrate_update },
	{ "Sound priority", "Sets sound thread priority", "SDL.Sound.Priority",	priority_update },
	{ "Lowpass", "Enables low-pass filter",	"SDL.Sound.LowPass", lowpass_update },
	{ "Volume", "Sets global volume", "SDL.Sound.Volume", volume_update },
	{ "Triangle volume", "Sets Triangle volume", "SDL.Sound.TriangleVolume", triangle_update },
	{ "Square1 volume", "Sets Square 1 volume",	"SDL.Sound.Square1Volume", square1_update },
	{ "Square2 volume",	"Sets Square 2 volume", "SDL.Sound.Square2Volume", square2_update },
	{ "Noise volume", "Sets Noise volume", "SDL.Sound.NoiseVolume",	noise_update },
	{ "PCM volume", "Sets PCM volume", "SDL.Sound.PCMVolume", pcm_update }
};

int RunSoundSettings() {
	static int index = 0;
	static int spy = 74;
	int done = 0, y, i;

	int max_entries = 8;
	int menu_size = 9;

	static int offset_start = 0;
	static int offset_end = menu_size > max_entries ? max_entries : menu_size;

	char tmp[32];
	int itmp;

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

				if (index >= offset_start)
					spy -= 15;

				if ((offset_start > 0) && (index < offset_start)) {
					offset_start--;
					offset_end--;
				}
			} else {
				index = menu_size-1;
				offset_end = menu_size;
				offset_start = menu_size <= max_entries ? 0 : offset_end - max_entries;
				spy = 74 + 15*(index - offset_start);
			}
		}

		if (parsekey(DINGOO_DOWN, 1)) {
			if (index < (menu_size - 1)) {
				index++;

				if (index < offset_end)
					spy += 15;

				if ((offset_end < menu_size) && (index >= offset_end)) {
					offset_end++;
					offset_start++;
				}
			} else {
				index = 0;
				offset_start = 0;
				offset_end = menu_size <= max_entries ? menu_size : max_entries;
				spy = 74;
			}
		}

		if (parsekey(DINGOO_RIGHT, 1) || parsekey(DINGOO_LEFT, 1))
			sd_menu[index].update(g_key);

		// Draw stuff
		if (g_dirty) {
			draw_bg(vbuffer, g_bg);

			// Draw time and battery every minute
			DrawText(vbuffer, g_time, 148, 5);
			DrawText(vbuffer, g_battery, 214, 5);

			DrawChar(vbuffer, SP_SOUND_SETTINGS, 40, 38);

			// Draw menu
			for (i = offset_start, y = 70; i < offset_end; i++, y += 15) {
				DrawText(vbuffer, sd_menu[i].name, 60, y);

				g_config->getOption(sd_menu[i].option, &itmp);
				sprintf(tmp, "%d", itmp);

				DrawText(vbuffer, tmp, 210, y);
			}

			// Draw info
			DrawText(vbuffer, sd_menu[index].info, 16, 225);

			// Draw selector
			DrawChar(vbuffer, SP_SELECTOR, 44, spy);

			// Draw offset marks
			if (offset_start > 0)
				DrawChar(vbuffer, SP_UPARROW, 274, 62);
			if (offset_end < menu_size)
				DrawChar(vbuffer, SP_DOWNARROW, 274, 203);

			g_dirty = 0;
		}

		dingoo_timer_delay(16);

		// Update real screen
		memcpy(screen->pixels, vbuffer, 320 * 240 * sizeof(unsigned short));
		SDL_Flip(screen);
	}

	// Clear screen
	dingoo_clear_video();

	g_dirty = 1;
	return 0;
}
