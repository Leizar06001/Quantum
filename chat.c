#include "includes.h"

#define B_NEW_CLIENT 	0xF0
#define B_CLIENT_NAME	0xF1
#define B_CLIENT_COLOR 	0xF2
#define B_CLIENT_INFOS	0xF3

#define B_MESSAGE 		0xE0
#define B_POS 			0xE1
#define B_SERVER_MSG 	0xE2


#define SEND_TO_ALL		0
#define DONT_SEND_ORIG	1

char buffer_out[1024] = {0}; // Buffer for output messages
char tmp_buffer[512] = {0}; // Temporary buffer for processing messages

char debug_buf[1024] = {0}; // Buffer for debugging messages

int ask_for_display_update(Game *game){
	if (game == NULL) return -1; // Invalid game pointer

	pthread_mutex_lock(&game->display.m_display_update);
	game->display.need_main_update = 1; // Set the flag to update the main display
	pthread_mutex_unlock(&game->display.m_display_update);

	return 0;
}

int send_msg_all(int *fds, const char *msg, int len, int fd_orig, uint8_t dont_send_to_orig){
	for (int j = 0; j < MAX_CLIENTS; j++) {
		int out = fds[j];
		if (out > 0) {
			if (dont_send_to_orig == 0 || out != fd_orig){
				send(out, msg, len, 0);
			}
		}
	}
	return 0;
}

int recv_msg(Game *game, int fd_client, const char *msg, int len){
	int id = fd_client; // Assuming client fds start from 3

	if (len < 1 || !msg) return -1; // No message to process
	if (id < 0 || id >= MAX_CLIENTS) return -1; // Invalid client fd
	if (game == NULL) return -1; // Invalid game pointer

	// ******* DEBUG *******
	
	for(int i = 0; i < len; i++){
		sprintf(debug_buf + i * 3, "%02X ", (unsigned char)msg[i]); 
	}
	wprintw(game->display.info, "Client %d: %s\n", fd_client, debug_buf);
	wrefresh(game->display.info); 
	// **********************

	if (game->server_mode) {
		if (game->clients[id].connected == 0){
			if (msg[0] != (char)B_NEW_CLIENT) return -1; // Client not connected

			int name_len = 0;
			int pos = 1;
			while (pos < len && msg[pos] != (char)B_CLIENT_NAME) {
				game->clients[id].name[name_len++] = msg[pos++];
			}
			game->clients[id].name[name_len] = '\0'; // Null
			pos++; // Skip B_CLIENT_NAME
			if (pos < len) {
				game->clients[id].color = msg[pos++];
				game->clients[id].connected = 1; // Mark as having

				sprintf(tmp_buffer, "%s joined", game->clients[id].name);
				add_message(game, "[Server]", 0, tmp_buffer); // Add message to chat

				// Inform other clients
				// B_CLIENT_INFOS, client fd, dis/connected, color, name
				sprintf(buffer_out, "%c%c%c%c%s", (char)B_CLIENT_INFOS, (char)fd_client, 1, game->clients[id].color, game->clients[id].name);
				int len = strlen(game->clients[id].name) + 4;

				send_msg_all(game->com.client_fds, buffer_out, len, fd_client, DONT_SEND_ORIG);

				// Send connected client info to this client
				for(int i = 0; i < MAX_CLIENTS; i++) {
					if (game->clients[i].connected && i != id) {
						sprintf(buffer_out, "%c%c%c%c%s", (char)B_CLIENT_INFOS, (char)i, 1, game->clients[i].color, game->clients[i].name);
						int len = strlen(game->clients[i].name) + 4;
						send(fd_client, buffer_out, len, 0); // Send to the new client
					}
				}
			}
			return 0;
		}
	}

	if (!game->server_mode){
		id = msg[1]; // Get client id from message
	}

	switch ((uint8_t)msg[0]){
		case B_POS:
			if (len < 4) return -1; // Invalid position message length

			game->clients[id].x = msg[2];
			game->clients[id].y = msg[3];
			
			mvprintw(35 + id, 40, "Client %d (%d, %d)", id, game->clients[id].x, game->clients[id].y);
			ask_for_display_update(game); // Ask for display update
			
			if (game->server_mode){
				// Send position update to all clients
				sprintf(buffer_out, "%c%c%c%c", (char)B_POS, id, game->clients[id].x, game->clients[id].y);
				send_msg_all(game->com.client_fds, buffer_out, 4, fd_client, DONT_SEND_ORIG);
			}

			break;
		
		case B_MESSAGE:
			if (len < 3 || len > 255) return -1; // Invalid message length
			char text[256] = {0};
			memcpy(text, msg + 2, len - 2); // Copy message text

			// Add to chat
			add_message(game, game->clients[id].name, game->clients[id].color, text); // Add message to chat

			if (game->server_mode){
				// Send to other clients
				sprintf(buffer_out, "%c%c%s", (char)B_MESSAGE, (char)fd_client, text);
				int txtlen = strlen(text) + 2;

				send_msg_all(game->com.client_fds, buffer_out, txtlen, fd_client, DONT_SEND_ORIG);
			}
			break;
		
		case B_CLIENT_INFOS:
			if (len < 5) return -1; // Invalid client info message length
			if (msg[1] < 0 || msg[1] >= MAX_CLIENTS) return -1; // Invalid client id
			int client_id = msg[1];
			if (msg[2] == 1) { // Client connected
				game->clients[client_id].connected = 1;
				game->clients[client_id].color = msg[3]; // Set color
				memcpy(game->clients[client_id].name, msg + 4, len - 4); // Copy name
				game->clients[client_id].name[len - 4] = '\0'; // Null-terminate name

				sprintf(tmp_buffer, "%s joined", game->clients[client_id].name);
				add_message(game, "[Server]", 0, tmp_buffer); // Add message to chat
			} else if (msg[2] == 0) { // Client disconnected
				game->clients[client_id].connected = 0;
				game->clients[client_id].name[0] = '\0'; // Clear name
				game->clients[client_id].color = 0; // Reset color

				sprintf(tmp_buffer, "%s disconnected", game->clients[client_id].name);
				add_message(game, "[Server]", 0, tmp_buffer); // Add message to chat
			} 

			break;
	}

	return 0;
}

