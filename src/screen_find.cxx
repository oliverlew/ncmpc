/* ncmpc (Ncurses MPD Client)
 * (c) 2004-2018 The Music Player Daemon Project
 * Project homepage: http://musicpd.org
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "screen_find.hxx"
#include "screen_utils.hxx"
#include "screen_status.hxx"
#include "screen.hxx"
#include "keyboard.hxx"
#include "i18n.h"
#include "options.hxx"

#define FIND_PROMPT  _("Find")
#define RFIND_PROMPT _("Find backward")
#define JUMP_PROMPT _("Jump")

/* query user for a string and find it in a list window */
bool
screen_find(ScreenManager &screen, ListWindow *lw, command_t findcmd,
	    list_window_callback_fn_t callback_fn,
	    void *callback_data)
{
	bool found;
	const char *prompt = FIND_PROMPT;

	const bool reversed =
		findcmd == CMD_LIST_RFIND || findcmd == CMD_LIST_RFIND_NEXT;
	if (reversed)
		prompt = RFIND_PROMPT;

	switch (findcmd) {
	case CMD_LIST_FIND:
	case CMD_LIST_RFIND:
		if (screen.findbuf) {
			g_free(screen.findbuf);
			screen.findbuf=nullptr;
	}
		/* fall through */

	case CMD_LIST_FIND_NEXT:
	case CMD_LIST_RFIND_NEXT:
		if (!screen.findbuf) {
			char *value = options.find_show_last_pattern
				? (char *) -1 : nullptr;
			screen.findbuf=screen_readln(prompt,
						     value,
						     &screen.find_history,
						     nullptr);
		}

		if (screen.findbuf == nullptr)
			return true;

		found = reversed
			? lw->ReverseFind(callback_fn, callback_data,
					  screen.findbuf,
					  options.find_wrap,
					  options.bell_on_wrap)
			: lw->Find(callback_fn, callback_data,
				   screen.findbuf,
				   options.find_wrap,
				   options.bell_on_wrap);
		if (!found) {
			screen_status_printf(_("Unable to find \'%s\'"),
					     screen.findbuf);
			screen_bell();
		}
		return true;
	default:
		break;
	}
	return false;
}

/* query user for a string and jump to the entry
 * which begins with this string while the users types */
void
screen_jump(ScreenManager &screen, ListWindow *lw,
	    list_window_callback_fn_t callback_fn, void *callback_data,
	    list_window_paint_callback_t paint_callback, void *paint_data)
{
	const int WRLN_MAX_LINE_SIZE = 1024;
	int key = 65;

	if (screen.findbuf) {
		g_free(screen.findbuf);
		screen.findbuf = nullptr;
	}
	screen.findbuf = (char *)g_malloc0(WRLN_MAX_LINE_SIZE);
	/* In screen.findbuf is the whole string which is displayed in the status_window
	 * and search_str is the string the user entered (without the prompt) */
	char *search_str = screen.findbuf + g_snprintf(screen.findbuf, WRLN_MAX_LINE_SIZE, "%s: ", JUMP_PROMPT);
	char *iter = search_str;

	while(1) {
		key = screen_getch(screen.findbuf);
		/* if backspace or delete was pressed, process instead of ending loop */
		if (key == KEY_BACKSPACE || key == KEY_DC) {
			int i;
			if (search_str <= g_utf8_find_prev_char(screen.findbuf, iter))
				iter = g_utf8_find_prev_char(screen.findbuf, iter);
			for (i = 0; *(iter + i) != '\0'; i++)
				*(iter + i) = '\0';
			continue;
		}
		/* if a control key was pressed, end loop */
		else if (g_ascii_iscntrl(key) || key == KEY_NPAGE || key == KEY_PPAGE) {
			break;
		}
		else {
			*iter = key;
			if (iter < screen.findbuf + WRLN_MAX_LINE_SIZE - 3)
				++iter;
		}
		lw->Jump(callback_fn, callback_data, search_str);

		/* repaint the list_window */
		if (paint_callback != nullptr)
			lw->Paint(paint_callback, paint_data);
		else
			lw->Paint(callback_fn, callback_data);
		wrefresh(lw->w);
	}

	char *temp = g_strdup(search_str);
	g_free(screen.findbuf);
	screen.findbuf = temp;

	/* ncmpc should get the command */
	keyboard_unread(key);
}
