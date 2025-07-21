#include "includes.h"

// static char buffer_out[1024] = {0}; // Buffer for output messages
static char tmp_buffer[512] = {0}; // Temporary buffer for processing messages
static char debug_buf[1024] = {0}; // Buffer for debugging messages



int recv_msg(Game *game, const char *msg, int len){
	if (len < 1 || !msg) {
		wprintw(game->display.info, "Error: Empty message\n");
		wrefresh(game->display.info); 
		return -1; // No message to process
	}

	size_t pos = 0;
	while (pos < len){
		char *ptr 		= (char*)msg + pos;
		uint8_t msg_len = (uint8_t)ptr[0];
		
		if (msg_len > len) return -1;

		int id = ptr[2];

		if (id >= MAX_CLIENTS) {
			wprintw(game->display.info, "Error: Wrong id: %d\n", id);
			wrefresh(game->display.info); 
			return -1; // Invalid client fd
		}

		// ******* DEBUG *******
		for(int i = 0; i < msg_len; i++){
			sprintf(debug_buf + i * 3, "%02X ", (unsigned char)ptr[i]); 
		}
		pthread_mutex_lock(&game->display.m_display_update);
		wprintw(game->display.info, "Server: %s\n", debug_buf);
		wrefresh(game->display.info); 
		move_cursor_back(game);
		pthread_mutex_unlock(&game->display.m_display_update);
		// **********************
		
		switch ((uint8_t)ptr[1]){
			case B_POS:
				if (msg_len != L_CLIENT_POS) return -1; // Invalid position message length

				game->clients[id].x = ptr[3];
				game->clients[id].y = ptr[4];
				
				ask_for_display_update(game); // Ask for display update

				pos += msg_len;
				break;
			
			case B_MESSAGE:
				if (msg_len < 3 || msg_len > 255) return -1; // Invalid message length
				char text[256] = {0};
				memcpy(text, ptr + 3, msg_len - 3); // Copy message text

				// Add to chat
				add_message(game, game->clients[id].name, game->clients[id].color, text, id); // Add message to chat

				game->clients[id].last_msg = millis();

				ask_for_display_update(game);

				pos += msg_len;
				break;
			
			case B_COLOR:
				if (msg_len != L_CLIENT_COLOR) return -1;
				if (ptr[3] >= MIN_COLOR && ptr[3] <= MAX_COLOR) game->clients[id].color = ptr[3];
				pos += msg_len;
				ask_for_display_update(game);
				break;
			
			case B_CLIENT_INFOS:
				if (msg_len < 7) return -1; // Invalid client info message length
				if (ptr[2] < 0 || ptr[2] >= MAX_CLIENTS) return -1; // Invalid client id

				if (ptr[3] == 1) { // Client connected

					game->clients[id].connected = 1;
					game->clients[id].color = ptr[4]; // Set color
					game->clients[id].x = ptr[5];
					game->clients[id].y = ptr[6];
					memcpy(game->clients[id].name, ptr + 7, msg_len - 7); // Copy name
					game->clients[id].name[msg_len - 7] = '\0'; // Null-terminate name

					sprintf(tmp_buffer, "%s joined", game->clients[id].name);
					add_message(game, "[Server]", game->clients[id].color, tmp_buffer, -1); // Add message to chat

				} else if (ptr[3] == 0) { // Client disconnected

					sprintf(tmp_buffer, "%s disconnected", game->clients[id].name);
					add_message(game, "[Server]", game->clients[id].color, tmp_buffer, -1); // Add message to chat
					
					game->clients[id].connected = 0;
					game->clients[id].name[0] = '\0'; // Clear name
					game->clients[id].color = 0; // Reset color
				} 

				ask_for_display_update(game);

				pos += msg_len;
				break;
			
			case B_DOOR_CHANGE:
				if (msg_len != L_DOOR_CHANGE) return -1;

				int door_id = (int)ptr[2];
				if (door_id < MAX_DOORS){
					if (ptr[3]) {
						game->map.doors[door_id].state = 1;
						game->map.map[game->map.doors[door_id].y * game->map.w + game->map.doors[door_id].x] = 'V';
					} else {
						game->map.doors[door_id].state = 0;
						game->map.map[game->map.doors[door_id].y * game->map.w + game->map.doors[door_id].x] = 'v';
					}
				}

				ask_for_display_update(game);

				pos += msg_len;
				break;
		}
	}

	return 0;
}

