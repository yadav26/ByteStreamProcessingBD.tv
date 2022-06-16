
**ByteStreamProcessingBD.tv**
BirdDog.tv

Solution is designed considering the single producer and multiple consumer problem.

Though, a binary file is provided as input to read, in practical life data can be live stream of packets.

class Packet is designed with properties of the packet as described (1 byte, 1 byte, 2 bytes, 4 bytes)

- Thread thReader addresses the memory read of the input file and stream 'n' packet at once to workers for processing ( sorting in this case )
- Thread thEvenWriter process the even queue when signaled and once processed return control to thReader.
- Thread thOddWriter - process the odd queue when signaled and once processed return control to thReader.

**Performance related**
- Using List instead of vector as sorting a list is faster just redirect the pointers
- MAX_PACKETS_PROCESS - reader to workers load execution is handled. Possible minimum of 1 execution path of reader to 1 worker 

**C++ features used**
1. Multithreading
2. Guarding
3. Signaling of thread execution by using conditional variable wait and notify_all
4. auto
5. stl algorithms - copy and sort
6. fstream reading and file writing
7. using
8. move
9. inline default member variable initializaton
10. Runtime assert
11. namespace