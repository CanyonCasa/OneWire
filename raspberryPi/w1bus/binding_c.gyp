{
  "targets": [
    {
      "target_name": "w1bus",
      "sources": [ "w1bus.c", "w1io.c" ],
      "include_dirs": [
        "<!@(node -p \"require('node-addon-api').include\")",
        "/usr/local/include"
      ],
      "libraries": [
        "/usr/local/lib/libgpiod.so"
     ]
    }
  ]
}
