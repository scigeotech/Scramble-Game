#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#define server_port "8000"
#define processing_length 4096
char *success_response = "HTTP/1.1 200 OK\r\ncontent-type: text/html; charset=UTF-8 \r\n\r\n";
//------------------------------------------GAME START------------------------------------------
char user_input[100];
struct wordListNode {char string[30]; struct wordListNode *next;};
typedef struct wordListNode wordListNode; //refer to node structure by its name
wordListNode* root_node = NULL;
int word_count = 0;

struct gameListNode {char string[30]; bool foundWord; struct gameListNode *next;};
typedef struct gameListNode gameListNode; //refer to node structure by its name
gameListNode* root_game_node = NULL;

wordListNode master_word_node;
//-------------------- init functions
void new_wordListNode(wordListNode* *head, char str_input[30]) {
    wordListNode* this_node = (wordListNode*)malloc(sizeof(wordListNode));
    strncpy(this_node->string, str_input, sizeof(this_node->string)); //fill node
    this_node->next = NULL; //set next to null for now

    if (*head == NULL) {
        *head = this_node; //if no head exists, this is first
    } else {
        wordListNode *last_node = *head;
        while (last_node->next != NULL) {
            last_node = last_node->next; //go to last node
        }
        last_node->next = this_node; //put this node at the end
    }
}

int initialization() {
    srand(time(NULL)); //initiate RNG on time
    FILE *read_file = fopen("2of12.txt", "r"); //open file "2or12.txt", read-only
    char buffer[30];
    int node_count = 0;
    while (fgets(buffer, sizeof(buffer), read_file) != NULL) { //while file can still be read
        buffer[strcspn(buffer, "\r\n")] = '\0'; //remove newline
        for (int i=0; buffer[i]; i++) { //capitalize string for easier processing
            buffer[i] = toupper(buffer[i]);
        }
        new_wordListNode(&root_node, buffer); //create new node using read word, based off root node
        node_count++;
    }
    fclose(read_file);
    return node_count; //return count of words in dictionary, do later
}

//-------------------- Processing --------------------
void getLetterDistribution(char* word, int* dist) {
    for (int i=0; word[i]; i++) {
        if (isalpha(word[i])) { //is actual letter? in case of nums
            dist[word[i] - 'A']++; //increment letter freq. (array pos. relative to a)
        } //all inputs should have been capitalized beforehand, no safety checks
    }
}
bool compareCounts(int* choices, int* candidate) {
    for(int i=0; i<26; i++) {
        if (choices[i] < candidate[i]) { //if more letters used given
            return false; //immediate fail, any other checking is redundant
        }
    }
    return true;
}
wordListNode getRandomWord() {
    int pick_random = rand() % word_count; //pick random node from 0-word count
    int random_index = 0; //keep count of progress
    wordListNode* current_node = root_node; //start at root node
    while (random_index < pick_random) { //while index isn't at chosen node
        if((strlen(current_node->string) > 6) && (rand() % 5 == 0)) {
            return *current_node; //return "master word".
        } //note: 1/5 chance of picking word
        random_index++;
        current_node = current_node->next; //keep going to next node
    }
    return *current_node; //backup, return last (pick_random) node. if rand was 0, give first node.
}
void new_gameListNode(gameListNode* *head, char str_input[30]) { //for function below
    gameListNode* this_node = (gameListNode*)malloc(sizeof(gameListNode));
    strncpy(this_node->string, str_input, sizeof(this_node->string)); //fill node
    this_node->next = NULL; //set next to null for now
    this_node->foundWord = false;

    if (*head == NULL) {
        *head = this_node; //if no head exists, this is first
    } else {
        gameListNode *last_node = *head;
        while (last_node->next != NULL) {
            last_node = last_node->next; //go to last node
        }
        last_node->next = this_node; //put this node at the end
    }
}
void findWords(char master_word[30]) {
    int master_dist[26] = {0};
    getLetterDistribution(master_word, master_dist); //create base comparison dist
    wordListNode* current_node = root_node;
    while (current_node != NULL) { //check every node from root node
        int word_dist[26] = {0};
        getLetterDistribution(current_node->string, word_dist);
        if (compareCounts(master_dist, word_dist)) {
            new_gameListNode(&root_game_node, current_node->string);
        }
        current_node = current_node->next;
    }
}

bool isDone() {
    gameListNode* current_node = root_game_node;
    while (current_node != NULL) { //if even one word not found, immediate fail
        if (current_node->foundWord == false) {
            return false;
        }
        current_node = current_node->next;
    }
    return true; //no failures, thus true
}

