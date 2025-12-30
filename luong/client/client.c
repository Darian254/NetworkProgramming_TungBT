#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFF_SIZE 1024

int sock = -1;
int is_running = 1;

// Luồng nhận tin nhắn từ Server 
void* receive_handler(void* arg) {
    char buffer[BUFF_SIZE];
    int n;

    while (is_running) {
        memset(buffer, 0, BUFF_SIZE);
        n = recv(sock, buffer, BUFF_SIZE - 1, 0);
        if (n <= 0) {
            printf("\n[DISCONNECT] Mất kết nối tới Server.\n");
            is_running = 0;
            break;
        }

        int protocol;
        if (sscanf(buffer, "%d", &protocol) == 1) {
            printf("\r\033[K"); // Xóa dòng hiện tại để in thông báo mới sạch sẽ

            switch(protocol) {
                case 130: { // SEND_CHALLENGE_OK
                    int chal_id;
                    sscanf(buffer, "130 CHALLENGE_SENT %d", &chal_id);
                    printf("\n[THÀNH CÔNG] Lời mời đã gửi! ID Lời mời: %d\n", chal_id);
                    break;
                }
                case 131: { // ACCEPT_CHALLENGE_OK
                    int chal_id;
                    sscanf(buffer, "131 CHALLENGE_ACCEPTED %d", &chal_id);
                    printf("\n[THÔNG BÁO] Trận đấu bắt đầu cho lời mời %d!\n", chal_id);
                    break;
                }
                case 132: { // DECLINE_CHALLENGE_OK
                    printf("\n[THÔNG BÁO] Lời mời đã bị từ chối.\n");
                    break;
                }
                case 200: { // FIRE_EVENT
                    int atk, tar, dam, hp, arm;
                    sscanf(buffer, "200 %d %d %d %d %d", &atk, &tar, &dam, &hp, &arm);
                    printf("\n[CHIẾN TRƯỜNG] Tàu %d bắn Tàu %d (Dam: %d)\n", atk, tar, dam);
                    break;
                }
                case 140: {//rương xuất hiện
                    int c_id, c_type, px, py;
                    sscanf(buffer, "140 %d %d %d %d", &c_id, &c_type, &px, &py);

                    // Hiển thị câu hỏi ngay lập tức
                    printf("\n[SỰ KIỆN] RƯƠNG %d XUẤT HIỆN!", c_id);
                    if(c_type == 0) printf("\nCÂU HỎI (ĐỒNG): 1 + 1 = ?");
                    else if(c_type == 1) printf("\nCÂU HỎI (BẠC): Thủ đô Việt Nam?");
                    else printf("\nCÂU HỎI (VÀNG): Giao thức tầng giao vận nào tin cậy?");
        
                    printf("\nNHẬP: CHEST_OPEN %d <đáp án>\n", c_id);
                    break;
                }
                case 127: { // MỞ RƯƠNG THÀNH CÔNG 
                    int cid, reward, total;
                    sscanf(buffer, "127 CHEST_OK %d %d %d", &cid, &reward, &total);
                    printf("\n[CHÚC MỪNG] Bạn đã mở rương %d, nhận %d coin! Tổng: %d\n", cid, reward, total);
                    break;
                }
                case 210: { // THÔNG BÁO RƯƠNG ĐÃ BỊ MỞ 
                    char user[64]; int cid;
                    sscanf(buffer, "210 CHEST_COLLECTED %s %d", user, &cid);
                    printf("\n[THÔNG BÁO] Người chơi %s đã giải đố thành công! Rương %d đã biến mất.\n", user, cid);
                    break;
                }
                case 442: {
                printf("\n[SAI RỒI] Đáp án không chính xác, rương vẫn còn đó!\n");
                break;
    }
                default:
                    printf("\n[SERVER MSG] %s", buffer);
            }
            
            printf("\nNHẬP LỆNH: ");
            fflush(stdout);
        }
    }
    return NULL;
}

int main() {
    struct sockaddr_in serv_addr;//Đchi server
    pthread_t recv_thread;

    // Tạo Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {//str->bin
        perror("Invalid Address");
        return -1;
    }

    // Kết nối
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return -1;
    }

    printf("Kết nối thành công tới Server!\n");
    
    // Nhận tin nhắn từ Server
    if(pthread_create(&recv_thread, NULL, receive_handler, NULL) != 0) {
        perror("Thread Error");
        return -1;
    }

    // Gửi lệnh
    char send_buffer[BUFF_SIZE];
    while (is_running) {
        printf("NHẬP LỆNH (FIRE <TargetID> <Slot>): ");
        memset(send_buffer, 0, BUFF_SIZE);
        fgets(send_buffer, BUFF_SIZE, stdin);//\n
        

        send_buffer[strcspn(send_buffer, "\n")] = 0;/// n=0,find

        if (strcmp(send_buffer, "exit") == 0) {
            is_running = 0;
            break;
        }

        if (strlen(send_buffer) > 0) {//VD:"FIRE 3 2" 8 >0 
            send(sock, send_buffer, strlen(send_buffer), 0);
            usleep(100000); // Nghỉ 0.1s để chờ server phản hồi
        }
    }

    close(sock);
    return 0;
}