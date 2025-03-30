#include <iostream>
#include <string>
#include <httplib.h>

using namespace httplib;
using namespace std;

class WebServer {
private:
    Server server;
    string staticDir;

public:
    WebServer(const string& dir) : staticDir(dir) {}

    void setupRoutes() {
        // Serve static files
        server.set_base_dir(staticDir);

        // API endpoint
        server.Get("/api/hello", [](const Request& req, Response& res) {
            res.set_content("{\"message\":\"Hello from C++ server\"}", "application/json");
        });
    }

    void start(int port) {
        cout << "Server running at http://localhost:" << port << endl;
        server.listen("0.0.0.0", port);
    }
    void stop() {
        server.stop();
    }
};


