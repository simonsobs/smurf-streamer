#ifndef _DoubleQueue_H
#define _DoubleQueue_H

#include <mutex>
#include <queue>
#include <thread>

/*
 * A double buffer queue class to handle incoming data packets
 */
template <class T>
class DoubleQueue{
private:

    std::queue<T> write_queue;

    std::mutex mutex;

    int max_queue_size;

public:

    std::queue<T> read_queue;

    DoubleQueue(int max_queue_size=-1): max_queue_size(max_queue_size) {}

    // Adds element to write_queue
    void push(T x){
        std::lock_guard<std::mutex> lock(mutex);

        write_queue.push(x);

        if (max_queue_size != -1 && write_queue.size() > max_queue_size){
            printf("Exceded max queue size... Dropping frame. \n");
            write_queue.pop();
        }
    }

    // Swaps write queue with provided read_queue
    void swap(){
        std::lock_guard<std::mutex> lock(mutex);
        write_queue.swap(read_queue);
    }
};

#endif
