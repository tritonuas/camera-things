#include "mavlink.cpp"
#include "simple-cam.cpp"
#include "mavlink.hpp"
#include "obc_port.hpp"


int main() {

    OBCPort obcPort; 

    obcPort.start();
}

