#ifndef ROUTER_H
#define ROUTER_H

/**
 * @file router.h
 * @brief Command routing layer - dispatches parsed commands to business logic
 * 
 * This module bridges the network layer (connect.c) with business logic (session.c, users.c).
 * It receives parsed commands and routes them to appropriate handlers.
 */

/**
 * @brief Routes a command from client to the appropriate handler
 * 
 * Called by connect.c when a complete line is received from client.
 * Parses the command type and delegates to session/user/game handlers.
 * 
 * @param client_sock Socket file descriptor
 * @param command Raw command string (e.g., "REGISTER user1 pass123")
 * 
 * TODO: This function should:
 * 1. Parse command using parse_command() from command.h
 * 2. Look up session by socket using find_session_by_socket()
 * 3. Route to handlers in session.c:
 *    - REGISTER -> server_handle_register()
 *    - LOGIN -> server_handle_login()
 *    - WHOAMI -> server_handle_whoami()
 *    - BYE/LOGOUT -> server_handle_bye()
 *    - GETCOIN -> direct user table lookup
 *    - GETARMOR -> find_ship() + format response
 *    - BUYARMOR -> server_handle_buyarmor()
 * 4. Format response and call connection_send()
 * 5. Handle errors gracefully (RESP_SYNTAX_ERROR, RESP_NOT_LOGGED, etc.)
 */
void command_routes(int client_sock, char *command);

#endif // ROUTER_H
