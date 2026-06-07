#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

inline void handleConn(int conn_sock) {
  char buffer[1024];
  // Edge-triggered: this event is our only notification for everything
  // currently buffered, so keep reading until the socket is drained.
  while (true) {
    ssize_t n = recv(conn_sock, buffer, sizeof(buffer), 0);

    if (n > 0) {
      std::cout.write(buffer, n) << std::flush;
      send(conn_sock, "+PONG\r\n", 7, 0);
      continue; // there may be more buffered — keep draining
    }

    if (n == 0) {       // peer closed the connection
      close(conn_sock); // close also removes it from the epoll set
      return;
    }

    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return; // fully drained — wait for the next edge
    }

    std::cerr << "recv failed" << std::endl;
    close(conn_sock);
    return;
  }
}
int main() {
  constexpr int MAX_EVENTS = 20;
  auto server_socket = socket(AF_INET, SOCK_STREAM, 0);

  if (server_socket < 0) {
    std::cout << "Creating socket failed" << std::endl;
    return 1;
  }

  int reuse = 1;
  if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse,
                 sizeof(reuse)) < 0) {
    std::cerr << "setsockopt failed\n";
    return 1;
  }

  sockaddr_in server_address;

  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(6379);
  server_address.sin_addr.s_addr = INADDR_ANY;

  auto binded = bind(server_socket, (struct sockaddr *)&server_address,
                     sizeof(server_address));
  if (binded < 0) {
    std::cout << "Binding to socket port failed" << std::endl;
    return 1;
  }

  auto listened = listen(server_socket, 5);
  if (listened < 0) {
    std::cout << "listning on socket failed" << std::endl;
    return 1;
  }

  int epfd = epoll_create1(EPOLL_CLOEXEC);
  if (epfd == -1) {
    std::cerr << "epoll creation failed";
    return 1;
  }

  epoll_event ev{}, events[MAX_EVENTS];
  ev.events = EPOLLIN;
  ev.data.fd = server_socket;

  if (epoll_ctl(epfd, EPOLL_CTL_ADD, server_socket, &ev) == -1) {
    std::cerr << "epoll_ctl failed" << std::endl;
    return 1;
  }

  std::cout << "listening on server" << std::endl;

  while (true) {
    int nfds = epoll_wait(epfd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      if (errno == EINTR) {
        continue; // interrupted by a signal (debugger, etc.) — not an error
      }
      std::cerr << ("epoll_wait") << std::endl;
      return 1;
    }

    for (int n = 0; n < nfds; ++n) {
      if (events[n].data.fd == server_socket) {
        sockaddr conn_addr;
        socklen_t addrlen = sizeof(conn_addr);

        int conn_sock = accept(server_socket, &conn_addr, &addrlen);
        if (conn_sock == -1) {
          std::cerr << "accept client failed: " << conn_addr.sa_data
                    << std::endl;
        }

        int fcntl_flags = fcntl(conn_sock, F_GETFL, 0);
        if (fcntl_flags == -1) {
          std::cerr << "fcntl F_GETFL failed" << std::endl;
          return 1;
        }

        if (fcntl(conn_sock, F_SETFL, fcntl_flags | O_NONBLOCK) == -1) {
          std::cerr << "fcntl F_SETFL failed" << std::endl;
          return 1;
        }
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = conn_sock;
        if (epoll_ctl(epfd, EPOLL_CTL_ADD, conn_sock, &ev) == -1) {
          std::cerr << "epoll_ctl: conn_sock" << std::endl;
          return 1;
        }
      } else {
        handleConn(events[n].data.fd);
      }
    }
  }
}
