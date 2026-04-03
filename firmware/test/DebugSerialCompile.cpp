#include "DebugSerial.h"

int main()
{
    debuglog::begin();
    debuglog::print("JP1");
    debuglog::println(" DEBUG");
    debuglog::printlnHex8(0x5A);
    debuglog::heartbeat();
    debuglog::write('\n');
    return 0;
}