void *chat_server_thread(void *arg) {
	Game *game = (Game *)arg;
	
    int server_fd, client_fds[MAX_CLIENTS] = {0};
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];

	game->com.client_fds 	= client_fds;
	game->com.selffd 		= &server_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return NULL; }

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(server_fd);
		exit(1);
	}

    addr.sin_family 		= AF_INET;
    addr.sin_addr.s_addr 	= INADDR_ANY;
    addr.sin_port 			= htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); return NULL;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen"); close(server_fd); return NULL;
    }

    while (server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        int maxfd = server_fd;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_fds[i] > 0) {
                FD_SET(client_fds[i], &readfds);
                if (client_fds[i] > maxfd) maxfd = client_fds[i];
            }
        }

		struct timeval timeout = {0, 1000};  // 0 seconds, 0 microseconds

        if (select(maxfd + 1, &readfds, NULL, NULL, &timeout) < 0) {
            perror("select"); continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket = accept(server_fd, NULL, NULL);
            if (new_socket < 0) { perror("accept"); continue; }

            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = new_socket;
					sprintf(buffer, "New client: %d", new_socket);
					add_message(game, "[Server]", 0, buffer); // Add message to chat
                    added = 1;
					game->clients[client_fds[i] - 3].connected = 0;
                    break;
                }
            }
            if (!added) {
                send(new_socket, "Server full\n", 12, 0);
                close(new_socket);
            }
        }

		// Check for messages from clients
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int fd = client_fds[i];
            if (fd > 0 && FD_ISSET(fd, &readfds)) {
                int len = recv(fd, buffer, sizeof(buffer) - 1, 0);
                if (len <= 0) {
					sprintf(buffer, "Client %d disconnected", fd);
					game->clients[i].connected = 0; // Mark client as disconnected
					game->clients[i].name[0] = '\0'; // Clear name
					add_message(game, "[Server]", 0, buffer); // Add message to chat
                    close(fd);
                    client_fds[i] = 0;
                } else {
					
					recv_msg(game, fd, buffer, len);

					
					
                }
            }
        }


		// Get mutex lock for sending text
		pthread_mutex_lock(&game->chat.m_send_text);

		// NEW TEXT TO SEND
		if (game->chat.ready_to_send) {
			sprintf(buffer, "%c%c%s", (char)B_MESSAGE, 0, game->chat.text_to_send);
			send_msg_all(game->com.client_fds, buffer, strlen(game->chat.text_to_send) + 2, -1, SEND_TO_ALL);
			game->chat.text_to_send[0] = '\0'; // Clear the text to send
			game->chat.ready_to_send = 0; // Reset the flag
		}

		// NEW POS TO SEND
		if (game->chat.new_pos) {
			sprintf(buffer, "%c%c%c%c", (char)B_POS, 0, game->player.lastx, game->player.lasty);
			send_msg_all(game->com.client_fds, buffer, 4, -1, SEND_TO_ALL);
			game->chat.new_pos = 0; // Reset the new position flag
		}
		pthread_mutex_unlock(&game->chat.m_send_text); // Release the mutex if no text to send

    }
	server_running = 0;

    close(server_fd);
    return NULL;
}

