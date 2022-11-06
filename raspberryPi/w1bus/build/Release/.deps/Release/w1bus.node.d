cmd_Release/w1bus.node := ln -f "Release/obj.target/w1bus.node" "Release/w1bus.node" 2>/dev/null || (rm -rf "Release/w1bus.node" && cp -af "Release/obj.target/w1bus.node" "Release/w1bus.node")
