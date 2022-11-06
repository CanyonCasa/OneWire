{
  "targets": [
    {
      "target_name": "w1bus",
      "cflags!": [ "-fno-exceptions" ],
      "cflags_cc!": [ "-fno-exceptions" ],
      "sources": [ "w1bus.cpp", "w1io.cpp" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "/usr/local/include"
      ],
      "libraries": [
        "/usr/local/lib/libgpiodcxx.so"
     ],
      'defines': [ 'NAPI_DISABLE_CPP_EXCEPTIONS' ],
    }
  ]
}
