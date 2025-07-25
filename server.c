#include "includes.h"

static char debug_buf[1024];
static char buffer_out[1024];

unsigned long g_sig_received_time = 0;
unsigned long delay_restart = 30000;	//ms

int send_to_one(int fd_dest, const char *msg, int len){
	for(int i = 0; i < len; i++){
		sprintf(debug_buf + i * 3, "%02X ", (unsigned char)msg[i]); 
	}
	printf(MAGENTA"Sending to %d: %s\n"RESET, fd_dest, debug_buf);
	send(fd_dest, msg, len, 0);
	return 0;
}

int send_msg_all(int *fds, const char *msg, int len, int fd_orig, uint8_t dont_send_to_orig){
	for (int j = 0; j < MAX_CLIENTS; j++) {
		int out = fds[j];
		if (out > 0) {
			if (dont_send_to_orig == 0 || out != fd_orig){
				for(int i = 0; i < len; i++){
					sprintf(debug_buf + i * 3, "%02X ", (unsigned char)msg[i]); 
				}
				printf(B_MAGENTA"Sending to %d: %s\n"RESET, out, debug_buf);
				send(out, msg, len, 0);
			}
		}
	}
	return 0;
}

int send_txt_all_calc_dist(Game *game, int *fds, const char *msg, int len, int fd_orig, int id_orig){
	int id_dec = fd_orig - id_orig;
	if (id_dec < 0 || fd_orig <= 0) {
		printf("Erreur: send txt: id_dec or fd_orig\n");	
		return -1;
	}

	char new_txt[256] = {0};

	Player *client_orig = &game->clients[id_orig];
	if (client_orig->connected == 0 || client_orig->x <= 0 || client_orig->y <= 0) {
		printf("Erreur: send txt: from client not connected\n");
		return -1;
	}

	for (int j = 0; j < MAX_CLIENTS; j++) {
		int out = fds[j];

		if (out > 0) {
			int id = out - id_dec;
			Player *client_dest = &game->clients[id];

			if (client_dest->connected == 0 || client_dest->x <= 0 || client_dest->y <= 0) {
				printf("Erreur: send txt: to client not connected or wrong position\n");
				return -1;
			}

			// printf("PosO: %d, %d ; PosD: %d, %d\n", client_orig->x, client_orig->y, client_dest->x, client_dest->y);

			if (out != fd_orig){
				int dist = shortest_distance(game->map.map, game->map.w, game->map.h, client_orig->x, client_orig->y, client_dest->x, client_dest->y);
				
				if (dist > dist_max_txt_msg){	// TOO FAR, NOT SEND
					printf("Client %d to %d dist %d > Ignore\n", id_orig, id, dist);
				} else {
					if (dist > DIST_NOISE){		// SOME TEXT LOSS
						printf("Client %d to %d dist %d > Noise\n", id_orig, id, dist);
						int chances_noise = ((dist - DIST_NOISE) * 100) / (DIST_NO_HEAR - DIST_NOISE);
						int ic = 3;
						memcpy(new_txt, msg, ic);
						while (msg[ic]){
							if (rand() % 100 < chances_noise){
								new_txt[ic] = '.';
							} else {
								new_txt[ic] = msg[ic];
							}
							ic++;
						}
						new_txt[ic] = '\0';
						send_to_one(out, new_txt, len);
					} else {					// CLOSE ENOUGH SO JUST SEND THE MSG
						printf("Client %d to %d dist %d > OK\n", id_orig, id, dist);
						send_to_one(out, msg, len);
					}
				}
			}
		}
	}
	return 0;
}

