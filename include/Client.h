#ifndef WEBSERVER_CLIENT_H
#define WEBSERVER_CLIENT_H

#include <source_location>
#include <string>

#include "Buffer.h"

class Client {
 public:
  Client(int fileDescriptor, std::string information,
         unsigned short timeout = 60,
         const std::source_location& sourceLocation =
             std::source_location::current());

  Client(const Client& client) = delete;

  Client(Client&& client) noexcept;

  auto operator=(Client&& client) noexcept -> Client&;

  auto receive(const std::source_location& sourceLocation =
                   std::source_location::current()) -> void;

  auto send(const std::source_location& sourceLocation =
                std::source_location::current()) -> void;

  [[nodiscard]] auto read() -> std::string;

  auto write(const std::string_view& data) -> void;

  [[nodiscard]] auto get() const -> int;

  [[nodiscard]] auto getEvent() const -> unsigned int;

  [[nodiscard]] auto getTimeout() const -> unsigned short;

  [[nodiscard]] auto getInformation() const -> std::string_view;

  auto setKeepAlive(bool value) -> void;

  ~Client();

 private:
  int socket;
  unsigned int event;
  unsigned short timeout;
  bool keepAlive;
  std::string information;
  Buffer sendBuffer, receiveBuffer;
};

#endif  //WEBSERVER_CLIENT_H
