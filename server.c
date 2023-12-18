#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>  // Eklenen başlık dosyası
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>

#define PORT 8080

//* git token is below
//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

typedef struct user{
    char phone[30];
    char name[30];
    char surname[30];
    char id[11];
    struct user* next;
    struct user* contacts;
}USER;

typedef struct active_users{
    int client_socket;
    char userName[30];
    char userId[11];
    struct active_users* next_active_user;
}ACTIVEUSERS;

typedef struct threadArgs{
    ACTIVEUSERS* active_users_head;
    USER* userHead ; 
    int socket;
    char currentUserId[11];
}THREADARGS;

typedef struct message{
    char senderId[11];
    char receiverId[11];
    char message[128];
}MESSAGE;

int generate_unique_id();
int logIn_signIn_user(void *args);
void *handle_client(void *args);
void *setAllUsersFromFile(char* fileName,USER* userHead);
USER* mallocUser(char* id, char* name, char* surname,char* phone);
void sendUserList(void* args);
int sendUserContacts(USER* head,char* id,void* args);

void add_new_active_user(ACTIVEUSERS** active_users_head, int socket_id,USER* user);
void delete_active_user(ACTIVEUSERS** activeUserHead,int client_socket);
void printActiveUsers(ACTIVEUSERS* active_users_head);

void addUserToClientContactList(USER* userHead,char* userId,char* contactId,void* args);
int deleteContact(USER* userHead,char* userId,char* delContactId,void* args);
void printAllUsers(USER* user);
void freeAllUser(USER* head);

int main(int argc, char const *argv[]) {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);

    USER* userHead = (USER*) malloc(sizeof(USER)); 
    strcpy(userHead->id,"0000000000");
    strcpy(userHead->name,"USER HEAD COMMON WITH");
    strcpy(userHead->surname,"ALL THE THREADS");

    userHead->next = NULL;
    userHead->contacts = NULL;

    ACTIVEUSERS* act_user_head = (ACTIVEUSERS*) malloc(sizeof(ACTIVEUSERS));
    act_user_head->next_active_user = NULL;
    strcpy(act_user_head->userId,"00000000000");


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

        THREADARGS* threadArgs = (THREADARGS*) malloc(sizeof(THREADARGS));
        threadArgs->userHead = userHead;
        threadArgs->active_users_head = act_user_head;

        // Her bağlantı için bir thread oluştur
        pthread_t thread;
        threadArgs->socket = new_socket;

        if (pthread_create(&thread, NULL, handle_client, (void *)threadArgs) < 0) {
            perror("could not create thread on server side");
            exit(1);
        }

        // Detached thread oluştur
        pthread_detach(thread);
    }

    // Dinleme soketini kapat
    close(server_fd);
    return 0;
}

void printActiveUsers(ACTIVEUSERS* active_users_head){
    ACTIVEUSERS* tmp = active_users_head;
    printf("Here are the all of the active users\n");

    while(tmp != NULL){
        printf("socket: %d, User Name: %s, User Id: %s\n",tmp->client_socket,tmp->userName,tmp->userId);
        tmp=tmp->next_active_user;
    }
}

void delete_active_user(ACTIVEUSERS** active_users,int socket){
    ACTIVEUSERS* tmp = *active_users;
    ACTIVEUSERS* tmp2 = tmp;

    while(tmp != NULL && tmp->client_socket != socket){
        tmp2 = tmp;
        tmp = tmp->next_active_user;
    }

    if(tmp==NULL) // silinmek istenen eleman tabloda bulunmuyor
        return;

    if(tmp == (*active_users)){
        // head silinecek
        (*active_users) = (*active_users)->next_active_user;
        free(tmp);
    }else{
        tmp2->next_active_user=tmp->next_active_user;
        free(tmp);
    }

}

void freeAllUser(USER* head){
    USER* tmp = head->next;
    USER* tmp2 = tmp;
    head->next=NULL;
    while(tmp!=NULL){
        tmp2 = tmp;
        tmp = tmp->next;
        free(tmp2);
    }
}

void printAllUsers(USER* user){
    USER* tmp = user;

    printf("All of the users All following\n");
    while(user != NULL){
        printf("%s  %s  %s  %s  \n",user->id,user->name,user->surname,user->phone);
        user=user->next;
    }
}