int serv_recv_msg(Game *game, int fd_client, int id_client, const char *msg, int len){
	int id = id_client; // Assuming client fds start from 3
	char buf_len = 0;

	if (len < 1 || !msg) {
		printf(B_RED"Error: Empty message\n"RESET);
		return -1; // No message to process
	}
	if (id >= MAX_CLIENTS) {
		printf(B_RED"Error: Wrong id: %d\n"RESET, id);
		return -1; // Invalid client fd
	}

	size_t pos = 0;
	while (pos < len){
		char *ptr 		= (char*)msg + pos;
		uint8_t msg_len = (uint8_t)ptr[0];

		if (msg_len > len) return -1;

		// ******* DEBUG *******
		for(int i = 0; i < msg_len; i++){
			sprintf(debug_buf + i * 3, "%02X ", (unsigned char)ptr[i]); 
		}
		printf(YELLOW"\n>> Client %d, fd:%d : %s\n"RESET, id_client, fd_client, debug_buf);
		// **********************

		if (game->clients[id].connected == 0){
			if (ptr[1] != (char)B_NEW_CLIENT) return -1; // Client not connected
			if (msg_len < 8) return -1;

			game->clients[id].color = ptr[4];
			game->clients[id].x		= ptr[5];
			game->clients[id].y		= ptr[6];
			game->clients[id].face_id = ptr[7];
			game->clients[id].body_id = ptr[8];
			game->clients[id].legs_id = ptr[9];

			int name_len = msg_len - 10;
			if (name_len > 20) name_len = 20;
			char requested_name[20];
			memcpy(requested_name, ptr + 10, name_len);
			requested_name[name_len] = '\0';

			memcpy(game->clients[id].name, requested_name, name_len);
			game->clients[id].name[name_len] = '\0'; 

			// Check if another client has the same pseudo
			int pseudo_already_exists = 0;
			int pseudo_number = 0;
			do {
				pseudo_already_exists = 0;
				for(int i = 0; i < MAX_CLIENTS; i++){
					if (game->clients[i].connected){
						if (strcmp(game->clients[i].name, game->clients[id].name) == 0){
							pseudo_number++;
							sprintf(game->clients[id].name, "%s_%d", requested_name, pseudo_number);
							pseudo_already_exists = 1;
							printf("Client pseudo changed to %s\n", game->clients[id].name);
							break;
						}
					}
				}
			} while (pseudo_already_exists);

			name_len = strlen(game->clients[id].name);

			game->clients[id].connected = 1; // Mark as having

			printf(B_GREEN"[Client %d] joined: '%s'\n"RESET, id, game->clients[id].name);

			// Send connected clients info to the new client
			for(int i = 1; i < MAX_CLIENTS; i++) {
				if (game->clients[i].connected && i != id) {
					buf_len = 10 + strlen(game->clients[i].name);
					sprintf(buffer_out, "%c%c%c%c%c%c%c%c%c%c%s", buf_len,
															(char)B_CLIENT_INFOS, 
															(char)i, 
															1, 
															game->clients[i].color, 
															game->clients[i].x,
															game->clients[i].y,
															game->clients[i].face_id,
															game->clients[i].body_id,
															game->clients[i].legs_id,
															game->clients[i].name);
					send_to_one(fd_client, buffer_out, buf_len);
				}
			}

			// Inform other clients
			buf_len = 10 + strlen(game->clients[id].name);
			sprintf(buffer_out, "%c%c%c%c%c%c%c%c%c%c%s", buf_len,	
													(char)B_CLIENT_INFOS, 
													(char)id, 
													1, 
													game->clients[id].color, 
													game->clients[id].x,
													game->clients[id].y,
													game->clients[id].face_id,
													game->clients[id].body_id,
													game->clients[id].legs_id,
													game->clients[id].name);

			send_msg_all(game->com.client_fds, buffer_out, buf_len, fd_client, DONT_SEND_ORIG);

			pos += msg_len;
			
			return 0;
		}

		switch ((uint8_t)ptr[1]){
			case B_POS:
				if (msg_len != L_CLIENT_POS) return -1; // Invalid position message length

				game->clients[id].x = ptr[3];
				game->clients[id].y = ptr[4];
				
				printf(B_BLUE"[Client %d] moved (%d, %d)\n"RESET, id, game->clients[id].x, game->clients[id].y);
				
				// Send position update to all clients
				sprintf(buffer_out, "%c%c%c%c%c", L_CLIENT_POS, (char)B_POS, id, game->clients[id].x, game->clients[id].y);
				send_msg_all(game->com.client_fds, buffer_out, L_CLIENT_POS, fd_client, DONT_SEND_ORIG);
				
				pos += msg_len;

				break;
			
			case B_MESSAGE:
				if (msg_len < 3 || msg_len > 255) return -1; // Invalid message length
				char text[256] = {0};
				memcpy(text, ptr + 3, msg_len - 3); // Copy message text

				printf(B_BLUE"[Client %d] said: '%s'\n"RESET, id, text);

				// Send to other clients
				sprintf(buffer_out, "%c%c%c%s", msg_len, (char)B_MESSAGE, (char)id, text);
				// send_msg_all(game->com.client_fds, buffer_out, msg_len, fd_client, DONT_SEND_ORIG);
				send_txt_all_calc_dist(game, game->com.client_fds, buffer_out, msg_len, fd_client, id);

				pos += msg_len;

				break;
			
			case B_COLOR:
				if (msg_len != L_CLIENT_COLOR) return -1;

				if (ptr[3] >= MIN_COLOR && ptr[3] <= MAX_COLOR){
					printf(B_BLUE"[Client %d] new color: %d\n"RESET, id, ptr[3]);
					game->clients[id].color = ptr[3];

					// Send to other clients
					sprintf(buffer_out, "%c%c%c%c", L_CLIENT_COLOR, (char)B_COLOR, (char)id, ptr[3]);
					send_msg_all(game->com.client_fds, buffer_out, L_CLIENT_COLOR, fd_client, DONT_SEND_ORIG);

				} else {
					printf(RED"[Client %d] wrong color: %d\n"RESET, id, ptr[3]);
				}

				pos += msg_len;

				break;
			
			case B_DOOR_CHANGE:
				if (msg_len != L_DOOR_CHANGE) return -1;

				int door_id = (int)ptr[2];
				if (door_id < MAX_DOORS){
					printf(B_CYAN"[Client %d] Door: %d > %d\n"RESET, id, door_id, ptr[3]);
					game->map.doors[(int)door_id].state = ptr[4];

					// Send to other clients
					sprintf(buffer_out, "%c%c%c%c", L_DOOR_CHANGE, (char)B_DOOR_CHANGE, door_id, ptr[3]);
					send_msg_all(game->com.client_fds, buffer_out, L_DOOR_CHANGE, fd_client, DONT_SEND_ORIG);
				} else {
					printf(RED"[Client %d] wrong door: %d\n"RESET, id, door_id);
				}

				pos += msg_len;
				break;

			case B_PERSO_CHANGE:
				if (msg_len != L_PERSO_CHANGE) return -1;
				if (ptr[3] >= NB_FACES || ptr[4] >= NB_BODYS || ptr[5] >= NB_LEGS) return -1;
				printf(B_CYAN"[Client %d] Face: %d, Body: %d, Legs: %d\n"RESET, id, ptr[3], ptr[4], ptr[5]);
				game->clients[id].face_id = ptr[3];
				game->clients[id].body_id = ptr[4];
				game->clients[id].legs_id = ptr[5];

				// Send to other clients
				sprintf(buffer_out, "%c%c%c%c%c%c", L_PERSO_CHANGE, (char)B_PERSO_CHANGE, id, ptr[3], ptr[4], ptr[5]);
				send_msg_all(game->com.client_fds, buffer_out, L_PERSO_CHANGE, fd_client, DONT_SEND_ORIG);

				pos += msg_len;
				break;
		}
	}

	return 0;
}

