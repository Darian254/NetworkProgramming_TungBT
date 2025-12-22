# Implementation Guide: Server Merge with TODOs

## Summary
You now have a **modular, epoll-based TCP server architecture** ready to implement. All files are created with detailed TODO comments marking what needs to be done.

## File Structure

### ✅ Created (Ready to Use)
1. **command.c/h** - Command parsing (COMPLETE - copied from phu)
2. **app_context.c/h** - Global state management (COMPLETE - ready to use)
3. **router.c/h** - Command routing (COMPLETE - wired to all existing handlers)
4. **server_new.c** - New epoll-based server (COMPLETE - clean entry point)
5. **server.h** - Server API (COMPLETE)

### 🔧 Modified
1. **connect.c** - Added greeting on connection (line 72)
2. **Makefile** - Updated with new objects

### 📦 Unchanged (Existing Business Logic)
1. **session.c/h** - All handlers intact (LOGIN, REGISTER, BYE, WHOAMI, BUYARMOR)
2. **users.c/h** - User hash table intact
3. **users_io.c/h** - File persistence intact
4. **db.c/db_schema.h** - Game logic intact
5. **epoll_loop.c/h** - Already exists
6. **connect.c/h** - Core logic already exists

---

## Build & Test Steps

### Step 1: Build New Server
```bash
cd ducanh
make clean
make server
```

**Expected Output:**
```
gcc -Wall -Wextra -std=c11 -O2 -c TCP_Server/server_new.c -o TCP_Server/server_new.o
gcc -Wall -Wextra -std=c11 -O2 -c TCP_Server/app_context.c -o TCP_Server/app_context.o
gcc -Wall -Wextra -std=c11 -O2 -c TCP_Server/router.c -o TCP_Server/router.o
gcc -Wall -Wextra -std=c11 -O2 -c TCP_Server/command.c -o TCP_Server/command.o
... (other files)
gcc -Wall -Wextra -std=c11 -O2 -pthread -o server TCP_Server/*.o
```

**If Build Fails:**
- Check that all header includes are correct
- Verify session.h exports: `server_handle_register`, `server_handle_login`, etc.
- Verify db_schema.h exports: `find_ship`, `find_current_match_by_username`, etc.

### Step 2: Run Server
```bash
./server
```

**Expected Output:**
```
[INFO] Loaded users successfully.
[INFO] Session manager initialized.
========================================
Server (Epoll + Non-Blocking IO)
Port: 5500
Max clients: 10000
Max events: 1024
========================================
[INFO] Server listening on port 5500
```

### Step 3: Test with Existing Client
In another terminal:
```bash
cd ducanh
make client
./client 127.0.0.1 5500
```

**Expected Behavior:**
1. Client connects
2. Receives greeting "100"
3. Menu displays
4. Commands work (REGISTER, LOGIN, WHOAMI, etc.)

---

## Architecture Flow

### Connection Flow
```
1. Client connects
2. epoll_loop.c detects EPOLLIN on listen_sock
3. Calls connection_create(new_sock)
4. connect.c:
   - Allocates connection_t
   - Sets non-blocking
   - Sends greeting "100\r\n"
5. Client receives greeting
```

### Command Flow
```
1. Client sends: "REGISTER alice pass123\r\n"
2. epoll detects EPOLLIN on client socket
3. connect.c:connection_on_read()
   - Buffers incoming data
   - Finds complete line
   - Calls command_routes(sock, line)
4. router.c:command_routes()
   - Parses command (type="REGISTER", args="alice pass123")
   - Finds session by socket
   - Locks mutex
   - Calls server_handle_register(...)
   - Unlocks mutex
   - Formats response
   - Calls connection_send()
5. connect.c queues response
6. epoll detects EPOLLOUT
7. connect.c sends buffered response
8. Client receives: "100\r\n"
```

---

## TODO Checklist

### Phase 1: Verify Build ✓
- [x] All files compile without errors
- [x] Server binary created

### Phase 2: Verify Startup ✓
- [x] Server starts without crashes
- [x] Listens on configured PORT
- [x] User table loads from file

### Phase 3: Verify Protocol
- [ ] Client connects and receives greeting
- [ ] REGISTER command works
- [ ] LOGIN command works
- [ ] WHOAMI command works
- [ ] BYE/LOGOUT command works

### Phase 4: Verify Game Logic
- [ ] GETCOIN returns user balance
- [ ] GETARMOR works (requires match)
- [ ] BUYARMOR works (requires match + coins)