void displayWorld(char HTML_buffer[processing_length], size_t buffer_size) {
    printf("displayworld test\n"); //testing
    int master_dist[26] = {0};
    getLetterDistribution(master_word_node.string, master_dist);
    sprintf(HTML_buffer, "%s", success_response);

    strncat(HTML_buffer, "<!DOCTYPE html><html><title>Scramble!</title><body>", buffer_size - strlen(HTML_buffer) - 1);
    strncat(HTML_buffer, "<p>Letters: ", buffer_size - strlen(HTML_buffer) - 1);
    for (int i=0; i<26; i++) {
        for(int j=0; j<master_dist[i]; j++) { //print letter depending frequency
            char str_hold[4]; //secondary buffer to hold "A "
            snprintf(str_hold, sizeof(str_hold), "%c ", 'A' + i);
            strncat(HTML_buffer, str_hold, buffer_size - strlen(HTML_buffer) - 1);
        }
    }
    strncat(HTML_buffer, "<p>", buffer_size - strlen(HTML_buffer) - 1);
    strncat(HTML_buffer, "<h3>[------------------------------------------------]</h3>", buffer_size - strlen(HTML_buffer) - 1);
    gameListNode* current_node = root_game_node;
    while (current_node != NULL) { //per node
        if (current_node->foundWord) { //if word found (bool)
            strncat(HTML_buffer, "<p>FOUND: ", buffer_size - strlen(HTML_buffer) - 1);
            strncat(HTML_buffer, current_node->string, buffer_size - strlen(HTML_buffer) - 1);
            strncat(HTML_buffer, "</p>", buffer_size - strlen(HTML_buffer) - 1);
        } else {
            strncat(HTML_buffer, "<p>", buffer_size - strlen(HTML_buffer) - 1);
            char underscore_buffer[30] = "";
            printf("HINT : %s\n", current_node->string); //testing
            for(int i=0; i<strlen(current_node->string); i++) { //print underscore per letter
                strncat(underscore_buffer, "_ ", buffer_size - strlen(HTML_buffer) - 1);
            }
            strncat(HTML_buffer, underscore_buffer, buffer_size - strlen(HTML_buffer) - 1);
            strncat(HTML_buffer, "</p>", buffer_size - strlen(HTML_buffer) - 1);
        }
        current_node = current_node->next;
    }
    if (!isDone()) { //continue
        strncat(HTML_buffer, "<p>Enter a guess:</p>", buffer_size - strlen(HTML_buffer) - 1);
        strncat(HTML_buffer, "<form action=\"words\" method=\"get\">", buffer_size - strlen(HTML_buffer) - 1);
        strncat(HTML_buffer, "<input type=\"text\" name=\"move\" autofocus>", buffer_size - strlen(HTML_buffer) - 1);
        strncat(HTML_buffer, "<input type=\"submit\" value=\"Submit\"></input></form>", buffer_size - strlen(HTML_buffer) - 1);
    } else { //victory
        strncat(HTML_buffer, "<p>Congratulations! You solved it! <a href=\"words?move=\">Another?</a></p>", buffer_size - strlen(HTML_buffer) - 1);
        strncat(HTML_buffer, "<p>You can also quit: <a href=\"words?move=quit_now_please\">Click here to end the game.</a></p>", buffer_size - strlen(HTML_buffer) - 1);
    }
    strncat(HTML_buffer, "</body></html>", buffer_size - strlen(HTML_buffer) - 1);
    //printf("Page output: %s\n", HTML_buffer); //testing
}
//-------------------- cleanup
void cleanupGameListNodes() {
    gameListNode* current_node = root_game_node;
    while (current_node != NULL) {
        gameListNode* nextNode = current_node->next; //for reference; prevent memory orphanage
        free(current_node);
        current_node = nextNode; //access nextNode reference. eventually NULL.
    }
}
void cleanupWordListNodes() {
    wordListNode* current_node = root_node;
    while (current_node != NULL) {
        wordListNode* nextNode = current_node->next; //for reference; prevent memory orphanage
        free(current_node);
        current_node = nextNode; //access nextNode reference. eventually NULL.
    }
}
void teardown() {
    printf("All Done.\n");
}
//------------------------------------------------Server / Game Handling------------------------------------------------
char *input_path;
char file_missing[64] = "HTTP/1.0 404 Not Found\r\nContent-Length: 13\r\n\r\n404 Not Found";

