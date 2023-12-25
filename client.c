#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>  // Eklenen başlık dosyası
#include <sys/time.h>

#define PORT 8080

typedef struct user{
    char phone[30];
    char name[30];
    char surname[30];
    char id[11];
    struct user* next;
    struct user* contacts;
}USER;

struct ThreadData {
    int client_fd;
    int messaging_now;
    char currentlyMessaging[11];
    char userId[11];
};

typedef struct active_users{
    int client_socket;
    char userName[30];
    char userId[11];
    struct active_users* next_active_user;
}ACTIVEUSERS;

typedef struct message{
    char senderName[30];
    char receiverName[30];
    char senderId[11];
    char receiverId[11];
    char message[30];
    int isRead;;
    char messageId[11];
}MESSAGE;


void printMenu();
int logIn_signIn_user(int client_fd,void* args);
int generate_unique_id();
void* handle_server(void *args);

int main(int argc, char const *argv[]) {
    int status, valread, client_fd;
    struct sockaddr_in serv_addr;
    char buffer[1024];
    int selection;
    pthread_t thread;

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

    //* burda birde serverden gelen mesajlari dinlemek icin surekli calismasini bekledigimiz bir threade ihtiyacimiz var. 
    
    // Thread için veri yapısını oluştur
    struct ThreadData thread_data;
    thread_data.messaging_now = 0;
    thread_data.client_fd = client_fd;

    // Thread oluştur
    if (pthread_create(&thread, NULL, handle_server, (void *)&thread_data) != 0) {
        perror("Could not create thread");
        return -1;
    }

    pthread_detach(thread);

    if(logIn_signIn_user(client_fd,(void* )&thread_data) < 0){
        // Bağlantıyı kapat
        close(client_fd);
        pthread_cancel(thread);
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));   

    while (selection != 8) {
        printMenu();
        //fgets(hello, sizeof(hello), stdin);
        scanf("%d",&selection);

        switch(selection){
            case 1: //List All users 
                send(client_fd, "/listAllUsers", strlen("/listAllUsers"), 0);

                break;
            
            case 2: //List All users 
                send(client_fd, "/listMyContacts", strlen("/listMyContacts"), 0);
                
                break;

            case 3: //add client
                send(client_fd, "/addContact", strlen("/addContact"), 0);
                scanf("%s",buffer);
                send(client_fd,buffer,1024-1,0);
                break;
            

            case 4: //delete user 
                send(client_fd, "/deleteContact", strlen("/deleteContact"), 0);
                scanf("%s",buffer);
                send(client_fd,buffer,1024-1,0);
                break;
            
            case 5: //send messages
                send(client_fd, "/sendMessage", strlen("/sendMessage"), 0);
                scanf("%s",buffer);
                send(client_fd,buffer,1024-1,0);
                strncpy(thread_data.currentlyMessaging,buffer,11);
                thread_data.messaging_now = 1; //* -> kullanicinin girdigi degerin contactslarinda var oldugunu varsayalim
                printf("THERE is the previous messages\n");

                while(thread_data.messaging_now == 1){
                    printf("Please enter /exitMessage for exitting\n");
                    scanf("%s",buffer);
                    send(client_fd,buffer,1024-1,0);
                    if(strncmp(buffer,"/exitMessage",strlen("/exitMessage")) == 0){
                        thread_data.messaging_now = 0;
                        memset(thread_data.currentlyMessaging, 0, 11); 
                    }
                    memset(buffer, 0, sizeof(buffer)); 
                }

                break;

            case 6: //list contacts
                send(client_fd, "/checkMessages", strlen("/checkMessages"), 0);
                printf("All the users is listed below\n");
               
                break;
            
            case 7: //list All active Users
                printf("Here are the all of the active users:\n");
                send(client_fd, "/listAllActiveUsers", strlen("/listAllActiveUsers"), 0);

                break;

             case 8: //exit

                  send(client_fd, "/logOutUser", strlen("/logOutUser"), 0);
                
                break;

            defeult:
                printf("There is an error with your choice");
                break;
        }
    }

    // Bağlantıyı kapatildi
    close(client_fd); 
    
    return 0;
}

