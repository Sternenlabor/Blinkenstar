#include "DebugSerial.h"

/**
 * Compile-time smoke test for the JP1 debug serial API surface.
 *
 * @returns Zero after exercising the available debug logging entry points.
 */
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
