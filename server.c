#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>  // Eklenen başlık dosyası
#include <sys/time.h>

#define PORT 8080

//* git token is below
//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

typedef struct user{
    char phone[30];
    char name[30];
    char surname[30];
    char id[11];
    struct user* next;
}USER;


typedef struct files{
    FILE* file;
    char fileName[30];
    struct files* next;
}FILES;


typedef struct threadArgs{
    FILES* filesHead ;
    USER* userHead ; 
    int socket;
}THREADARGS;

int generate_unique_id();
int logIn_signIn_user(void *args);
void *handle_client(void *args);
void *setAllUsers(USER* userHead);
USER* mallocUser(char* id, char* name, char* surname);
void sendUserList(void* args);

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    USER* userHead = (USER*) malloc(sizeof(USER)); 
    FILES* filesHead = (FILES*) malloc(sizeof(FILES));
    THREADARGS* threadArgs = (THREADARGS*) malloc(sizeof(THREADARGS));

    threadArgs->filesHead = filesHead;
    threadArgs->userHead = userHead;

    userHead->next = NULL;

    FILE* apps = fopen("./apps/allUsers","a+"); 
        
    if (apps == NULL) {
        perror("file error");
        return 1; // İşlemi sonlandır
    }

    filesHead->file = apps;
    strcpy(filesHead->fileName,"./app/allUsers");
    filesHead->next = NULL;


    // Soket dosya tanımlayıcısı oluştur
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Soketi 8080 portuna zorla bağla gerekirse oraya civile.
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Soketi belirtilen adres ve portla ilişkilendir
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Gelen bağlantıları dinle
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    while (1) {
        // Yeni bir bağlantı kabul et
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // Her bağlantı için bir thread oluştur
        pthread_t thread;
        threadArgs->socket = new_socket;
        
        printf("dosya  :%p\n",apps);

        if (pthread_create(&thread, NULL, handle_client, (void *)threadArgs) < 0) {
            perror("could not create thread on server side");
            exit(1);
        }

        // Detached thread oluştur
        pthread_detach(thread);
    }

    while(filesHead != NULL){
        fclose(filesHead->file);
        filesHead = filesHead->next;
    }

    // Dinleme soketini kapat
    close(server_fd);
    return 0;
}



void *handle_client(void *args){

    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    FILES* FileHead = threadArgs->filesHead;
    USER* userHead = threadArgs->userHead;
    
    ssize_t valread;
    char buffer[1024] = {0};
    int isExit = 1;

    if(logIn_signIn_user(args) < 0 ){
        // Soketi kapat
        send(client_socket, "/close", strlen("/close"), 0);
        close(client_socket);
        pthread_exit(NULL);
        exit(-1);
    }

    send(client_socket, "/succeed", strlen("/succeed"), 0);

    //* head de user var ve tum kullanicilar eklenmis vaziyette.
    setAllUsers(threadArgs->userHead);
    while(userHead != NULL){
        printf("%s\n",userHead->id);
        userHead = userHead->next;
    }


    while (isExit) {
        memset(buffer, 0, sizeof(buffer));
        // kullaniciyi login yada register yap
       
        valread = read(client_socket, buffer, 1024 - 1);
        
        if (valread <= 0) {
            printf("There was an error in server side while reading user command");
            exit(-1);
        }
        
        if(strncmp(buffer,"/listAllUsers",strlen("/listAllUsers")) == 0){
            printf("from server /listAllUsers\n");
            sendUserList(args);

        } if(strncmp(buffer,"/listMyContacts",strlen("/listMyContacts")) == 0){
            printf("from server /listMyContacts\n");

        }else if(strncmp(buffer,"/addNewUser",strlen("/addNewUser")) == 0){
            printf("from server /addNewUser\n");
            printf("Buffer: %s\n\n", buffer);
        
        }else if(strncmp(buffer,"/deleteUser",strlen("/deleteUser")) == 0){
            printf("from server /deleteUser\n");
            printf("Buffer: %s\n\n", buffer);

        }else if(strncmp(buffer,"/sendMessage",strlen("/sendMessage")) == 0){
            printf("from server /sendMessage\n");
            printf("Buffer: %s\n\n", buffer);

        }else if(strncmp(buffer,"/checkMessages",strlen("/checkMessages")) == 0){
            printf("from server /checkMessages\n");
            printf("Buffer: %s\n\n", buffer);
        
        }else if(strncmp(buffer,"/logOutUser",strlen("/logOutUser")) == 0){
            printf("from server /logOutUser\n");
            printf("Buffer: %s\n\n", buffer);
            isExit = 0;

        }   

        //İstemciye "hello" mesajını gönder
        //send(client_socket, "buffer", strlen(buffer), 0);
    }

    // Soketi kapat
    close(client_socket);

    pthread_exit(NULL);
}