void add_new_active_user(ACTIVEUSERS** active_users_head, int socket_id,USER* user){
    ACTIVEUSERS* tmp = (ACTIVEUSERS*) malloc(sizeof(ACTIVEUSERS));
    ACTIVEUSERS* tmp2 = *active_users_head;

    if(tmp==NULL){
        printf("There was error while allocating space for the new active user\n");
        exit(-1);
    }

    tmp->client_socket = socket_id;
    strcpy(tmp->userName,user->name);
    strcpy(tmp->userId,user->id);

    while(tmp2->next_active_user != NULL)
        tmp2 = tmp2->next_active_user;

    tmp2->next_active_user = tmp; // en iyi ihtimalle headin nextine yazarak headi muhafaza ediyoruz
}


void *handle_client(void *args){

    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    USER* userHead = threadArgs->userHead;
    ACTIVEUSERS* actUserHead = threadArgs->active_users_head;
    
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

    freeAllUser(threadArgs->userHead);
    setAllUsersFromFile("./apps/allUsers",threadArgs->userHead);
    // Dosyada tum elemanlar oldugu icin sign up olma ihtimaline karsi simdi tum kullanicilari silelim
    // ve bastan dosyadan atayalim eski diziden kurtulmadan eklersek memory leak olur

    while (isExit) {

        printAllUsers(userHead);

        printActiveUsers(threadArgs->active_users_head);

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
            sendUserContacts(threadArgs->userHead,threadArgs->currentUserId,args);

        }else if(strncmp(buffer,"/addContact",strlen("/addContact")) == 0){
           sendUserList(args);

           send(client_socket,"/string",strlen("/string"),0);
           read(client_socket, buffer, strlen("/ready"));
           strcpy(buffer,"Yukaridaki contack listenize eklemek istediginizin id degerini giriniz:\n");
           send(client_socket,buffer,1024-1,0);

           //* simdi buffera kullanici isminin gelmesini bekliyoruz
           read(client_socket,buffer,1023);
           printf("gelen ID degeri:%s \n",buffer);
           //* add user to client contact list
           addUserToClientContactList(threadArgs->userHead,threadArgs->currentUserId,buffer,args);
            printf("Contact EKLENDI ******************************************\n");

        
        }else if(strncmp(buffer,"/deleteContact",strlen("/deleteContact")) == 0){
            printf("from server /deleteContact\n");
            sendUserContacts(threadArgs->userHead,threadArgs->currentUserId,args);
             //* simdi buffera kullanici isminin gelmesini bekliyoruz
            send(client_socket,"/string",strlen("/string"),0);
            read(client_socket, buffer, strlen("/ready"));
            strcpy(buffer,"Yukaridan silmek istediginizin id degerini giriniz:\n");
            send(client_socket,buffer,1024-1,0);

            read(client_socket,buffer,1023);
            printf("Silinmesi icin gelen ID degeri:%s \n",buffer);
            deleteContact(threadArgs->userHead, threadArgs->currentUserId,buffer,args);


        }else if(strncmp(buffer,"/sendMessage",strlen("/sendMessage")) == 0){
            printf("from server /deleteContact\n");
            sendUserContacts(threadArgs->userHead,threadArgs->currentUserId,args);

            send(client_socket,"/string",strlen("/string"),0);
            read(client_socket, buffer, strlen("/ready"));
            strcpy(buffer,"Contactlarinizdan mesaj atmak istediginizi secin:\n");
            send(client_socket,buffer,1024-1,0);

            read(client_socket,buffer,1023);
            printf("Mesaj yazmak icin gelen ID degeri:%s \n",buffer);

        }else if(strncmp(buffer,"/checkMessages",strlen("/checkMessages")) == 0){
            printf("from server /checkMessages\n");
            printf("Buffer: %s\n\n", buffer);
        
        }else if(strncmp(buffer,"/logOutUser",strlen("/logOutUser")) == 0){
            delete_active_user(&(threadArgs->active_users_head),client_socket);
            printActiveUsers((threadArgs->active_users_head));
            printf("from server /logOutUser\n");
            printf("Buffer: %s\n\n", buffer);
            isExit = 0;
        }   
    }
    // Soketi kapat
    close(client_socket);

    pthread_exit(NULL);
}


