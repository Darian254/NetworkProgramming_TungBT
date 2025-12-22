#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

#define BUFF_SIZE 4096
#define PORT 5500
#define BACKLOG 32
#define MAX_CLIENTS 10000
#define MAX_EVENTS 1024
#define DESIRED_NOFILE_LIMIT 65535
#define USERS_FILE "TCP_Server/users.txt"
#define HASH_SIZE 101

/**
 * @enum FunctionId
 * @brief IDs for user menu actions
 */
typedef enum {
    FUNC_REGISTER = 0,  /**< Register new account */
    FUNC_LOGIN  = 1,  /**< User login */
    FUNC_LOGOUT = 2,  /**< Logout */
    FUNC_WHOAMI = 3,  /**< Check current user */
    FUNC_EXIT   = 4,  /**< Exit program */
    FUNC_CHECK_COIN = 5,   /**< Check my coin balance */
    FUNC_CHECK_ARMOR = 6,  /**< Check my ship armor */
    FUNC_BUY_ARMOR = 7,     /**< Buy armor for my ship */
    FUNC_START_MATCH = 8,  /**< Start a match */
    FUNC_GET_MATCH_RESULT = 9 /**< Check match result */
} FunctionId;

/**
 * @brief Response codes for the TCP server protocol
 */
typedef enum {
    /* Success codes */
    RESP_REGISTER_OK = 100,       /**< Registration successful */
    RESP_LOGIN_OK = 110,          /**< Login successful */
    RESP_LOGOUT_OK = 134,         /**< Logout successful */
    RESP_WELCOME = 120,           /**< Initial connection greeting */
    RESP_WHOAMI_OK = 201,         /**< Whoami successful */
    RESP_COIN_OK = 202,           /**< Get coin successful */
    RESP_ARMOR_INFO_OK = 203,     /**< Get armor info successful */
    RESP_BUY_ITEM_OK = 334,       /**< Buy item successful */
    RESP_START_MATCH_OK = 126,    /**< Match started successfully */
    RESP_MATCH_RESULT_OK = 143,   /**< Match result retrieved successfully */
    
    /* Client error codes - Authentication */
    RESP_SYNTAX_ERROR = 301,      /**< Syntax error */
    RESP_NOT_CREATOR = 316,       /**< User is not team creator */
    RESP_ACCOUNT_LOCKED = 311,    /**< Account is locked */
    RESP_ACCOUNT_NOT_FOUND = 312, /**< Account does not exist */
    RESP_ALREADY_LOGGED = 313,    /**< Already logged in */
    RESP_WRONG_PASSWORD = 314,    /**< Incorrect password */
    RESP_NOT_LOGGED = 315,        /**< Not logged in */
    RESP_USERNAME_EXISTS = 331,   /**< Username already exists */
    RESP_BUY_ITEM_FAILED = 335,   /**< Buy item failed */
    RESP_INVALID_USERNAME = 402,  /**< Invalid username format */
    RESP_WEAK_PASSWORD = 403,     /**< Weak password */
    
    /* Match/Team error codes */
    RESP_TEAM_NOT_FOUND = 410,    /**< Team not found */
    RESP_OPPONENT_NOT_FOUND = 411, /**< Opponent team not found */
    RESP_TEAM_IN_MATCH = 412,     /**< Team already in a match */
    RESP_MATCH_CREATE_FAILED = 413, /**< Failed to create match */
    RESP_MATCH_NOT_FOUND = 414,   /**< Match not found */
    RESP_NOT_AUTHORIZED = 415,    /**< Not authorized to access match */
    RESP_MATCH_RUNNING = 416,     /**< Match is still running */
    
    /* Server error codes */
    RESP_INTERNAL_ERROR = 500,    /**< Internal server error */
    RESP_DATABASE_ERROR = 501,    /**< Database error */
    RESP_SERVER_BUSY = 503,       /**< Server too busy */
    RESP_NOT_IN_MATCH = 503,      /**< Not in a match */
    RESP_MATCH_STATE_ERROR = 504, /**< Match state does not allow action */
    
    /* Shop/Item error codes */
    RESP_ARMOR_NOT_FOUND = 520,   /**< Armor type does not exist */
    RESP_NOT_ENOUGH_COIN = 521,   /**< Not enough coins */
    RESP_ARMOR_SLOT_FULL = 522    /**< Armor slots full (max 2) */
} ResponseCode;