void sendUserList(void* args){

    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    FILES* tmp = threadArgs->filesHead;
    USER* userHead = threadArgs->userHead;
    char buffer[90];
    
    while(userHead != NULL){
        send(client_socket, "/userStruct", strlen("/userStruct"), 0);
        read(client_socket, buffer, strlen("/ready"));
        send(client_socket, userHead, sizeof(USER), 0);
        userHead = userHead->next;
    }

}

int logIn_signIn_user(void* args){
    
    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    FILES* fileHeadParam = threadArgs->filesHead;
    USER* userHead = threadArgs->userHead;

    FILE* tmpFiles = fopen("./apps/allUsers","a+");
    

    ssize_t valread;
    char buffer[1024] = {0};
    memset(buffer, 0, sizeof(buffer));

    USER* user = (USER*) malloc(sizeof(USER));
    USER* userTmp = (USER*) malloc(sizeof(USER));

    valread = read(client_socket, buffer, 1024 - 1);
    if (valread <= 0) {
        printf("There was an error in server side while reading user command user did not choose login or signup \n");
        exit(-1);
    }

    if(strncmp(buffer,"/signUpUser",11) == 0){
        valread = recv(client_socket, user, sizeof(USER),0);

        if(valread == sizeof(USER)){
            printf("\nYour id is: %s\n",user->id);
            printf("Your name is: %s\n",user->name);
            printf("Your surname is: %s\n",user->surname);
            printf("Your phone is:%s\n\n",user->phone);
            // useri dosyaya kaydedip sonrasinda return 1 
            
            fwrite(user,sizeof(USER),1,tmpFiles);

            if (ferror(tmpFiles))
                perror("Error writing to file");

            strcpy(userHead->name,user->name);
            strcpy(userHead->id,user->id);
            strcpy(userHead->surname,user->surname);
            strcpy(userHead->phone,user->phone);

            fclose(tmpFiles);
            free(user);
            free(userTmp);
            return 1;

        }else{
            printf("There was error while taking user struct for sign up\n");
            free(user);
            free(userTmp);
            return -1;
        }

    }else if(strncmp(buffer,"/loginUser",10) == 0){
        valread = recv(client_socket, user, sizeof(USER),0);
        if(valread == sizeof(USER)){

            printf("\nYour id is: %s\n",user->id);
            printf("Your name is: %s\n",user->name);
            printf("Your surname is: %s\n\n",user->surname);
            // useri dosyaya kaydedip sonrasinda return 1 

            while(fread(userTmp,sizeof(USER),1,tmpFiles) == 1){
                if(strcmp(user->id,userTmp->id) == 0){
                    printf("%s have been successfully logged in.\n",userTmp->id);

                    strcpy(userHead->name,userTmp->name);
                    strcpy(userHead->id,userTmp->id);
                    strcpy(userHead->surname,userTmp->surname);
                    fclose(tmpFiles);
                    free(user);
                    free(userTmp);
                    return 1;
                }
            }

            if (ferror(tmpFiles))
                perror("Error writing to file");
            
            printf("%s did not found in saved users operation failed\n",user->id);
            printf("User claimed with %s access denied.",userTmp->id);
            fclose(tmpFiles);
            free(user);
            free(userTmp);
            return -1;

        }else{
            printf("There was error while taking user struct for sign up\n");
            free(user);
            free(userTmp);
            return -1;
        }
    }
}

void *setAllUsers(USER* userHead){
    USER* tmp = userHead;
    USER userTmp;
    FILE* tmpFiles = fopen("./apps/allUsers","r");

    while(fread(&userTmp,sizeof(USER),1,tmpFiles) == 1){
        if(strcmp(userTmp.id,userHead->id) != 0){
            tmp->next = mallocUser(userTmp.id,userTmp.name,userTmp.surname);
            tmp = tmp->next;
        }
    }

    fclose(tmpFiles);

}


USER* mallocUser(char* id, char* name, char* surname){
    USER* tmp = (USER*) malloc(sizeof(USER));
    if(tmp==NULL){
        printf(" There was an error while allocating a user\n");
        exit(-1);
    }
    strcpy(tmp->id,id);
    strcpy(tmp->name,name);
    strcpy(tmp->surname,surname);
    tmp->next=NULL;
    
    return tmp; 
}