void acceptInput(char *url) {
    char *url_process = strstr(url, "?move="); //test for proper url
    url_process += 6; //increment to get to the important part
    strncpy(user_input, url_process, strlen(url_process) + 1); //copy url_process to user_input for easy processing
    printf("%s, %s\n", url_process, user_input); //testing
    for (int i=0; user_input[i]; i++) { //capitalize input for easier processing
        user_input[i] = toupper(user_input[i]);
    }
    gameListNode* current_node = root_game_node;
    while (current_node != NULL) { //for each gameListNode, match with input
        if (strcmp(current_node->string, user_input) == 0) {
            current_node->foundWord = true;
            return; //there should be no duplicate words or nodes; end loop.
        }
        current_node = current_node->next;
    }
    printf("User input was: %s\n", user_input);
}

int server_setup() {
    int socket_fd; //socket/listen on socket_fd, accept on new_fd
    struct addrinfo hints, *serv_info, *current;

    memset(&hints, 0, sizeof(hints)); //hints for connection. clear to 0
    hints.ai_family = AF_UNSPEC; //for socket()
    hints.ai_socktype = SOCK_STREAM; //TCP
    hints.ai_flags = AI_PASSIVE; //my ip address

    //get destination data
    int addr_info = getaddrinfo(NULL, server_port, &hints, &serv_info); //self (NULL), port 8000, fill serv_info
    if (addr_info != 0) {//error
        fprintf(stderr, "getaddrinfo attempt error: %s\n", gai_strerror(addr_info));
        exit(1);
    }

    for (current = serv_info; current != NULL; current = current->ai_next) {
        //use TCP socket
        if ((socket_fd = socket(current->ai_family, current->ai_socktype, current->ai_protocol)) == -1) {
            printf("server socket attempt error, retrying\n");
            continue;
        }
        if (bind(socket_fd, current->ai_addr, current->ai_addrlen) == -1) {
            printf("server bind error, retrying\n");
            close(socket_fd);
            continue;
        }
        break; //success?
    }
    freeaddrinfo(serv_info); //done

    if (listen(socket_fd, 16) == -1) { //backlog 16
        printf("server listen error, exiting\n");
        exit(1);
    }

    printf("Server waiting to accept connections on port %s\n", server_port);
    return socket_fd;
}

void quick_redirect(char HTML_buffer[processing_length], size_t buffer_size) {
    sprintf(HTML_buffer, "%s", success_response);
    strncat(HTML_buffer, "<!DOCTYPE html><html>", buffer_size - strlen(HTML_buffer) - 1);
    strncat(HTML_buffer, "<head><meta http-equiv=\"refresh\" content=\"0;url=words?move=\"></head>", buffer_size - strlen(HTML_buffer) - 1);
    strncat(HTML_buffer, "<body>Redirecting to game...</body></html>", buffer_size - strlen(HTML_buffer) - 1);
}

void handleGame(int server_socket) { //formerly gameLoop, now handleGame
    master_word_node = getRandomWord(); //initialization
    root_game_node = NULL; //clear root_game_node in case of repeat. 
    findWords(master_word_node.string);

    do {
        struct sockaddr_storage client_address; //address information
        
        //accept() section
        socklen_t sin_size = sizeof(client_address); //client, server address sizes
        int client_sock = accept(server_socket, (struct sockaddr *)&client_address, &sin_size); //new_fd is client address
        printf("Connection accepted.\n");
        printf("looping\n");

        printf("url pre-processing\n");
        char receive_buffer[processing_length];
        memset(receive_buffer, 0, sizeof(receive_buffer)); //clear buffer 0
        
        int bytes_read = read(client_sock, receive_buffer, processing_length - 1);
        printf("received %s, bytes read: %d\n", receive_buffer, bytes_read); //testing
        if (bytes_read == 0) {
            printf("[READ FAILURE]\n");
            close(client_sock);
            exit(1); //fail
        }
        //GET /data.html HTTP/1.1, URL path processing
        printf("processing url\n");
        char *url = strtok(receive_buffer, " ");
        url = strtok(NULL, " ");
        printf("%s\n", url); //testing
        char *url_process = strstr(url, "?move=");
        if (url_process == NULL || strcmp(url_process, "?move=") == 0) { //reset?
            printf("acceptinput resetting game, %s\n", url);
            cleanupGameListNodes(); //reminder: DO NOT CLEAR WORDLISTNODES HERE OR GAME WILL BREAK
            printf("cleared gameListNodes\n");
            master_word_node = getRandomWord(); //initialization
            root_game_node = NULL; //clear root_game_node in case of repeat. 
            findWords(master_word_node.string);
            sprintf(url, "/words?move=start_new_game");
            if(url_process == NULL) {
                char redirect_buffer[processing_length];
                quick_redirect(redirect_buffer, sizeof(redirect_buffer));
                write(client_sock, redirect_buffer, strlen(redirect_buffer));
            }
            printf("acceptinput finished resetting game with master word %s, url %s\n", master_word_node.string, url);
        } else if (strcmp(url_process, "?move=quit_now_please") == 0) {
            printf("Quitting game...\n");
            break; //quit the game, break loop
        }
        
        printf("processing acceptInput\n"); //possibly swap with below?
        acceptInput(url);

        char HTML_buffer[processing_length] = {0};
        displayWorld(HTML_buffer, sizeof(HTML_buffer));
        write(client_sock, HTML_buffer, strlen(HTML_buffer));
        close(client_sock);
    } while (1);
}

