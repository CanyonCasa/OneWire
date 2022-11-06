#include <napi.h>
#include "w1io.hpp"

Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
  return W1io::Init(env, exports);
}

NODE_API_MODULE(w1bus, InitAll)
