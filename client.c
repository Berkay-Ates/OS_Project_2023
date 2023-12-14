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
    char name[30];
    char surname[30];
    char id[11];
    struct user* next;

}USER;

struct ThreadData {
    int client_fd;
};

void printMenu();
int logIn_signIn_user(int client_fd);
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
    thread_data.client_fd = client_fd;

    // Thread oluştur
    if (pthread_create(&thread, NULL, handle_server, (void *)&thread_data) != 0) {
        perror("Could not create thread");
        return -1;
    }

    if(logIn_signIn_user(client_fd) < 0){
        // Bağlantıyı kapat
        close(client_fd);
        pthread_cancel(thread);
        return -1;
    }

    memset(buffer, 0, sizeof(buffer));   

    while (selection != 7) {
        printMenu();
        //fgets(hello, sizeof(hello), stdin);
        scanf("%d",&selection);

        switch(selection){
            case 1: //List All users 
                send(client_fd, "/listAllUsers", strlen("/listAllUsers"), 0);
                printf("All Users are listed below:\n");

                break;
            
            case 2: //List All users 
                send(client_fd, "/listMyContacts", strlen("/listMyContacts"), 0);
                printf("All your clients are listed below:\n");

                break;

            case 3: //add user 
                send(client_fd, "/addNewUser", strlen("/addNewUser"), 0);
                printf("Please select which user you want to add to your contacts List\n");
                
                break;
            

            case 4: //delete user 
                send(client_fd, "/deleteUser", strlen("/deleteUser"), 0);
                printf("Please select which user you want to delete\n");
                
                break;
            
            case 5: //send messages
                send(client_fd, "/sendMessage", strlen("/sendMessage"), 0);
                printf("Please select which user and type your messages");
                
                break;
            
            case 6: //list contacts
                send(client_fd, "/checkMessages", strlen("/checkMessages"), 0);
                printf("All the users is listed below\n");
               
                break;
            
            case 7: //exit
                  send(client_fd, "/logOutUser", strlen("/logOutUser"), 0);
                
                break;

            defeult:
                printf("There is an error with your choice");
                
                break;
        }
    }

    // Bağlantıyı kapat
    pthread_cancel(thread);
    close(client_fd); 

    return 0;
}

void* handle_server(void* args){
    struct ThreadData *thread_data = (struct ThreadData *)args;
    int client_fd = thread_data->client_fd;
    char buffer[1024] = {0};
    int isExit = 1;
    USER* user = (USER*)malloc(sizeof(USER));

    while (isExit) {
        // Serverdan gelen veriyi oku
        ssize_t valread = read(client_fd, buffer, sizeof(buffer) - 1);
        if (valread <= 0) {
            perror("Error reading from server\n");
            isExit = 0;
        }

        //* server tarafi once ne yollayacagini soyleyecek sonra da yollacacagi veriyi yollayacak 
        //* yollayacagi veri bittikten sonra da bitirdigini bildiren bir bitti paketi yollayacak

        if(strncmp(buffer,"/close",strlen("/close"))== 0){
            printf("Server has denied your id\n");
            isExit=-1;

        }else if(strncmp(buffer,"/userStruct",strlen("/userStruct")) == 0){
            send(client_fd, "/ready", strlen("/ready"), 0);
            recv(client_fd, user, sizeof(USER),0);
            printf(" Id: %s, Name: %s, Surname: %s\n",user->id,user->name,user->surname);

        } 

        memset(buffer, 0, sizeof(buffer));
    }

    // Threadi sonlandır
    pthread_exit(NULL);
}

int logIn_signIn_user(int client_fd){
    int selection;
    USER* user = (USER*) malloc(sizeof(USER));
    user->next = NULL;
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

        printf("\nYour id is: %s\n",user->id);
        printf("Your name is: %s\n",user->name);
        printf("Your surname is: %s\n\n",user->surname);

        //* simdi user struct verisini karsiya yollayalim 
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
    printf("3-Add User\n");
    printf("4-Delete User\n");
    printf("5-Send Message\n");
    printf("6-Check Messages\n");
    printf("7-exit\n");
    printf("selection:");

}
