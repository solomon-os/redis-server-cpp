
#include "server.hpp"

int main() {
  Handler handler;
  Server server(handler);

  server.listen();
}