void *handle_request(void *client_sock) {
    int client_socket = *(int*)client_sock;
    free(client_sock);

    char receive_buffer[processing_length];
    memset(receive_buffer, 0, sizeof(receive_buffer)); //clear buffer 0
    int bytes_read = read(client_socket, receive_buffer, processing_length - 1);
    if (bytes_read == 0) {
        close(client_socket);
        return NULL;
    }

    //GET /data.html HTTP/1.1
    //path processing
    char *strtok_r_save;
    char *strtok_buffer = strtok_r(receive_buffer, " ", &strtok_r_save);
    if (strncmp(strtok_buffer, "GET", strlen("GET")) != 0) {
        close(client_socket);
        return NULL;
    }
    char *processed_path = strtok_r(NULL, " ", &strtok_r_save);
    if (processed_path[0] == '/') processed_path++;
    char true_path[processing_length];
    snprintf(true_path, sizeof(true_path), "%s/%s", input_path, processed_path);

    struct stat file_status;
    //does the file exist? what is the size?
    if (stat(true_path, &file_status) < 0 || !S_ISREG(file_status.st_mode)) {
        if (stat(true_path, &file_status) < 0) {
            printf("file missing\n");
            printf("true_path: %s\n", true_path); //testing
        }
        if (!S_ISREG(file_status.st_mode)) {
            printf("is not file\n");
        }
        write(client_socket, file_missing, strlen(file_missing));
        close(client_socket);
        return NULL;
    }

    //send back response
    char success_response[processing_length];
    snprintf(success_response, sizeof(success_response), "HTTP/1.0 200 OK\r\nContent-Length: %ld\r\n\r\n", file_status.st_size);
    write(client_socket, success_response, strlen(success_response));

    int file_open = open(true_path, O_RDONLY);
    char file_buffer[processing_length];
    int file_bytes_read;
    while ((file_bytes_read = read(file_open, file_buffer, processing_length)) > 0) {
        write(client_socket, file_buffer, file_bytes_read);
    }
    close(file_open);

    close(client_socket);
    pthread_exit(NULL);
    return NULL;
}

//IT WORKS!!!
int main(int argc, char *argv[]) {
    int server_socket = server_setup();
    printf("Server ready on port %s\n", server_port);
    printf("Play game: no additional arguments, go to localhost:8000\n");
    printf("Serve files: need just 2 arguments, do ./Scramble (directory)\n\n");
    if (argc != 2) {
        printf("Starting the game...\n");
        word_count = initialization();
        handleGame(server_socket);
        cleanupGameListNodes(); //test
        cleanupWordListNodes(); //test
        teardown();
    } else {
        printf("Serving files...\n");
        //./Server files
        //http://localhost:8000/data.html
        input_path = argv[1]; //hold path
    
        //send/receive
        while(1) {
            struct sockaddr_storage client_address; //address information

            //accept() section
            socklen_t sin_size = sizeof(client_address); //client, server address sizes
            int new_fd = accept(server_socket, (struct sockaddr *)&client_address, &sin_size); //new_fd is client address
            printf("Connection accepted.\n");
        
            int *client_socket_ptr = malloc(sizeof(int));
            *client_socket_ptr = new_fd;

            pthread_t thread_id;
            //handle_request is the function that this thread will run
            //client_socket_ptr is the argument that will be passed to the function
            int pthread = pthread_create(&thread_id, NULL, handle_request, client_socket_ptr);
            sleep(1); // otherwise, the thread doesn’t have time to start before exit…
            if(pthread != 0) {
                printf("pthread creation fail\n");
                close(new_fd);
                free(client_socket_ptr);
            }
            pthread_join(thread_id, NULL); //finish thread
        }
    }
    close(server_socket);
    return 0;
}