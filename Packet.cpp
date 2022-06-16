#include "Packet.h"
#include <cassert>

using namespace BirdDog::Packet;
//Following will parse the stream into bytes
void Packet::generatePacketFromByteStream(std::vector<BYTE>&& v, std::list<uint32_t>& evenQueue, std::list<uint32_t>& oddQueue)
{
    std::cout << v.size() << std::endl;
    assert(v.size()%8 == 0, "packet size not supported.");

    //assuming we recieve vector of size 8
    //read 1 byte then, 1 byte then 2 bytes and finally last 4 bytes
    first = v[0];
    if (0 == first % 2)
        evenQueue.emplace_back(first);
    else
        oddQueue.emplace_back(first);

    second = v[1];
    if (0 == second % 2)
        evenQueue.emplace_back(second);
    else
        oddQueue.emplace_back(second);

    third = v[2] << 8 | v[3];
    if (0 == third % 2)
        evenQueue.emplace_back(third);
    else
        oddQueue.emplace_back(third);

    fourth = v[4] << 24 | v[5] << 16 | v[6] << 8 | v[7];
    if (0 == fourth % 2)
        evenQueue.emplace_back(fourth);
    else
        oddQueue.emplace_back(fourth);
}
   