#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>  // Eklenen başlık dosyası
#include <sys/time.h>

int generate_unique_id() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)tv.tv_sec;
}

int main(){

    printf("%d",generate_unique_id());

}