int deleteContact(USER* userHead,char* userId,char* delContactId,void* args){
    
    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
   
    USER* user =  userHead->next; 
    USER* userContact;
    USER* userContact2;
    USER* userContact3=NULL;

    FILE* contactFile;
    char fileName[30];

    while(user!=NULL && strcmp(user->id,userId) != 0){
        printf("roaming on the users \n");
        user = user->next;
    }

    if(user == NULL){
        printf("olmayan user icin contactlarindan biri silinmeye calisildi\n");
        return -1;
    }

    printf("usering contactina ulasiliiyor===================\n");
    userContact = user->contacts;



     printf("usering contactina ulasildi===================\n");

    while(userContact != NULL && strcmp(userContact->id,delContactId) != 0){
        printf("roaming on the contacts------------------------- \n");
        userContact2 = userContact;
        userContact = userContact ->contacts;
    }

    if(userContact == NULL){
        printf("olmayan bir contact user listesinden silinmeye calisildi\n");
        return -1;
    }

    if(userContact == user->contacts){
        //head siliniyor demektir.
        user->contacts = user->contacts->contacts;
    }else
        userContact2->contacts = userContact->contacts;

    userContact3 = user->contacts;

    //* simdi user contactin contactlarini dosyaya tekrardan yazalim 
    strcpy(fileName,"./");
    strcat(fileName,user->name);
    strcat(fileName,"_");
    strcat(fileName,user->surname);
    strcat(fileName,"/contacts");


    contactFile = fopen(fileName,"w");
    printf("dosya =========================== %p\n",contactFile);

    while(userContact3!=NULL){
        fwrite(userContact3,sizeof(USER),1,contactFile);
        userContact3=userContact3->contacts;
    }

    fclose(contactFile);
    free(userContact);
}



void addUserToClientContactList(USER* userHead,char* userId,char* contactId,void* args){

    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;

    USER* user = userHead->next;
    USER* userContact ;
    USER* tmpContact;
    USER* tmp;
    char fileName[30];
    FILE* tmpFile;

    while(strcmp(user->id,userId) !=0 && user != NULL){
        user = user->next;
    }

    if(user == NULL){
        printf("Client sent invalid ID so request Failed wich was belong to id= %s \n",userId);
        return;
    }

    //* user bulundu => user da 

    userContact = userHead->next;    

    while(userContact != NULL && strcmp(userContact->id,contactId) != 0 ){
        userContact = userContact->next;
    }

    if(userContact == NULL){
        printf("Client sent invalid ID so request Failed wich was belong to id=%s client\n",userId);
        return;
    }

    //* eklenecek kontakt bulundu => userContact da 

    if(user->contacts == NULL){
        tmp = mallocUser(userContact->id,userContact->name,userContact->surname,userContact->phone);
        user->contacts = tmp;
        
    } else{
        tmpContact = user->contacts;
        while(tmpContact->contacts != NULL){
            tmpContact = tmpContact->contacts;
        }

        tmp = mallocUser(userContact->id,userContact->name,userContact->surname,userContact->phone);
        tmpContact->contacts = tmp;

    }

    strcpy(fileName,"./");
    strcat(fileName,user->name);
    strcat(fileName,"_");
    strcat(fileName,user->surname);
    strcat(fileName,"/contacts");
        
    tmpFile = fopen(fileName,"a+");
    fwrite(tmp,sizeof(USER),1,tmpFile);
    fclose(tmpFile);
}



void sendUserList(void* args){

    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    USER* userHead = threadArgs->userHead;
    char buffer[256];
    
    while(userHead != NULL){
        send(client_socket, "/userStruct", strlen("/userStruct"), 0);
        read(client_socket, buffer, strlen("/ready"));
        send(client_socket, userHead, sizeof(USER), 0);
        printf("kullanici yollandi\n");
        userHead = userHead->next;
    }
}

