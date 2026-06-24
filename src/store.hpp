#pragma once

#include <string>
#include <unordered_map>

using namespace std;

class Store {
  unordered_map<string, char *> kv;
};
