<h1 align="center">
  <a href="https://libwebtransport.example"><img src="./.github/Screenshot 2025-03-17 012510.png" alt="libwebTransport" height="150px"></a>
  <br>
  libwebtransport
  <br>
</h1>
<h4 align="center">A pure C/C++ implementation of the WebTransport API leveraging QUIC and HTTP/3</h4>
<p align="center">
    <a href="https://github.com/deep-neural/libwebtransport"><img src="https://img.shields.io/badge/libwebTransport-C/C++-blue.svg?longCache=true" alt="libwebTransport" /></a>
  <a href="https://datatracker.ietf.org/doc/html/rfc9000"><img src="https://img.shields.io/static/v1?label=RFC&message=9000&color=brightgreen" /></a>
  <a href="https://datatracker.ietf.org/doc/html/rfc9001"><img src="https://img.shields.io/static/v1?label=RFC&message=9001&color=brightgreen" /></a>
  <a href="https://datatracker.ietf.org/doc/html/rfc9002"><img src="https://img.shields.io/static/v1?label=RFC&message=9002&color=brightgreen" /></a>
  <a href="https://datatracker.ietf.org/doc/html/rfc9114"><img src="https://img.shields.io/static/v1?label=RFC&message=9114&color=brightgreen" /></a>
  <br>
    <a href="https://github.com/deep-neural/libwebtransport"><img src="https://img.shields.io/static/v1?label=Build&message=Documentation&color=brightgreen" /></a>
    <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-5865F2.svg" alt="License: MIT" /></a>
</p>
<br>

### New Release

libwebTransport v1.0.0 has been released! See the [release notes](https://github.com/deep-neural/libwebtransport/) to learn about new features, enhancements, and breaking changes.

If you aren’t ready to upgrade yet, check the [tags](https://github.com/deep-neural/libwebtransport) for previous stable releases.

We appreciate your feedback! Feel free to open GitHub issues or submit changes to stay updated in development and connect with the maintainers.

-----

### Usage

libwebtransport is distributed as a pure C/C++ library. To integrate it into your project, ensure you have a compatible C/C++ compiler and the necessary build tools (e.g., Make, CMake). Clone the repository and link against the library in your build system.

## Simple API
<table>
<tr>
<th> Server </th>
<th> Client </th>
</tr>
<tr>
<td>

```cpp
#include <web_transport.h>
#include <iostream>

int main() {
    web_transport::Server server("0.0.0.0", 443);
    
    server.setCertFile("./publickey.pem");
    server.setKeyFile("./privatekey.pem");
    
    server.onSession([](void* session_ptr, const std::string& path) {

        auto* session = static_cast<web_transport::ServerSession*>(session_ptr);
        std::cout << "New session on path: " << path << std::endl;

        session->onDatagramRead([session](const std::vector<uint8_t>& data) {
            std::cout << "onDatagramRead: " << data.data() << std::endl;
        });

        return true;
    });
    
    server.initialize();
    server.listen();
    
    return 0;
}
```

</td>
<td>

```cpp
#include <web_transport.h>

int main() {
    web_transport::Client client("https://example.com/path");
    
    client.setPublicKey("./publickey.pem");
    
    client.onSessionOpen([](void* session_ptr) {
        auto* session = static_cast<web_transport::ClientSession*>(session_ptr);
        std::cout << "Session opened!" << std::endl;
        
        // Send a datagram
        session->sendDatagram({1, 2, 3, 4});
    });
    
    client.connect();
    client.runEventLoop();
    
    return 0;
}
```

</td>
</tr>
</table>

**[Example Applications](examples/README.md)** contain code samples demonstrating common use cases with libwebTransport.

**[API Documentation](https://libwebtransport.example/docs)** provides a comprehensive reference of our Public APIs.

Now go build something amazing! Here are some ideas to spark your creativity:
* Transfer large files in real-time with QUIC’s low latency and stream multiplexing.
* Develop a real-time multiplayer game server with ultra-responsive data channels.
* Create interactive live-streaming applications featuring dynamic data exchange.
* Implement low-latency remote control and telemetry for embedded and IoT devices.
* Integrate server push and bidirectional streams for cutting-edge web applications.

## Building

See [BUILDING.md](https://github.com/danielv4/libwebtransport/blob/master/BUILDING.md) for building instructions.

### Features

#### WebTransport API
* Pure C/C++ implementation of the emerging [WebTransport](https://www.w3.org/TR/webtransport/) API for bidirectional data streams.
* Supports both reliable and unreliable data delivery modes.
* Enables server push, stream multiplexing, and efficient session management.

#### QUIC & HTTP/3 Powered Connectivity
* Built on QUIC—harnessing features like 0-RTT connection establishment and connection migration.
* Leverages HTTP/3 for reduced latency, improved congestion control, and robust performance.
* Multiplexed streams allow concurrent data transfers without head-of-line blocking.

#### Data Streams
* Bidirectional and unidirectional streams for flexible data transfer.
* Offers ordered and unordered delivery options to suit various application needs.
* Customizable stream priorities and flow control mechanisms.

#### Security
* Utilizes TLS 1.3 integrated within QUIC for state-of-the-art encryption.
* Provides end-to-end secure data channels with advanced protection against network threats.

#### Pure C/C++
* Written entirely in C/C++ with no external dependencies beyond standard libraries.
* Wide platform support: Windows, macOS, Linux, FreeBSD, and more.
* Optimized for high performance with fast builds and a comprehensive test suite.
* Easily integrated into existing projects using common build systems.

### Contributing

Check out the [contributing guide](https://github.com/deep-neural/libwebtransport/wiki/Contributing) to join the team of dedicated contributors making this project possible.

### License

MIT License - see [LICENSE](LICENSE) for full text
