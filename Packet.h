#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <algorithm>
#include <condition_variable>

namespace BirdDog::Packet{
    
    using BYTE = unsigned char;

    class Packet {
        uint8_t first = 0;
        uint8_t second = 0;
        uint16_t third = 0;
        uint32_t fourth = 0;

    public:
        void generatePacketFromByteStream(std::vector<BYTE>&& v, std::list<uint32_t>& , std::list<uint32_t>&);
    };
}