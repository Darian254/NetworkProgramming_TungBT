#pragma once

/**
 * @brief Routes the command to the appropriate handler function.
 * @param client_sock The socket file descriptor of the client.
 * @param command The command string received from the client.
 */
void command_routes(int client_sock, char *command);
