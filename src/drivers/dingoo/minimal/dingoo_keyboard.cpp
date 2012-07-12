/* faking/emulating gp2x.c by using gtk */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <termios.h>
#include <linux/vt.h>
#include <linux/kd.h>
//#include <linux/keyboard.h> 

/* RIPPED FROM THE SDL LIBS */

int keyboard_fd = -1, saved_kbd_mode = -1, current_vt = -1, saved_vt = -1;
struct termios saved_kbd_termios; 

#define SDL_arraysize(array)	(sizeof(array)/sizeof(array[0])) 

int OpenKeyboard(void)
{
	/* Open only if not already opened */
 	if ( keyboard_fd < 0 ) {
		static const char * const tty0[] = { "/dev/tty0", "/dev/vc/0", NULL };
		static const char * const vcs[] = { "/dev/vc/%d", "/dev/tty%d", NULL };
		int i, tty0_fd;

		/* Try to query for a free virtual terminal */
		tty0_fd = -1;
		for ( i=0; tty0[i] && (tty0_fd < 0); ++i ) {
			tty0_fd = open(tty0[i], O_WRONLY, 0);
		}
		if ( tty0_fd < 0 ) {
			tty0_fd = dup(0); /* Maybe stdin is a VT? */
		}
		ioctl(tty0_fd, VT_OPENQRY, &current_vt);
		close(tty0_fd);

		if ( (geteuid() == 0) && (current_vt > 0) ) {
			for ( i=0; vcs[i] && (keyboard_fd < 0); ++i ) {
				char vtpath[12];

				snprintf(vtpath, SDL_arraysize(vtpath), vcs[i], current_vt);
				keyboard_fd = open(vtpath, O_RDWR, 0);

				/* This needs to be our controlling tty
				   so that the kernel ioctl() calls work
				*/
				if ( keyboard_fd >= 0 ) {
					tty0_fd = open("/dev/tty", O_RDWR, 0);
					if ( tty0_fd >= 0 ) {
						ioctl(tty0_fd, TIOCNOTTY, 0);
						close(tty0_fd);
					}
				}
			}
		}
 		if ( keyboard_fd < 0 ) {
			/* Last resort, maybe our tty is a usable VT */
			struct vt_stat vtstate;

			keyboard_fd = open("/dev/tty", O_RDWR);

			if ( ioctl(keyboard_fd, VT_GETSTATE, &vtstate) == 0 ) {
				current_vt = vtstate.v_active;
			} else {
				current_vt = 0;
			}
 		}

		/* Make sure that our input is a console terminal */
		{ int dummy;
		  if ( ioctl(keyboard_fd, KDGKBMODE, &dummy) < 0 ) {
			close(keyboard_fd);
			keyboard_fd = -1;
			printf("Unable to open a console terminal");
		  }
		}

 	}
 	return(keyboard_fd);
} 

int InGraphicsMode(void)
{
	return((keyboard_fd >= 0) && (saved_kbd_mode >= 0));
}

void LeaveGraphicsMode(void)
{
	if ( InGraphicsMode() ) {
		ioctl(keyboard_fd, KDSETMODE, KD_TEXT);
		ioctl(keyboard_fd, KDSKBMODE, saved_kbd_mode);
		tcsetattr(keyboard_fd, TCSAFLUSH, &saved_kbd_termios);
		saved_kbd_mode = -1;

		/* Head back over to the original virtual terminal */
		ioctl(keyboard_fd, VT_UNLOCKSWITCH, 1);
		if ( saved_vt > 0 ) {
			ioctl(keyboard_fd, VT_ACTIVATE, saved_vt);
		}
	}
}
 
void CloseKeyboard(void)
{
	if ( keyboard_fd >= 0 ) {
		LeaveGraphicsMode();
		if ( keyboard_fd > 0 ) {
			close(keyboard_fd);
		}
	}
	keyboard_fd = -1;
}
  
int EnterGraphicsMode(void)
{
	struct termios keyboard_termios;

	/* Set medium-raw keyboard mode */
	if ( (keyboard_fd >= 0) && !InGraphicsMode() ) {

		/* Switch to the correct virtual terminal */
		if ( current_vt > 0 ) {
			struct vt_stat vtstate;

			if ( ioctl(keyboard_fd, VT_GETSTATE, &vtstate) == 0 ) {
				saved_vt = vtstate.v_active;
			}
			if ( ioctl(keyboard_fd, VT_ACTIVATE, current_vt) == 0 ) {
				ioctl(keyboard_fd, VT_WAITACTIVE, current_vt);
			}
		}

		/* Set the terminal input mode */
		if ( tcgetattr(keyboard_fd, &saved_kbd_termios) < 0 ) {
			printf("ERROR: Unable to get terminal attributes\n");
			if ( keyboard_fd > 0 ) {
				close(keyboard_fd);
			}
			keyboard_fd = -1;
			return(-1);
		}
		if ( ioctl(keyboard_fd, KDGKBMODE, &saved_kbd_mode) < 0 ) {
			printf("ERROR: Unable to get current keyboard mode\n");
			if ( keyboard_fd > 0 ) {
				close(keyboard_fd);
			}
			keyboard_fd = -1;
			return(-1);
		}
		keyboard_termios = saved_kbd_termios;
		keyboard_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
		keyboard_termios.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
		keyboard_termios.c_cc[VMIN] = 0;
		keyboard_termios.c_cc[VTIME] = 0;
		if (tcsetattr(keyboard_fd, TCSAFLUSH, &keyboard_termios) < 0) {
			CloseKeyboard();
			printf("ERROR: Unable to set terminal attributes\n");
			return(-1);
		}
		/* This will fail if we aren't root or this isn't our tty */
		if ( ioctl(keyboard_fd, KDSKBMODE, K_MEDIUMRAW) < 0 ) {
			CloseKeyboard();
			printf("ERORR: Unable to set keyboard in raw mode\n");
			return(-1);
		}
		if ( ioctl(keyboard_fd, KDSETMODE, KD_GRAPHICS) < 0 ) {
			CloseKeyboard();
			printf("ERROR: Unable to set keyboard in graphics mode");
			return(-1);
		}
		/* Prevent switching the virtual terminal */
		ioctl(keyboard_fd, VT_LOCKSWITCH, 1);
	}
	return(keyboard_fd);
} 

