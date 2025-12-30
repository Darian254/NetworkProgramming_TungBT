# Server Merge Guide: Epoll + Non-Blocking IO Architecture

## Overview
This guide explains how to merge the `phu` epoll-based architecture into the `ducanh` TCP server while preserving existing game logic.

## Current Architecture Analysis

### ducanh/TCP_Server (Current)
- **Architecture**: Hybrid select-based with worker threads (partially implemented)
- **Components**:
  - `server.c`: Main entry with select loops (commented out workers)
  - `session.c/h`: Session management with `ServerSession`, handlers (login, register, etc.)
  - `users.c/h`: User hash table with persistence
  - `db_schema.h/db.c`: Game data (teams, matches, ships, armor)
  - `file_transfer.c/h`: send_line/recv_line helpers
  - `epoll_loop.c`: Exists but not integrated
  - `connect.c/h`: Exists but references missing `router.h`

### phu (Target Architecture)
- **Architecture**: Single-threaded epoll + edge-triggered non-blocking IO
- **Components**:
  - `main.c`: Simple entry point
  - `server.c/h`: Initialization and lifecycle (server_init, server_run, server_shutdown)
  - `epoll_loop.c`: Event loop handling EPOLLIN/EPOLLOUT
  - `connect.c/h`: Connection management with buffered I/O
  - `router.c/h`: Command routing (currently stubs)
  - `command.c/h`: Command parsing
  - `models.h`: Simple session/user structs

## Merge Strategy

### Phase 1: Infrastructure (Router + Command Layer)
Create routing layer to connect epoll events to existing session handlers.

**Files to Create:**
1. `TCP_Server/router.c` - Routes commands to session handlers
2. `TCP_Server/router.h` - Router interface
3. `TCP_Server/command.c` - Command parsing (from phu)
4. `TCP_Server/command.h` - Command structures

### Phase 2: Server Initialization
Replace current `main()` in `server.c` with clean epoll bootstrap.

**Files to Modify:**
1. `TCP_Server/server.c` - Add server_init/server_run/server_shutdown
2. `TCP_Server/server.h` - Add header with public API

### Phase 3: Integration
Wire everything together and ensure greeting/protocol compatibility.

**Files to Modify:**
1. `TCP_Server/connect.c` - Send initial greeting on connection
2. `Makefile` - Update object list

### Phase 4: Global State Management
Add global user table and session manager initialization.

**Files to Create:**
1. `TCP_Server/app_context.c` - Global state (user_table, mutex)
2. `TCP_Server/app_context.h` - Global state interface

## Architecture Benefits

### Before (Current)
```
Client → socket → select/worker threads → inline command parsing → session handlers
```

### After (Target)
```
Client → socket → epoll → connect.c (buffering) → router.c → session handlers → users/db
```

### Advantages:
- **Single-threaded**: No thread synchronization overhead, simpler debugging
- **Scalable**: Edge-triggered epoll handles 10K+ connections efficiently
- **Modular**: Clear separation (network layer, routing, business logic)
- **Maintainable**: Each component has single responsibility

## File Organization

```
TCP_Server/
├── Network Layer (epoll-based)
│   ├── server.c/h          - Lifecycle management
│   ├── epoll_loop.c/h      - Event loop
│   └── connect.c/h         - Connection & buffering
│
├── Routing Layer
│   ├── router.c/h          - Command dispatch
│   └── command.c/h         - Command parsing
│
├── Business Logic
│   ├── session.c/h         - Session handlers (login, register, etc.)
│   ├── users.c/h           - User hash table
│   ├── users_io.c/h        - Persistence
│   ├── db.c/db_schema.h    - Game data
│   └── hash.c/h            - Hashing utilities
│
├── Utilities
│   ├── file_transfer.c/h   - send_line/recv_line
│   ├── util.c/h            - Helpers
│   └── config.c/h          - Configuration
│
└── Global State
    └── app_context.c/h     - Shared resources
```

## Next Steps

1. Review the TODO comments in the files I'll create
2. Build incrementally: `make server` after each phase
3. Test each phase before moving to next
4. Existing client should work without changes

## Testing Strategy

1. **Phase 1 Test**: Build with new router (stubs should compile)
2. **Phase 2 Test**: Server starts and listens on PORT
3. **Phase 3 Test**: Client connects and receives greeting
4. **Phase 4 Test**: Full command flow (REGISTER, LOGIN, WHOAMI, etc.)

## Rollback Plan

If issues arise, your current `server.c` logic is preserved in:
- `session.c` - All handlers intact
- `users.c` - Hash table logic intact
- `db.c` - Game logic intact

Only network layer changes, business logic untouched.
