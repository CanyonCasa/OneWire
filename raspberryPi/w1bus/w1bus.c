//#include <napi.h>
#include "w1io.h"

napi_value Init(napi_env env, napi_value exports) {
  return W1io::Init(env, exports);
}

NODE_API_MODULE(w1bus, Init)
