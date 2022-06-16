// ByteStreamProcessor.cpp : 
//

#include <fstream>
#include <iostream>
#include <vector>
#include <list>
#include <thread>
#include <mutex>
#include <algorithm>
#include <condition_variable>
#include "Packet.h"

using namespace BirdDog::Packet;



using BYTE = unsigned char;
using namespace std;
constexpr long MAX_PACKETS_PROCESS = 10; // for 10 reader path is to 1 worker load processing

std::condition_variable cv;
std::mutex m;
bool readyEven = false;
bool readyOdd = false;
bool processedEven = false;
bool processedOdd = false;
bool finished = false;

//Anshul - Note
//Using list as it is faster then vector for sorting
std::list<uint32_t> evenQueue;
std::list<uint32_t> oddQueue;


//History of packet collection 
//no other purpose then debugging
vector<Packet> packets;

//Thread call back address the memory read of the input file
//stream 'n' packet at once to workers for processing ( sorting in this case)
void cbReader(string task3binfileppath)
{
    std::cout << "Reader enter\n";
    {
        std::lock_guard lk(m);
    }
    //minimum 1 packet to process
    uint32_t max_packets_to_process = MAX_PACKETS_PROCESS > 0 ? MAX_PACKETS_PROCESS : 1;

    std::streampos task3binSize;
    std::ifstream task3binfile(task3binfileppath, std::ios::binary);

    // start and end position
    task3binfile.seekg(0, std::ios::end);
    task3binSize = task3binfile.tellg();
    task3binfile.seekg(0, std::ios::beg);

    std::vector<BYTE> vMemoryData(task3binSize);
    //1. Read the file onto memory.
    task3binfile.read((char*)&vMemoryData[0], task3binSize);
    task3binfile.close();

    //Anshul - Special case if file is truncated or not a multiple of 8
    //then we need to validate the very last read as the remaining size

    auto packetsToProcess = 0;

    for (auto i = 0; i < vMemoryData.size()/8;  )
    {
        std::cout << "Reader - processing\n";
        Packet p;
        vector<BYTE> vtemp; vtemp.reserve(8);
        auto itr = vMemoryData.begin() + (i * 8);
        std::copy(itr, itr+8, back_inserter(vtemp));
        //move the temporary
        p.generatePacketFromByteStream(std::move(vtemp), evenQueue, oddQueue);
        packets.emplace_back(p);
        i += 8;

        ++packetsToProcess;
        //performance tweak - could be more then one packet to process at one time
        //reducing burden on workers
        if (packetsToProcess < max_packets_to_process && i < vMemoryData.size() / 8)
            continue;

        packetsToProcess = 0;
        readyEven = true;
        readyOdd = true;
        cv.notify_all();
        //std::cout << "Reader - waiting for workers\n";
        // wait for the worker
        {
            std::unique_lock ulk(m);
            cv.wait(ulk, [] {return processedOdd && processedEven; });
            processedOdd = processedEven = false;
        }
        //std::cout << "Reader - waitover for workers\n";
    }

    //sleep to give workers enough window to finish
    finished = true;
    readyEven = true;
    readyOdd = true;
    cv.notify_all();
    std::cout << "Reader exiting\n";
}

/// <summary>
/// This thread will gain control from Reader/Producer - then process data(sort the queue)
/// and return the control to Reader for more data
/// </summary>
void cbEvenWriter()
{
    std::cout << "EvenWriter enter\n";
    //Wait reader to finish reading and processing data
    while (!finished)
    {
        //std::cout << "EvenWriter waiting\n";

        std::unique_lock lk(m);
        cv.wait(lk, [] {return readyEven; });
        readyEven = false;

        std::cout << "EvenWriter - Processing\n";

        /// 2. Writes the numbers in two different files in the following format:
        //  a.First file will contain all the even numbers in ascending order

        //SORT the even queue using stl algorithm - now lambdas can take auto
        //std::sort(evenQueue.begin(), evenQueue.end(),
        //    [](const auto& a, const auto& b) { return a < b; });
        evenQueue.sort();

        processedEven = true;
        lk.unlock();
        cv.notify_all();
        //std::cout << "EvenWriter processedEven, notify_all\n";
    }
    std::cout << "EvenWriter exit\n";
}

/// <summary>
/// This thread will gain control from Reader/Producer - then process data
/// and return the control to Reader for more data
/// </summary>
void cbOddWriter()
{
    std::cout << "OddWriter enter\n";
    //Wait reader to finish reading and processing data
    while (!finished) 
    {
        //std::cout << "OddWriter waiting\n";

        std::unique_lock lk(m);
        cv.wait(lk, [] {return readyOdd; });
        readyOdd = false;

        std::cout << "OddWriter - Processing\n";

        /// 2. Writes the numbers in two different files in the following format:
        //  b.Second file will contain all the odd numbers in descending order

        // removed the type to list as sorting list is faster
        // following commented code is for vectors
        //std::sort(oddQueue.begin(), oddQueue.end(),
        //    [](const auto& a, const auto& b) { return a > b; });
        oddQueue.sort();

        processedOdd = true;
        lk.unlock();
        cv.notify_all();
        //std::cout << "EvenWriter processedOdd, notify_all\n";
    }
    std::cout << "OddWriter exit\n";
}

void flushEvenData()
{
    fstream feven;

    feven.open("even.bin", std::fstream::out | std::fstream::trunc);

    for (auto& v : evenQueue)
        feven << std::hex << v << std::endl;

    feven.close();
}

void flushOddData()
{
    fstream fodd;

    fodd.open("odd.bin", std::fstream::out | std::fstream::trunc);

    for (auto it = oddQueue.rbegin(); it != oddQueue.rend(); ++it)
        fodd << std::hex << *it << std::endl;

    fodd.close();
    std::cout << "OddWriter exit\n";
}

/// <summary>
/// Anshul - It is a single producer and multiple consumer problem
/// Assuming Reader is receiving continous stream of 86240 bytes or 64 bytes(1-packet) in this case to process each time
/// or could be a live stream
/// 
/// packets - for debugging purpose (history)
/// 
/// Reader will parse single packet 64 bytes at one time
/// Reader will signal worker threads to process the queue ( which is even/odd sorting in this case)
/// This has been designed as such to consider a continous stream of packet/s, 1 or MAX_PACKETS_PROCESS packets can be processed at any instance(line#108)
/// </summary>
/// <param name="argc"></param>
/// <param name="argv"></param>
/// <returns></returns>
int main(int argc, char* argv[])
{
    if (argc < 2)
    {
        cout << "Please enter input binary filename path.";
        return 0;
    }

    //Reader thread
    std::thread thReader(cbReader, argv[1]);

    //process even number queue writing
    std::thread thEvenWriter(cbEvenWriter);

    //process odd number queue writing
    std::thread thOddWriter(cbOddWriter);

    thReader.join();
    thEvenWriter.join();
    thOddWriter.join();
    
    //Dumping into file kept as separate as this is the final or timely bound outcome for continous stream.
    flushEvenData();
    flushOddData();

    return 0;
}