void handle_sigint(int sig){
    (void)sig;  // unused
	printf(B_RED"\nSigINT recieved, shutdown now !\n"RESET);
    server_running = 0;
}

void handle_sigterm(int sig){
    (void)sig;  // unused
	printf(B_RED"\nSigTERM recieved, shutdown in %dsec\n"RESET, (int)delay_restart / 1000);
	g_sig_received_time = millis();
    // server_running = 0;
}


int main_server(Game *game){
	printf(B_GREEN"\nStarting server...\n"RESET);

	signal(SIGINT, handle_sigint);
	signal(SIGTERM, handle_sigterm);
	signal(SIGPIPE, SIG_IGN);	// ignore signal if pipe closed (client disconnected, error when send)

	int shutdown_msg_sent = 0;

	if (init_map(game, map) == -1) goto exit_point;

	int server_fd, client_fds[MAX_CLIENTS] = {0};
    struct sockaddr_in addr;
    char buffer[BUFFER_SIZE];

	game->com.client_fds 	= client_fds;
	game->com.selffd 		= &server_fd;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { 
		perror("socket"); 
		goto exit_point;
	}

	int opt = 1;
	if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		close(server_fd);
		goto exit_point;
	}

    addr.sin_family 		= AF_INET;
    addr.sin_addr.s_addr 	= INADDR_ANY;
    addr.sin_port 			= htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); close(server_fd); goto exit_point;
    }

    if (listen(server_fd, 5) < 0) {
        perror("listen"); close(server_fd); goto exit_point;
    }

	printf(B_GREEN"Server running !\n\n"RESET);

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
            // perror("select"); 
			continue;
        }

        if (FD_ISSET(server_fd, &readfds)) {
            int new_socket = accept(server_fd, NULL, NULL);
            if (new_socket < 0) { perror("accept"); continue; }

            int added = 0;
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_fds[i] == 0) {
                    client_fds[i] = new_socket;
					int id = i + 1;

					printf(B_CYAN"\n[NEW CONNECTION] ID: %d, FD: %d\n"RESET, id, new_socket);

                    added = 1;
					game->clients[id].connected = 0;
                    break;
                }
            }
            if (!added) {
                send(new_socket, "Server full\n", 12, 0);
                close(new_socket);
            }
        }

		// Check for messages from clients
        for (int i = 0; i < MAX_CLIENTS - 1; i++) {
            int fd = client_fds[i];
			int id = i + 1;

            if (fd > 0 && FD_ISSET(fd, &readfds)) {
                int len = recv(fd, buffer, sizeof(buffer) - 1, 0);

                if (len <= 0) {

					// Client disconnected
					printf(B_YELLOW"\n[DISCONNECTED] ID:%d, FD: %d\n"RESET, id, fd);
					
					int buf_len = strlen(game->clients[id].name) + 7;
					sprintf(buffer, "%c%c%c%c%c%s", buf_len, (char)B_CLIENT_INFOS, (char)id, 0, game->clients[id].color, game->clients[id].name);
					send_msg_all(game->com.client_fds, buffer, buf_len, fd, DONT_SEND_ORIG);

					game->clients[id].connected = 0; // Mark client as disconnected
					game->clients[id].name[0] = '\0'; // Clear name

                    close(fd);
                    client_fds[i] = 0;
                } else {
					
					// Message received
					serv_recv_msg(game, fd, id, buffer, len);

                }
            }
        }

		if (g_sig_received_time != 0){
			if (shutdown_msg_sent == 0){
				char shutdown_msg[128] = {0};
				sprintf(shutdown_msg, "!! Server restart in %dsec !!", (int)delay_restart / 1000);
				int msg_len = strlen(shutdown_msg);
				sprintf(buffer, "%c%c%c%s", msg_len, (char)B_MESSAGE, (char)0, shutdown_msg);
				send_msg_all(game->com.client_fds, buffer, msg_len, 0, DONT_SEND_ORIG);
				shutdown_msg_sent = 1;
			}
			if (millis() - g_sig_received_time > delay_restart){
				server_running = 0;
			}
		}

    }
	printf(B_RED"Shutdown server...\n"RESET);

exit_point:
    close(server_fd);

	if (game->map.map) free(game->map.map);

	return 0;
}