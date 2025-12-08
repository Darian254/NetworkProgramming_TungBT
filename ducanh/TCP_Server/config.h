#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

/**
 * @enum FunctionId
 * @brief IDs for user menu actions
 */
typedef enum {
    FUNC_REGISTER = 0,  /**< Register new account */
    FUNC_LOGIN  = 1,  /**< User login */
    FUNC_LOGOUT = 2,  /**< Logout */
    FUNC_WHOAMI = 3,  /**< Check current user */
    FUNC_EXIT   = 4   /**< Exit program */
} FunctionId;

/**
 * @brief Response codes for the TCP server protocol
 */
typedef enum {
    /* Success codes */
    RESP_REGISTER_OK = 100,       /**< Registration successful */
    RESP_LOGIN_OK = 110,          /**< Login successful */
    RESP_LOGOUT_OK = 134,         /**< Logout successful */
    RESP_WHOAMI_OK = 201,         /**< Whoami successful */
    
    /* Client error codes */
    RESP_SYNTAX_ERROR = 301,      /**< Syntax error */
    RESP_ACCOUNT_LOCKED = 311,    /**< Account is locked */
    RESP_ACCOUNT_NOT_FOUND = 312, /**< Account does not exist */
    RESP_ALREADY_LOGGED = 313,    /**< Already logged in */
    RESP_WRONG_PASSWORD = 314,    /**< Incorrect password */
    RESP_NOT_LOGGED = 315,        /**< Not logged in */
    RESP_USERNAME_EXISTS = 331,   /**< Username already exists */
    RESP_INVALID_USERNAME = 402,  /**< Invalid username format */
    RESP_WEAK_PASSWORD = 403,     /**< Weak password */
    
    /* Server error codes */
    RESP_INTERNAL_ERROR = 500,    /**< Internal server error */
    RESP_SERVER_BUSY = 503        /**< Server too busy */
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
    {RESP_REGISTER_OK,       "You have registered successfully."},
    {RESP_LOGIN_OK,          "You have logged in successfully."},
    {RESP_LOGOUT_OK,         "You have logged out successfully."},
    {RESP_WHOAMI_OK,         "Current user identified."},
    {RESP_SYNTAX_ERROR,      "SYNTAX_ERROR invalid_command_format"},
    {RESP_ACCOUNT_LOCKED,    "This account is locked."},
    {RESP_ACCOUNT_NOT_FOUND, "Account does not exist."},
    {RESP_ALREADY_LOGGED,    "This session is already logged in."},
    {RESP_WRONG_PASSWORD,    "Incorrect password."},
    {RESP_NOT_LOGGED,        "Not logged in."},
    {RESP_USERNAME_EXISTS,   "Username already exists."},
    {RESP_INVALID_USERNAME,  "Invalid username: length 3 to 20, only alphanumeric allowed."},
    {RESP_WEAK_PASSWORD,     "Weak password: minimum 8 characters required, must include uppercase, number, special character."},
    {RESP_INTERNAL_ERROR,    "INTERNAL_SERVER_ERROR unexpected_condition_occurred"},
    {RESP_SERVER_BUSY,       "SERVER_BUSY too_many_connections" }
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

