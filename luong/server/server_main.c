#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "game_logic.h"
#include "ship_defs.h"

#define SERVER_PORT 8080

extern Ship active_ships[];
extern int num_active_ships;

void* client_handler(void* socket_desc) {
     int sock = *(int*)socket_desc;
     free(socket_desc);
     char buffer[1024];
     int read_size;

     //Lognin
     Ship* my_ship = NULL;
     for(int i=0; i < num_active_ships; i++) {
        if(active_ships[i].socket_fd == -1) {
            active_ships[i].socket_fd = sock;//đánh dấu tàu thuộc client
            my_ship = &active_ships[i];
            break;
        }
     }
    if (my_ship) {
        printf("[CONNECTION] Client %d đã kết nối. Điều khiển Tàu %d (Team %d)\n", sock, my_ship->ship_id, my_ship->team_id);
        
        char msg[100];
        sprintf(msg, "LOGIN_OK You are Ship %d\n", my_ship->ship_id);
        send(sock, msg, strlen(msg), 0);
    } else {
        printf("[REJECT] Phòng đã đầy! Đóng kết nối %d.\n", sock);
        close(sock);
        return NULL;
    }

    while ((read_size = recv(sock, buffer, 1024, 0)) >0) {
        buffer[read_size] = '\0'; // Kết thúc chuỗi nhận được
        
        int target_id, slot;
        char command[10];

        int args = sscanf(buffer, "%s %d %d", command,&target_id, &slot);
        if (args >= 3 && strcmp(command, "FIRE") == 0) {
            printf("[LOG] Tàu %d bắn Tàu %d (Slot %d)\n", my_ship->ship_id, target_id, slot);
            process_fire_request(my_ship->ship_id, target_id, slot);
        } else {
            char* err = "400 Syntax Error\n";
            send(sock, err, strlen(err), 0);
        }
    }
        if(read_size == 0) {
            printf("Client Tàu %d đã ngắt kết nối .\n", my_ship -> ship_id);
            my_ship -> socket_fd = -1;
        } else if(read_size == -1) {
            perror("recv failed");
        }
        close(sock);
        return NULL;
    
}

int main() {
    int server_fd, client_sock, *new_sock;
    struct sockaddr_in server, client;

    load_all_ships();

    //Tạo socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if( server_fd == -1) {
        printf("Không thể tạo socket");
        return 1;
    }
    memset(&server, 0, sizeof(server));

    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    server.sin_addr.s_addr = htonl(INADDR_ANY);


    //Bind
    if(bind(server_fd, (struct sockaddr *)&server, sizeof(server)) <0) {
        perror("Bind failed");
        return 1;
    }
    listen(server_fd, 3);//Cho phép 3 kết nối chờ
    printf("SERVER GAME ĐANG CHẠY TẠI PORT %d \n", SERVER_PORT);
    
    int c = sizeof(struct sockaddr_in);
    // Kết nối liên tục 
    while( (client_sock = accept(server_fd, (struct sockaddr *)&client, (socklen_t*)&c)) ) {
        pthread_t client_thread//nghe dữ lieeuk từ client socket
        new_sock = malloc(sizeof(int));
        *new_sock = client_sock;
        
        if(pthread_create(&client_thread, NULL, client_handler, (void*)new_sock) < 0) {
            perror("Could not create thread");
            return 1;
        }
        // Detach thread để tự giải phóng tài nguyên khi xong
        pthread_detach(client_thread);
    }

    return 0;
}