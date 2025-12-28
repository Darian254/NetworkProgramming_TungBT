#ifndef CONFIG_H
#define CONFIG_H

#include <stddef.h>

/**
 * @enum FunctionId
 * @brief IDs for user menu actions
 */
typedef enum {
    FUNC_REGISTER = 0,      /**< Register new account */
    FUNC_LOGIN  = 1,        /**< User login */
    FUNC_LOGOUT = 2,        /**< Logout */
    FUNC_WHOAMI = 3,        /**< Check current user */
    FUNC_EXIT   = 4,        /**< Exit program */
    FUNC_CHECK_COIN = 5,    /**< Check my coin balance */
    FUNC_CHECK_ARMOR = 6,   /**< Check my ship armor */
    FUNC_BUY_ARMOR = 7,     /**< Buy armor for my ship */
    /* Team functions */
    FUNC_CREATE_TEAM = 8,   /**< Create new team */
    FUNC_DELETE_TEAM = 9,   /**< Delete team */
    FUNC_LIST_TEAMS = 10,   /**< List all teams */
    FUNC_JOIN_REQUEST = 11, /**< Request to join team */
    FUNC_LEAVE_TEAM = 12,    /**< Leave current team */
    FUNC_TEAM_MEMBERS = 13,     /**< List team members */
    FUNC_KICK_MEMBER = 14,      /**< Kick member */
    FUNC_APPROVE_JOIN = 15,     /**< Approve join request */
    FUNC_REJECT_JOIN = 16,      /**< Reject join request */
    FUNC_INVITE_MEMBER = 17,    /**< Invite member */
    FUNC_ACCEPT_INVITE = 18,    /**< Accept invitation */
    FUNC_REJECT_INVITE = 19     /**< Reject invitation */
} FunctionId;

/**
 * @brief Response codes for the TCP server protocol
 */
