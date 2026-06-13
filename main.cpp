#include <iostream>
#include <cstdlib>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <vector>

using namespace std;

int main(){
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        perror("socket");
        return 1;  
    }
    
    struct sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if(bind(server_fd , (sockaddr*)&server_addr , sizeof(server_addr))<0){
        perror("bind");
        return 1;
    }

    int connection_backlog = 5;
    if(listen(server_fd, connection_backlog) < 0){
        perror("listen");
        return 1;
    }

    
    while(true){
        struct sockaddr_in client_addr{};
        socklen_t client_addr_len = sizeof(client_addr);
        cout <<"waiting for client to connect...\n";

        int client_fd;
        if((client_fd = accept(server_fd , (sockaddr*)&client_addr , &client_addr_len ))<0){
            perror("accept");
            continue;
        }

        cout<< "client connected...\n";

        char buffer[1024];
        int bytes_received;

        if((bytes_received = recv(client_fd , buffer , sizeof(buffer)-1 ,0))<0){
            perror("recv");
            close(client_fd);
            continue;
        }
        
        if(bytes_received == 0){
            cout << "client disconnected...\n";
            close(client_fd);
            continue;
        }
        
        buffer[bytes_received] = '\0';
        size_t request_index;
        string request(buffer);
        if((request_index = request.find("\r\n"))!= string::npos){
            request = request.substr(0 , request_index);
        }
        else{
            cerr << "invalid HTTP request...";
            close(client_fd);
            continue;
        }

        string request_parameters[3] = {"", "" , ""};

        string* request_ptr = request_parameters;
        for(size_t i=0 ; i<request.size() ; i++){
            if(request[i] == ' '){
                if(request_ptr != &request_parameters[2]){
                    request_ptr++;
                }
                continue;
            }
            *request_ptr += request[i];
        }

        string method = request_parameters[0];
        string path = request_parameters[1];
        string version = request_parameters[2];

        cout<<method<<"\n" << path <<"\n" << version << "\n";

        string body;
        string status;

        if(path == "/"){
            body = "home page";
            status = "HTTP/1.1 200 OK";
        }
        else if(path == "/hello"){
            body = "hello vishesh";
            status = "HTTP/1.1 200 OK";
        }
        else{
            body = "page not found";
            status = "HTTP/1.1 404 Not Found";
        }

        string response = status + "\r\n\r\n" + body +"\n";
        
        int bytes_sent;                  
        if((bytes_sent = send(client_fd , response.c_str() , response.size() , 0))<=0){
            perror("send");
            close(client_fd);
            continue;
        }


        close(client_fd);
    }
    return 0;
}