void *chat_client_thread(void *arg) {
	Game *game = (Game *)arg;
	int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE];

	// mvprintw(40, 0, "Connecting to %s", game->server_ip);

    // 1. Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) { 
		perror("socket"); 
		strcpy(game->exit_error, "Failed to create socket.");
		game->print_error = 1; // Set print error flag
		return NULL; 
	}

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, game->server_ip, &serv_addr.sin_addr) <= 0) {
        perror("inet_pton"); 
		close(sockfd);
		strcpy(game->exit_error, "Invalid server IP address.");
		game->print_error = 1; // Set print error flag
		return NULL;
    }

    // 2. Connect to server
    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect"); 
		close(sockfd); 
		strcpy(game->exit_error, "Failed to connect to server.");
		game->print_error = 1; // Set print error flag
		return NULL;
    }

	game->clients[0].connected = 1; // Mark client as connected
	game->clients[0].color = 2;
	strncpy(game->clients[0].name, "Server", sizeof(game->clients[0].name) - 1);

	char send_buf[64];
	send_buf[0] = (char)0xF0;
	strncpy(send_buf + 1, game->player.name, sizeof(send_buf) - 2);
	int buf_pos = strlen(game->player.name) + 1;
	send_buf[buf_pos++] = (char)0xF1; // Color
	send_buf[buf_pos++] = game->player.color; // Color
	send(sockfd, send_buf, buf_pos, 0); // Send player info

    // 3. Main loop with select()
    while (server_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        // FD_SET(0, &readfds);        // stdin
        FD_SET(sockfd, &readfds);   // server socket
        int maxfd = sockfd;

		struct timeval timeout = {0, 1000};  // 0 seconds, 0 microseconds

        int activity = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

		// Get mutex lock for sending text
		pthread_mutex_lock(&game->chat.m_send_text);

		// NEW TEXT TO SEND
		if (game->chat.ready_to_send) {
			sprintf(buffer, "%c%c%s", (char)B_MESSAGE, 0, game->chat.text_to_send);
			if (send(sockfd, buffer, strlen(game->chat.text_to_send) + 2, 0) < 0) {
				perror("send");
				break;
			}
			game->chat.text_to_send[0] = '\0'; // Clear the text to send
			game->chat.ready_to_send = 0; // Reset the flag
		}

		// NEW POS TO SEND
		if (game->chat.new_pos) {
			sprintf(buffer, "%c%c%c%c", (char)B_POS, 0, (char)game->player.lastx, (char)game->player.lasty);
			if (send(sockfd, buffer, 4, 0) < 0) {
				perror("send");
				break;
			}
			game->chat.new_pos = 0; // Reset the new position flag
		}
		pthread_mutex_unlock(&game->chat.m_send_text); // Release the mutex if no text to send

        // 5. Read from server
        if (FD_ISSET(sockfd, &readfds)) {
            int len = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
            if (len <= 0) {
				strcpy(game->exit_error, "Disconnected from server.");
				game->print_error = 1; // Set print error flag
                break;
            }
            buffer[len] = '\0';

			recv_msg(game, sockfd, buffer, len); // Process received message
        }
    }
	server_running = 0;

	mvprintw(42, 0, "EXIT COM THREAD"); // Print sent message
	refresh(); // Refresh to show the message

    close(sockfd);
    return NULL;
}