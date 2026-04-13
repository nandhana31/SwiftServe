#include <iostream>
#include <cstring>
#include <unistd.h>
#include <thread>
#include <sstream>  
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <vector>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ctime>

using namespace std;

// Shared queue
queue<int> client_queue;
mutex mtx;
condition_variable cv;

// Logging mutex
mutex log_mtx;

// 📄 Read file
string read_file(const string& path) {
    ifstream file(path, ios::binary);
    if (!file.is_open()) return "";

    string content((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return content;
}

// 🧠 MIME type detection
string get_mime_type(const string& path) {
    if (path.find(".html") != string::npos) return "text/html";
    if (path.find(".css") != string::npos) return "text/css";
    if (path.find(".js") != string::npos) return "application/javascript";
    if (path.find(".json") != string::npos) return "application/json";
    if (path.find(".png") != string::npos) return "image/png";
    if (path.find(".jpg") != string::npos) return "image/jpeg";
    return "text/plain";
}

// Thread-safe logging
void log_request(const string& method, const string& path) {
    lock_guard<mutex> lock(log_mtx);

    ofstream log_file("logs.txt", ios::app);

    time_t now = time(0);
    string dt = ctime(&now);
    dt.pop_back();

    log_file << "[" << dt << "] " << method << " " << path << endl;
}

// Handle client
void handle_client(int client_socket) {
    char buffer[30000] = {0};
    read(client_socket, buffer, 30000);

    string request(buffer);

    if (request.empty()) {
        close(client_socket);
        return;
    }

    // Ignore favicon
    if (request.find("favicon.ico") != string::npos) {
        close(client_socket);
        return;
    }

    // Parse first line
    string first_line = request.substr(0, request.find("\r\n"));

    string method, path;
    stringstream ss(first_line);
    ss >> method >> path;

    cout << method << " " << path << endl;

    // 📝 Log request
    log_request(method, path);

    string response_body;
    string status_line;
    string content_type = "text/html";

    // Routing + static files
    if (path == "/") {
        path = "/index.html";
    }

    if (path == "/api") {
        status_line = "HTTP/1.1 200 OK\r\n";
        content_type = "application/json";
        response_body = "{\"message\": \"Hello from API 🚀\"}";
    } else {
        string file_path = "public" + path;
        response_body = read_file(file_path);

        if (response_body.empty()) {
            status_line = "HTTP/1.1 404 Not Found\r\n";
            response_body = "<html><body><h1>404 Not Found</h1></body></html>";
        } else {
            status_line = "HTTP/1.1 200 OK\r\n";
            content_type = get_mime_type(file_path);
        }
    }

    // ✅ Content-Length added
    string response =
        status_line +
        "Content-Type: " + content_type + "\r\n" +
        "Content-Length: " + to_string(response_body.size()) + "\r\n" +
        "Connection: close\r\n\r\n" +
        response_body;

    send(client_socket, response.c_str(), response.size(), 0);

    close(client_socket);
}

// Worker threads
void worker() {
    while (true) {
        int client_socket;

        {
            unique_lock<mutex> lock(mtx);
            cv.wait(lock, [] { return !client_queue.empty(); });

            client_socket = client_queue.front();
            client_queue.pop();
        }

        handle_client(client_socket);
    }
}

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (server_fd == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    cout << "Server running on http://localhost:8080" << endl;

    // Thread pool
    const int THREAD_COUNT = 4;
    vector<thread> pool;

    for (int i = 0; i < THREAD_COUNT; i++) {
        pool.emplace_back(worker);
    }

    // Accept loop
    while (true) {
        int new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen);

        if (new_socket < 0) {
            perror("Accept failed");
            continue;
        }

        {
            lock_guard<mutex> lock(mtx);
            client_queue.push(new_socket);
        }

        cv.notify_one();
    }

    close(server_fd);
    return 0;
}