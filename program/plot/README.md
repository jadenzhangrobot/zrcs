# ZMQ JSON Publisher for PlotJuggler

This project demonstrates how to send JSON data over ZeroMQ to be visualized in PlotJuggler.

## Dependencies

- C++17 compiler
- CMake (>= 3.10)
- ZeroMQ
- nlohmann_json

## Build

1.  Create a build directory:
    ```bash
    mkdir build
    cd build
    ```

2.  Run CMake:
    ```bash
    cmake ..
    ```

3.  Compile the project:
    ```bash
    make
    ```

## Run

```bash
./publisher
```

## PlotJuggler

1.  Start PlotJuggler.
2.  From the "Streaming" menu, select "ZMQ Subscriber".
3.  Enter the address `tcp://localhost:5556`.
4.  The data should appear in the timeseries list.