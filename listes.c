#include "includes.h"

void add_message(Game *game, const char *name, const int color, const char *text) {
	if (!game || !name || !text) return; // Vérification de la validité des pointeurs

	Messages *new_message = malloc(sizeof(Messages));
	if (!new_message) return; // Vérification de l'allocation mémoire

	int name_length = strlen(name);
	int text_length = strlen(text);
	strncpy(new_message->name, name, name_length);
	new_message->name[name_length] = '\0'; // Assurer la terminaison de la chaîne
	new_message->color = color;
	strncpy(new_message->text_buffer, text, text_length);
	new_message->text_buffer[text_length] = '\0'; // Assurer la terminaison de la chaîne
	new_message->next = NULL;

	pthread_mutex_lock(&game->chat.m_chat_box);
	
	if (game->chat.messages == NULL) {
		game->chat.messages = new_message; // Premier message
	} else {
		Messages *current = game->chat.messages;
		while (current->next != NULL) {
			current = current->next; // Trouver le dernier message
		}
		current->next = new_message; // Ajouter le nouveau message à la fin
	}

	pthread_mutex_unlock(&game->chat.m_chat_box);
}

void clear_messages(Game *game) {
	if (!game) return; // Vérification de la validité du pointeur

	pthread_mutex_lock(&game->chat.m_chat_box);
	Messages *current = game->chat.messages;
	while (current != NULL) {
		Messages *next = current->next;
		free(current); // Libérer la mémoire du message
		current = next; // Passer au message suivant
	}
	game->chat.messages = NULL; // Réinitialiser la liste des messages
	pthread_mutex_unlock(&game->chat.m_chat_box);
}

void print_messages(Game *game) {
	if (!game) return; // Vérification de la validité du pointeur

	pthread_mutex_lock(&game->chat.m_chat_box);
	Messages *current = game->chat.messages;
	while (current != NULL) {
		wattron(game->display.chat, COLOR_PAIR(current->color + 1)); // Activer la couleur du message
		wprintw(game->display.chat, " %s: %s\n", current->name, current->text_buffer); // Afficher le message
		wattroff(game->display.chat, COLOR_PAIR(current->color + 1)); // Désactiver la couleur

		Messages *next = current->next; // Sauvegarder le pointeur vers le message suivant
		free(current); // Libérer la mémoire du message actuel
		current = next; // Passer au message suivant
		wrefresh(game->display.chat); // Rafraîchir la fenêtre de chat
	}
	game->chat.messages = NULL; // Réinitialiser la liste des messages après affichage
	pthread_mutex_unlock(&game->chat.m_chat_box);
}