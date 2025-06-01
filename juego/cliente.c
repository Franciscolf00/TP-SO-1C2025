#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

char* ip = "127.0.0.1";  // o "localhost"
int PUERTO = 12345;


int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PUERTO);
    inet_pton(AF_INET, ip, &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(1);
    }

    char buffer[256];
    while (1) {
        int r = read(sock, buffer, sizeof(buffer)-1);
        if (r <= 0) break;
        buffer[r] = '\0';
        printf("%s", buffer);

        if (strstr(buffer, "Es tu turno")) {
            // Leer intento de usuario
            fgets(buffer, sizeof(buffer), stdin);
            write(sock, buffer, strlen(buffer));
        }
        if (strstr(buffer, "¿Querés seguir jugando?")) {
            fgets(buffer, sizeof(buffer), stdin);
            write(sock, buffer, strlen(buffer));
            if (buffer[0] == 'n' || buffer[0] == 'N') break;
        }
    }

    close(sock);
    return 0;
}