/**
 * @brief Structure mapping response code to human-readable message
 */
typedef struct {
    ResponseCode code;
    const char *message;
} ResponseMessage;

/**
 * @brief Array of response code to message mappings
 * 
 * This array provides all possible response codes and their 
 * corresponding user-friendly messages.
 */
static const ResponseMessage RESPONSE_MESSAGES[] = {
    /* Success codes */
    {RESP_REGISTER_OK,       "You have registered successfully."},
    {RESP_LOGIN_OK,          "You have logged in successfully."},
    {RESP_LOGOUT_OK,         "You have logged out successfully."},
    {RESP_WELCOME,           "Welcome! Connected to server."},
    {RESP_WHOAMI_OK,         "Current user identified."},
    {RESP_COIN_OK,           "Your coin balance retrieved successfully."},
    {RESP_ARMOR_INFO_OK,     "Your armor information retrieved successfully."},
    {RESP_BUY_ITEM_OK,       "Item purchased successfully."},
    {RESP_START_MATCH_OK,    "Match started successfully."},
    {RESP_MATCH_RESULT_OK,   "Match result retrieved successfully."},
    
    /* Authentication errors */
    {RESP_NOT_CREATOR,       "You are not the team creator."},
    {RESP_SYNTAX_ERROR,      "SYNTAX_ERROR invalid_command_format"},
    {RESP_ACCOUNT_LOCKED,    "This account is locked."},
    {RESP_ACCOUNT_NOT_FOUND, "Account does not exist."},
    {RESP_ALREADY_LOGGED,    "This session is already logged in."},
    {RESP_WRONG_PASSWORD,    "Incorrect password."},
    {RESP_NOT_LOGGED,        "You are not logged in."},
    {RESP_USERNAME_EXISTS,   "Username already exists."},
    {RESP_BUY_ITEM_FAILED,   "Item purchase failed."},
    {RESP_INVALID_USERNAME,  "Invalid username: length 3 to 20, only alphanumeric allowed."},
    {RESP_WEAK_PASSWORD,     "Weak password: minimum 8 characters required, must include uppercase, number, special character."},
    
    /* Match/Team errors */
    {RESP_TEAM_NOT_FOUND,    "Team not found."},
    {RESP_OPPONENT_NOT_FOUND, "Opponent team not found."},
    {RESP_TEAM_IN_MATCH,     "Team is already in a match."},
    {RESP_MATCH_CREATE_FAILED, "Failed to create match."},
    {RESP_MATCH_NOT_FOUND,   "Match not found."},
    {RESP_NOT_AUTHORIZED,    "You are not authorized to access this match."},
    {RESP_MATCH_RUNNING,     "Match is still running."},
    
    /* Server errors */
    {RESP_INTERNAL_ERROR,    "INTERNAL_ERROR"},
    {RESP_DATABASE_ERROR,    "DATABASE_ERROR"},
    {RESP_SERVER_BUSY,       "SERVER_BUSY too_many_connections"},
    {RESP_NOT_IN_MATCH,      "You are not currently in a match."},
    {RESP_MATCH_STATE_ERROR, "Match state does not allow this action."},
    
    /* Shop errors */
    {RESP_ARMOR_NOT_FOUND,   "Armor type does not exist."},
    {RESP_NOT_ENOUGH_COIN,   "Not enough coins to complete the purchase."},
    {RESP_ARMOR_SLOT_FULL,   "Armor slots full (max 2)."}
};

#define RESPONSE_MESSAGES_COUNT (sizeof(RESPONSE_MESSAGES) / sizeof(RESPONSE_MESSAGES[0]))

/**
 * @brief Get a human-readable message for a given response code
 * 
 * @param code The response code to look up
 * @return A pointer to the message string, or NULL if code not found
 */
const char *get_response_message(ResponseCode code);

#endif /* CONFIG_H */

