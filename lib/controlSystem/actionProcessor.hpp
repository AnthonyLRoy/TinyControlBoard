#include "serial.hpp"
#include "spi.hpp"
#include "relay.hpp"
#include "actionsResponse.hpp"

namespace controlSystem
{
    class actionProcessor
    {
    private:
        /* data */
    public:
        actionProcessor(serialBus::Serial &serialBusRef, relays::StandardRelay& relaysRef, spibus::SPI &spiRef);
        void process(actions::actionResponse response);
    private:
        serialBus::Serial& serial;
        relays::StandardRelay& relays;
        spibus::SPI& spiBus;
    };
}