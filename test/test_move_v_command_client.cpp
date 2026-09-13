#include "message.pb.h"

#include <zmq.hpp>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    std::string endpoint{"tcp://127.0.0.1:5555"};
    int durationMs{2000};
    bool execute{false};
    std::vector<double> velocities;
};

void printUsage(const char* program)
{
    std::cout
        << "Usage: " << program
        << " --execute [--endpoint URL] [--duration-ms N]"
           " v1 [v2 ... vN]\n\n"
        << "WARNING: this sends real motion commands to the configured NRT server.\n"
        << "Example for a 7-axis model:\n  " << program
        << " --execute 0.05 0 0 0 0 0 0\n";
}

int parsePositiveInt(const std::string& value, const char* option)
{
    size_t used = 0;
    const int parsed = std::stoi(value, &used);
    if (used != value.size() || parsed <= 0)
        throw std::invalid_argument(std::string(option) + " must be positive");
    return parsed;
}

Options parseOptions(int argc, char* argv[])
{
    Options options;
    for (int i = 1; i < argc; ++i)
    {
        const std::string arg = argv[i];
        if (arg == "--execute")
        {
            options.execute = true;
        }
        else if (arg == "--endpoint")
        {
            if (++i >= argc)
                throw std::invalid_argument("--endpoint requires a value");
            options.endpoint = argv[i];
        }
        else if (arg == "--duration-ms")
        {
            if (++i >= argc)
                throw std::invalid_argument("--duration-ms requires a value");
            options.durationMs = parsePositiveInt(argv[i], "--duration-ms");
        }
        else if (arg == "--help" || arg == "-h")
        {
            printUsage(argv[0]);
            std::exit(0);
        }
        else
        {
            size_t used = 0;
            const double velocity = std::stod(arg, &used);
            if (used != arg.size() || !std::isfinite(velocity))
                throw std::invalid_argument("velocities must be finite numbers");
            options.velocities.push_back(velocity);
        }
    }

    if (!options.execute)
        throw std::invalid_argument("--execute is required");
    if (options.velocities.empty() || options.velocities.size() > 24)
        throw std::invalid_argument("provide 1..24 joint velocities");
    return options;
}

std::string sendCommand(zmq::socket_t& socket,
                        const std::string& name,
                        const std::vector<double>& args)
{
    zrcs_message::MotionCommand command;
    command.set_command(name);
    for (double value : args)
        command.add_args(value);

    std::string payload;
    if (!command.SerializeToString(&payload))
        throw std::runtime_error("failed to serialize " + name);

    zmq::message_t request(payload.size());
    std::memcpy(request.data(), payload.data(), payload.size());
    if (!socket.send(request, zmq::send_flags::none))
        throw std::runtime_error("failed to send " + name);

    zmq::message_t response;
    if (!socket.recv(response, zmq::recv_flags::none))
        throw std::runtime_error("timed out waiting for " + name + " reply");

    return {static_cast<const char*>(response.data()), response.size()};
}

void requireOk(zmq::socket_t& socket,
               const std::string& name,
               const std::vector<double>& args)
{
    const std::string reply = sendCommand(socket, name, args);
    std::cout << name << " -> " << reply << '\n';
    if (reply != "OK")
        throw std::runtime_error(name + " was rejected: " + reply);
}

void bestEffortStop(const std::string& endpoint) noexcept
{
    try
    {
        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::req);
        socket.set(zmq::sockopt::rcvtimeo, 1000);
        socket.set(zmq::sockopt::sndtimeo, 1000);
        socket.set(zmq::sockopt::linger, 0);
        socket.connect(endpoint);
        const std::string reply = sendCommand(socket, "SYS_STOP", {});
        std::cerr << "cleanup SYS_STOP -> " << reply << '\n';
    }
    catch (...)
    {
        std::cerr << "cleanup SYS_STOP could not be delivered\n";
    }
}

} // namespace

int main(int argc, char* argv[])
{
    Options options;
    bool moveStarted = false;
    try
    {
        options = parseOptions(argc, argv);

        zmq::context_t context(1);
        zmq::socket_t socket(context, zmq::socket_type::req);
        socket.set(zmq::sockopt::rcvtimeo, 3000);
        socket.set(zmq::sockopt::sndtimeo, 3000);
        socket.set(zmq::sockopt::linger, 0);
        socket.connect(options.endpoint);

        std::vector<double> startArgs;
        startArgs.reserve(options.velocities.size() + 1);
        startArgs.push_back(static_cast<double>(options.velocities.size()));
        startArgs.insert(startArgs.end(), options.velocities.begin(),
                         options.velocities.end());
        requireOk(socket, "MoveV", startArgs);
        moveStarted = true;

        std::this_thread::sleep_for(
            std::chrono::milliseconds(options.durationMs));

        requireOk(socket, "SYS_STOP", {});
        moveStarted = false;
        std::cout << "MoveV command test completed.\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "MoveV command test failed: " << error.what() << '\n';
        if (moveStarted)
            bestEffortStop(options.endpoint);
        printUsage(argv[0]);
        return 1;
    }
}
