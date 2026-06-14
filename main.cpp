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
#include <unordered_map>
#include <sstream>
#include <fcntl.h>

using namespace std;

struct HttpRequest{
    string method;
    string path;
    string version;
    unordered_map <string , string> header;
};

HttpRequest parseRequest(const string &request){
    HttpRequest req;

    stringstream ss(request);
    string token;
    vector <string> headers;
    while(getline(ss , token , '\n')){
        if(!token.empty() && token.back() == '\r'){
            token.pop_back();
        }
        headers.push_back(token);
    }

    if(headers.empty()){
        cerr << "empty request\n";
        return{};
    }
    
    stringstream request_line(headers[0]);
    vector <string> tokens;
    while(getline(request_line , token , ' ')){
        tokens.push_back(token);
    }
    if(tokens.size() < 3){
        cerr << "invalid request";
        return{};
    }
    req.method = tokens[0];
    req.path = tokens[1];
    req.version = tokens[2];

    headers.erase(headers.begin());
    
    for(const string& s : headers){
        if(s.empty())continue;
        size_t key_index = s.find(':');
        if(key_index == string::npos)continue;
        string key = s.substr(0 , key_index);
        req.header[key] = s.substr(key_index + 2);
    }
    cout << "PATH: [" << req.path << "]\n";
    return req;
}

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

        string raw_request(buffer);

        HttpRequest request = parseRequest(raw_request);

        string body;
        string status;
        string content_type = "text/plain";
        

        if(request.path == "/"){
            body = "home page";
            status = "HTTP/1.1 200 OK";
        }
        else if(request.path == "/hello"){
            body = "hello vishesh";
            status = "HTTP/1.1 200 OK";
        }
        else if(request.path.find("/echo/") == 0){
            body = request.path.substr(6);
            status = "HTTP/1.1 200 OK";
        }
        else if(request.path.find("/user-agent") == 0){
            body = request.header["User-Agent"];
            status = "HTTP/1.1 200 OK";
        }
        else if(request.path.find("/files/") == 0){
            string filename = request.path.substr(7);
            string filepath = "files/" + filename;
            int fd = open(filepath.c_str() , O_RDONLY);
            if(fd == -1){
                body = "file not found";
                status = "HTTP/1.1 404 Not Found";
            }
            else{
                char buffer[1024];
                ssize_t bytes_read;
                body = "";
                status = "HTTP/1.1 200 OK";
                while(true){
                    if((bytes_read = read(fd , buffer , sizeof(buffer)))<0){
                        perror("read");
                        body = "read failed";
                        status = "HTTP/1.1 500 Internal Server Error";
                        break;
                    }
                    else if(bytes_read == 0){
                        break;
                    }
                    else{
                        body += string(buffer , bytes_read);
                    }
                }
                close(fd);
            }
        }
        else{
            body = "page not found";
            status = "HTTP/1.1 404 Not Found";
        }

        size_t content_len = body.size();

        string response = status + "\r\n" +
                         "content-type: " + content_type +"\r\n" +
                         "content-length: " + to_string(content_len) + "\r\n\r\n"+
                         body;
        
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