### Phase 5: Load Testing
- [ ] Multiple concurrent clients work
- [ ] No memory leaks (valgrind)
- [ ] No race conditions
- [ ] Graceful shutdown

---

## Troubleshooting

### Build Issues

**Problem: `undefined reference to server_handle_register`**
- **Solution:** Add `-pthread` to LDFLAGS in Makefile
- **Why:** session.c uses pthread_mutex

**Problem: `session.h: No such file or directory`**
- **Solution:** Check that session.h is in TCP_Server/
- **Why:** router.c needs session handler prototypes

**Problem: `router.c: conflicting types for find_session_by_socket`**
- **Solution:** Check session.h exports this function
- **Why:** Router needs to map socket → session

### Runtime Issues

**Problem: Server exits immediately**
- **Check:** Port not already in use (`netstat -an | grep 5500`)
- **Check:** Permissions (need sudo for port < 1024)
- **Check:** users.txt file exists in TCP_Server/

**Problem: Client connection hangs**
- **Check:** Greeting is sent in connect.c (line 72)
- **Check:** Firewall not blocking
- **Debug:** Add printf in connection_create()

**Problem: Commands not working**
- **Check:** router.c is receiving commands (add printf)
- **Check:** Response is being sent (add printf before connection_send)
- **Debug:** Capture with tcpdump

---

## Next Steps After Merge

### 1. Add Team Management
Create new handlers in router.c for:
- CREATE_TEAM
- DELETE_TEAM
- JOIN_REQUEST
- TEAM_INVITE
- KICK_MEMBER

**Pattern:**
```c
else if (strcmp(type, "CREATE_TEAM") == 0) {
    char team_name[32];
    if (sscanf(payload, "%31s", team_name) != 1) {
        // error
    } else {
        // Call team handler from db.c
        Team *team = create_team(team_name, session->username);
        // Format response
    }
}
```

### 2. Add Challenge System
Implement challenge workflow:
- SEND_CHALLENGE
- ACCEPT_CHALLENGE
- DECLINE_CHALLENGE
- START_MATCH

### 3. Add Match Logic
Implement real-time match handling:
- ATTACK
- DEFEND
- USE_ITEM
- SURRENDER

### 4. Add Persistence
Save game state periodically:
- Team data to teams.txt
- Match results to matches.txt
- Ship states (or clear on match end)

---

## Performance Tuning

### 1. Buffer Sizes
Adjust in config.h:
```c
#define BUFF_SIZE 4096  // Increase if long messages
#define MAX_CLIENTS 10000  // Increase for more users
```

### 2. Epoll Flags
Current: EPOLLET (edge-triggered)
- Pro: More efficient, fewer syscalls
- Con: Must drain buffer completely

Alternative: EPOLLIN (level-triggered)
- Pro: Simpler, can partial-read
- Con: More wakeups

### 3. Thread Safety
Current: Single mutex for all user operations
- Pro: Simple, no deadlocks
- Con: Contention under high load

Future: Fine-grained locking
- Per-user mutexes
- Read-write locks
- Lock-free data structures

---

## Migration Path

### Current State
- Old `server.c` with select/workers (partially implemented)
- Business logic in `session.c` (working)
- Network layer mixed with business logic

### After This Merge
- Clean `server_new.c` with epoll (fully implemented)
- Business logic still in `session.c` (untouched)
- Network layer separated via `router.c`

### Benefits
1. **Simpler:** No worker threads, no pipes, no complex synchronization
2. **Faster:** Edge-triggered epoll scales to 10K+ connections
3. **Modular:** Clear separation of concerns
4. **Testable:** Can unit-test router independently
5. **Maintainable:** Each file has single responsibility

---

## Success Criteria

✅ **Merge Complete When:**
1. Server builds without warnings
2. Server starts and listens
3. Client connects and operates normally
4. All existing commands work (REGISTER/LOGIN/etc.)
5. No memory leaks
6. No crashes under normal load

🎯 **Ready for Production When:**
1. Load tested with 100+ concurrent clients
2. All game features implemented
3. Proper error handling everywhere
4. Logging added
5. Configuration externalized
6. Documentation updated

---

## Contact & Support

If you encounter issues:
1. Check TODO comments in each file
2. Review MERGE_GUIDE.md
3. Test incrementally (don't skip phases)
4. Add printf debugging at each layer
5. Use valgrind for memory issues
6. Use gdb for crashes

**Remember:** Your existing business logic is safe in session.c, users.c, and db.c. Only the network layer changed!