void* handle_server(void* args){
    struct ThreadData *thread_data = (struct ThreadData *)args;
    int client_fd = thread_data->client_fd;
    char buffer[1024] = {0};
    int isExit = 1;
    USER* user = (USER*)malloc(sizeof(USER));
    ACTIVEUSERS* activeUser = (ACTIVEUSERS*) malloc(sizeof(ACTIVEUSERS));
    MESSAGE* message = (MESSAGE*) malloc(sizeof(MESSAGE));

    while (isExit) {

        ssize_t valread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            perror("EXITTING\n");
            isExit = 0;
        }

        if(strncmp(buffer,"/close",strlen("/close")) == 0){
            printf("Server has denied your id\n");
            isExit=-1;

        }else if(strncmp(buffer,"/userStruct",strlen("/userStruct")) == 0){
            send(client_fd, "/ready", strlen("/ready"), 0);
            recv(client_fd, user, sizeof(USER),0);
            printf("Id: %s, Name: %s, Surname: %s , Phone: %s \n",user->id,user->name,user->surname,user->phone);

        }else if(strncmp(buffer,"/string",strlen("/string")) == 0){
            send(client_fd, "/ready", strlen("/ready"), 0);
            read(client_fd, buffer, sizeof(buffer) - 1);
            printf("%s",buffer);

        }else if(strncmp(buffer,"/activeUserStruct",strlen("/activeUserStruct")) == 0){
            send(client_fd, "/ready", strlen("/ready"), 0);
            recv(client_fd, activeUser, sizeof(ACTIVEUSERS),0);
            printf("Socket: %d , Id: %s, Name: %s \n",activeUser->client_socket,activeUser->userId,activeUser->userName);

        }else if(strncmp(buffer,"/userMessageStruct",strlen("/userMessageStruct")) == 0){
           
            send(client_fd,"/read",strlen("/ready"),0);
            recv(client_fd,message,sizeof(MESSAGE),0);
            if(strncmp(message->senderId,thread_data->currentlyMessaging,11) == 0 || strncmp(message->senderId,thread_data->userId,11) == 0){
                printf("%s: %s, %d (1=read,0=not_read) \n",message->senderName,message->message,message->isRead);
            }else{
                printf("You have new message from %s \n",message->senderName);
            }

        }else if(strncmp(buffer,"/setMessaging",strlen("/setMessaging")) == 0){

            send(client_fd, "/ready", strlen("/ready"), 0);
            read(client_fd, buffer, sizeof(buffer) - 1);

            if(strncmp(buffer,"/exitMessaging",strlen("/exitMessaging")) == 0){
                thread_data->messaging_now = 0;

            }else if(strncmp(buffer,"/startMessaging",strlen("/startMessaging")) == 0){
                thread_data->messaging_now = 1;
            }                                
        }

        memset(buffer, 0, sizeof(buffer)); 
    }
    // Threadi sonlandır
    pthread_exit(NULL);
}

int logIn_signIn_user(int client_fd,void* args){
    struct ThreadData *thread_data = (struct ThreadData *)args;
    int selection;
    USER* user = (USER*) malloc(sizeof(USER));
    user->next = NULL;
    user->contacts = NULL;
    int id ;
    char idChar[11];

    printf("You can login with current account or Create new one\n");
    printf("1-Login\n");
    printf("2-Sign Up\n");
    
    scanf("%d",&selection);

    if(selection == 1){
        //* login user 
        send(client_fd, "/loginUser", strlen("/loginUser"), 0);

        printf("10 haneli id degerinizi giriniz:");
        scanf("%s",user->id);
        
        strncpy(thread_data->userId,user->id,11);
        send(client_fd, user, sizeof(USER), 0);
        

    }else if(selection == 2){
        //* signUp user
        send(client_fd, "/signUpUser", strlen("/signUpUser"), 0);

        id = generate_unique_id();
        sprintf(user->id, "%d", id);

        printf("please enter your name:");
        scanf("%s",user->name);

        printf("please enter your surname:");
        scanf("%s",user->surname);

        printf("please enter your phone number:");
        scanf("%s",user->phone);

        printf("\nYour id is: %s\n",user->id);
        printf("Your name is: %s\n",user->name);
        printf("Your surname is: %s\n",user->surname);
        printf("Your phone is: %s\n\n",user->phone);

        //* simdi user struct verisini karsiya yollayalim 
        strncpy(thread_data->userId,user->id,11);
        send(client_fd, user, sizeof(USER), 0);

    }else{
        printf("You made wrong selection program exitting\n");
        free(user);
        return -1;
    }

    free(user);
    return 1;
}


int generate_unique_id() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int)tv.tv_sec;
}


void printMenu(){
    printf("Please type your selection:\n");
    printf("1-List All users\n");
    printf("2-List Contacts\n");
    printf("3-Add Contact\n");
    printf("4-Delete Contact\n");
    printf("5-Send Message\n");
    printf("6-Check Messages\n");
    printf("7-Check Active Users\n");
    printf("8-exit\n");
    printf("selection:");

}
