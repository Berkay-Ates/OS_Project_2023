#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define PORT 8080

// Thread için kullanılan veri yapısı
struct ThreadData {
    int client_fd;
};

// Thread işlevi - Serverdan gelen istekleri dinler
void *listen_server(void *data) {
    struct ThreadData *thread_data = (struct ThreadData *)data;
    int client_fd = thread_data->client_fd;
    char buffer[1024] = {0};

    while (1) {
        // Serverdan gelen veriyi oku
        ssize_t valread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            perror("Error reading from server");
            break;
        }

        // Okunan veriyi ekrana yazdır
        printf("Server response: %s\n", buffer);

        // Bufferı temizle
        memset(buffer, 0, sizeof(buffer));
    }

    // Threadi sonlandır
    pthread_exit(NULL);
}

int main(int argc, char const *argv[]) {
    int status, client_fd;
    struct sockaddr_in serv_addr;

    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        return -1;
    }

    if ((status = connect(client_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) {
        perror("Connection Failed");
        return -1;
    }

    // Thread için veri yapısını oluştur
    struct ThreadData thread_data;
    thread_data.client_fd = client_fd;

    // Thread oluştur
    pthread_t thread;
    if (pthread_create(&thread, NULL, listen_server, (void *)&thread_data) != 0) {
        perror("Could not create thread");
        return -1;
    }

    // Ana thread, kullanıcının istediği komutları işler
    char message[1024];
    while (1) {
        printf("Enter a message for the server (type 'exit' to close the connection): ");
        fgets(message, sizeof(message), stdin);

        if (strncmp(message, "exit", 4) == 0) {
            // Eğer kullanıcı "exit" yazarsa döngüden çık
            break;
        }

        // İstemciye veriyi gönder
        send(client_fd, message, strlen(message), 0);
        printf("Message sent to server\n");
    }

    // Ana threadin işini bitirince, dinleme threadini sonlandır
    pthread_cancel(thread);

    // Bağlantıyı kapat
    close(client_fd);

    return 0;
}