typedef enum {
    /* Success codes */
    RESP_REGISTER_OK = 100,       /**< Registration successful */
    RESP_LOGIN_OK = 110,          /**< Login successful */
    RESP_JOIN_APPROVED = 111,     /**< Join request approved */
    RESP_JOIN_REJECTED = 112,     /**< Join request rejected */
    RESP_LIST_TEAMS_OK = 204,    /**< List teams successful */
    RESP_TEAM_MEMBERS_LIST_OK = 205,   /**< Team members list successful */
    RESP_TEAM_CREATED = 120,      /**< Team created successfully */
    RESP_JOIN_REQUEST_SENT = 121, /**< Join request sent */
    RESP_TEAM_LEAVE_OK = 123,     /**< Left team successfully */
    RESP_TEAM_INVITED = 124,      /**< Invitation sent */
    RESP_JOIN_REQUEST_RECEIVED = 125, /**< Join request received (notification) */
    RESP_TEAM_INVITE_RECEIVED = 130,  /**< Team invite received  */
    RESP_TEAM_INVITE_ACCEPTED = 126,  /**< Invitation accepted */
    RESP_TEAM_INVITE_REJECTED = 127,  /**< Invitation rejected */
    RESP_TEAM_DELETED = 128,      /**< Team deleted */
    RESP_KICK_MEMBER_OK = 129,    /**< Member kicked successfully */
    RESP_LOGOUT_OK = 134,         /**< Logout successful */
    RESP_WHOAMI_OK = 201,         /**< Whoami successful */
    RESP_COIN_OK = 202,           /**< Get coin successful */
    RESP_ARMOR_INFO_OK = 203,     /**< Get armor info successful */
    RESP_BUY_ITEM_OK = 334,       /**< Buy item successful */
    
    /* Client error codes - Command/Syntax */
    RESP_BAD_COMMAND = 300,       /**< Unknown/invalid command */
    RESP_SYNTAX_ERROR = 301,      /**< Syntax error */
    RESP_PLAYER_NOT_FOUND = 302,  /**< Player does not exist */
    RESP_ALREADY_IN_TEAM = 303,   /**< Already in a team */
    
    /* Client error codes - Authentication */
    RESP_ACCOUNT_LOCKED = 311,    /**< Account is locked */
    RESP_ACCOUNT_NOT_FOUND = 312, /**< Account does not exist */
    RESP_ALREADY_LOGGED = 313,    /**< Already logged in */
    RESP_WRONG_PASSWORD = 314,    /**< Incorrect password */
    RESP_NOT_LOGGED = 315,        /**< Not logged in */
    
    /* Client error codes - Team Operations */
    RESP_TEAM_NOT_FOUND = 323,    /**< Team does not exist */
    RESP_NOT_CREATOR = 326,       /**< Not the team creator */
    RESP_NOT_IN_TEAM = 327,       /**< Not in any team */
    RESP_TEAM_FULL = 328,         /**< Team is full (max members reached) */
    RESP_INVITE_QUEUE_FULL = 329, /**< Invite queue is full */
    RESP_MEMBER_KICK_NOT_IN_TEAM = 330, /**< Member to kick not found */
    RESP_INVITE_NOT_FOUND = 333,   /**< Invitation not found */
    
    /* Client error codes - Registration */
    RESP_USERNAME_EXISTS = 331,   /**< Username already exists */
    RESP_TEAM_CREATE_FAILED = 332,/**< Team creation failed */
    RESP_BUY_ITEM_FAILED = 335,   /**< Buy item failed */
    RESP_INVALID_USERNAME = 402,  /**< Invalid username format */
    RESP_WEAK_PASSWORD = 403,     /**< Weak password */
    
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
 */
static const ResponseMessage RESPONSE_MESSAGES[] = {
    /* Success codes */
    {RESP_REGISTER_OK,       "You have registered successfully."},
    {RESP_LOGIN_OK,          "You have logged in successfully."},
    {RESP_JOIN_APPROVED,     "Join request has been approved."},
    {RESP_JOIN_REJECTED,     "Join request has been rejected."},
    {RESP_TEAM_CREATED,      "Team created successfully."},
    {RESP_JOIN_REQUEST_SENT, "Join request sent to team."},
    {RESP_TEAM_LEAVE_OK,     "You have left the team."},
    {RESP_TEAM_INVITED,      "Player invited to team."},
    {RESP_JOIN_REQUEST_RECEIVED, "Join request received."},
    {RESP_TEAM_INVITE_ACCEPTED,  "Team invitation accepted."},
    {RESP_TEAM_INVITE_REJECTED,  "Team invitation rejected."},
    {RESP_INVITE_NOT_FOUND, "No invitation found from this team."},
    {RESP_TEAM_DELETED,      "Team has been deleted."},
    {RESP_KICK_MEMBER_OK,    "Member kicked from team."},
    {RESP_LOGOUT_OK,         "You have logged out successfully."},
    {RESP_WHOAMI_OK,         "Current user identified."},
    {RESP_COIN_OK,           "Your coin balance retrieved successfully."},
    {RESP_ARMOR_INFO_OK,     "Your armor information retrieved successfully."},
    {RESP_BUY_ITEM_OK,       "Item purchased successfully."},
    
    /* Command/Syntax errors */
    {RESP_BAD_COMMAND,       "Unknown or invalid command."},
    {RESP_SYNTAX_ERROR,      "Syntax error: invalid command format."},
    {RESP_PLAYER_NOT_FOUND,  "Player does not exist."},
    {RESP_ALREADY_IN_TEAM,   "Already in a team."},
    
    /* Authentication errors */
    {RESP_ACCOUNT_LOCKED,    "This account is locked."},
    {RESP_ACCOUNT_NOT_FOUND, "Account does not exist."},
    {RESP_ALREADY_LOGGED,    "This session is already logged in."},
    {RESP_WRONG_PASSWORD,    "Incorrect password."},
    {RESP_NOT_LOGGED,        "You are not logged in."},
    
    /* Team operation errors */
    {RESP_TEAM_NOT_FOUND,    "Team does not exist."},
    {RESP_NOT_CREATOR,       "Only the team creator can perform this action."},
    {RESP_NOT_IN_TEAM,       "You are not in any team."},
    {RESP_TEAM_FULL,         "Team is full (maximum members reached)."},
    {RESP_INVITE_QUEUE_FULL, "Invite queue is full."},
    {RESP_MEMBER_KICK_NOT_IN_TEAM, "The member to kick is not in the team."},
    
    /* Registration errors */
    {RESP_USERNAME_EXISTS,   "Username already exists."},
    {RESP_TEAM_CREATE_FAILED,"Team creation failed (name may already exist)."},
    {RESP_BUY_ITEM_FAILED,   "Item purchase failed."},
    {RESP_INVALID_USERNAME,  "Invalid username: length 3 to 20, only alphanumeric allowed."},
    {RESP_WEAK_PASSWORD,     "Weak password: minimum 8 characters required, must include uppercase, number, special character."},
    
    /* Server errors */
    {RESP_INTERNAL_ERROR,    "Internal server error."},
    {RESP_DATABASE_ERROR,    "Database error."},
    {RESP_SERVER_BUSY,       "Server is too busy, please try again later."},
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