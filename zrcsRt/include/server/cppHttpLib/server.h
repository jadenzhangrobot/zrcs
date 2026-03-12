#include <iostream>
#include <string>
#include <httplib.h>

using namespace httplib;
using namespace std;

class WebServer {
private:
    Server server;
    string staticDir;
    int webport;

public:
    WebServer(const string& dir,int port) : staticDir(dir),webport(port)
    {

        server.set_base_dir(staticDir);

        // API endpoint
        server.Get("/api/hello", [](const Request& req, Response& res) {
            res.set_content("{\"message\":\"Hello from C++ server\"}", "application/json");
        });
        if (server.listen("0.0.0.0", port)==false)
        {
            throw std::runtime_error("Failed to start server");
        }
    }
    ~WebServer()
    {
        server.stop();
    }
};