int logIn_signIn_user(void* args){
    
    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    USER* userHead = threadArgs->userHead;
    ACTIVEUSERS* activeUserHead = threadArgs->active_users_head;

    FILE* tmpFiles = fopen("./apps/allUsers","a+");

    ssize_t valread;
    char buffer[1024] = {0};
    memset(buffer, 0, sizeof(buffer));

    USER* userTmp = (USER*) malloc(sizeof(USER));
    USER* userTmp2 = (USER*) malloc(sizeof(USER));

    valread = read(client_socket, buffer, 1024 - 1);
    if (valread <= 0) {
        printf("There was an error in server side while reading user command user did not choose login or signup \n");
        exit(-1);
    }

    if(strncmp(buffer,"/signUpUser",strlen("/signUpUser")) == 0){
        valread = recv(client_socket, userTmp, sizeof(USER),0);

        if(valread == sizeof(USER)){
            printf("\nYour id is: %s\n",userTmp->id);
            printf("Your name is: %s\n",userTmp->name);
            printf("Your surname is: %s\n",userTmp->surname);
            printf("Your phone is:%s \n",userTmp->phone);
            // useri dosyaya kaydedip sonrasinda return 1 
            
            fwrite(userTmp,sizeof(USER),1,tmpFiles);

            if (ferror(tmpFiles))
                perror("Error writing to file");

            
            strcpy(buffer,userTmp->name);
            strcat(buffer,"_");
            strcat(buffer,userTmp->surname);

            int result = mkdir(buffer,0777);

            if (result == 0) {
                printf("Dizin oluşturuldu: %s\n", buffer);
            } else {
                perror("Dizin oluşturma hatasi");
            }

            strcpy(threadArgs->currentUserId,userTmp->id);

            add_new_active_user(&(threadArgs->active_users_head),client_socket,userTmp);

            fclose(tmpFiles);
            return 1;

        }else{
            printf("There was error while taking user struct for sign up\n");
            free(userTmp);
            return -1;
        }

    }else if(strncmp(buffer,"/loginUser",strlen("/loginUser")) == 0){
        valread = recv(client_socket, userTmp, sizeof(USER),0);
        if(valread == sizeof(USER)){

            printf("\nYour id is: %s\n",userTmp->id);
            printf("Your name is: %s\n",userTmp->name);
            printf("Your surname is: %s\n\n",userTmp->surname);
            // useri dosyaya kaydedip sonrasinda return 1 

            while(fread(userTmp2,sizeof(USER),1,tmpFiles) == 1){
                if(strcmp(userTmp2->id,userTmp->id) == 0){
                    printf("%s have been successfully logged in.\n",userTmp->id);

                    strcpy(threadArgs->currentUserId,userTmp->id);
                    fclose(tmpFiles);

                    add_new_active_user(&(threadArgs->active_users_head),client_socket,userTmp2);

                    return 1;
                }
            }

            if (ferror(tmpFiles))
                perror("Error writing to file");
            
            printf("%s did not found in saved users operation failed\n",userTmp->id);
            printf("User claimed with %s access denied.",userTmp->id);
            fclose(tmpFiles);
            free(userTmp);
            return -1;

        }else{
            printf("There was error while taking user struct for sign up\n");
            free(userTmp);
            return -1;
        }
    }
}

int sendUserContacts(USER* head,char* id,void* args){
    THREADARGS* threadArgs = (THREADARGS *)args;
    int client_socket = threadArgs->socket;
    char* buffer[256];

    USER* userHead = head ->next;
    USER* userContact;

    while(userHead!=NULL && strcmp(userHead->id,id) != 0){
        userHead=userHead->next;
    }

    if(userHead == NULL){
        printf("The id was not exists in current users");
        return  -1;
    }

    userContact = userHead->contacts;
    printf("Here is the contact of the %s id preson",userHead->id);

    while(userContact != NULL){
        send(client_socket, "/userStruct", strlen("/userStruct"), 0);
        read(client_socket, buffer, strlen("/ready"));
        send(client_socket, userContact, sizeof(USER), 0);
        printf("contact yollandi\n");
        userContact = userContact->contacts;
    }

    return 0;
}

void *setAllUsersFromFile(char* fileName,USER* userHead){
    USER* tmp = userHead;
    USER userTmp;
    USER* contactTmp;
    USER userBUFF;
    char contactFile[30];
    FILE* tmpFiles = fopen(fileName,"r");
    FILE* contactFP;

    while(fread(&userTmp,sizeof(USER),1,tmpFiles) == 1){
        tmp->next = mallocUser(userTmp.id,userTmp.name,userTmp.surname,userTmp.phone);
        tmp = tmp->next;
        contactTmp = tmp;
        //TODO set users all friends exists in the folder
        strcpy(contactFile,"./");
        strcat(contactFile,tmp->name);
        strcat(contactFile,"_");
        strcat(contactFile,tmp->surname);
        strcat(contactFile,"/contacts");
        contactFP = fopen(contactFile,"r");
        printf("%s ******** %s ******%p***********\n",contactFile,tmp->name,contactFP);

        while(contactFP != NULL && fread(&userBUFF,sizeof(USER),1,contactFP) == 1){
            contactTmp->contacts = mallocUser(userBUFF.id,userBUFF.name,userBUFF.surname,userBUFF.phone);
            printf("%s *************\n",tmp->name);
            contactTmp=contactTmp->contacts;
        }
        if(contactFP!=NULL)
            fclose(contactFP);
    }

    fclose(tmpFiles);

}


USER* mallocUser(char* id, char* name, char* surname,char* phone){
    USER* tmp = (USER*) malloc(sizeof(USER));
    if(tmp==NULL){
        printf(" There was an error while allocating a user\n");
        exit(-1);
    }
    strcpy(tmp->id,id);
    strcpy(tmp->name,name);
    strcpy(tmp->surname,surname);
    strcpy(tmp->phone,phone);
    tmp->next=NULL;
    tmp->contacts=NULL;
    
    return tmp; 
}

