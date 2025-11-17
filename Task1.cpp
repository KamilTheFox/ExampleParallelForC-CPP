#include <iostream>
#include <thread>
#include <shared_mutex>
#include <mutex>
#include <atomic>
#include <chrono>
#include <string>
#include <map>
#include "LiveCounter.h"

int shared_value = 0;
std::shared_mutex shared_mutex;
std::atomic<bool> running{true};
LiveCounter live_counter;

void reader_square() {
    while (running.load(std::memory_order_acquire)) {
        std::shared_lock lock(shared_mutex);
        int value = shared_value;
        lock.unlock();
        
        live_counter.update("square", "[Square] " + std::to_string(value) + "^2 = " + 
                                   std::to_string(value * value));
    }
}

void reader_double() {
    while (running.load(std::memory_order_acquire)) {
        std::shared_lock lock(shared_mutex);
        int value = shared_value;
        lock.unlock();
        
        live_counter.update("double", "[Double] " + std::to_string(value) + " * 2 = " + 
                                   std::to_string(value * 2));
    }
}

void reader_plus2() {
    while (running.load(std::memory_order_acquire)) {
        std::shared_lock lock(shared_mutex);
        int value = shared_value;
        lock.unlock();
        
        live_counter.update("plus2", "[Plus2] " + std::to_string(value) + " + 2 = " + 
                                  std::to_string(value + 2));
    }
}

void writer() {
    while (running.load(std::memory_order_acquire)) {
        std::unique_lock lock(shared_mutex);
        shared_value++;
        int current_value = shared_value;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        lock.unlock();
        
        live_counter.update("writer", ">>> Writer: shared_value = " + std::to_string(current_value));
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

int main() {
    live_counter.init_display();
    
    std::thread writer_thread(writer);
    std::thread reader1_thread(reader_square);
    std::thread reader2_thread(reader_double);
    std::thread reader3_thread(reader_plus2);
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    running.store(false, std::memory_order_release);
    
    writer_thread.join();
    reader1_thread.join();
    reader2_thread.join();
    reader3_thread.join();
    
    std::cout << "\033[10;1H\nProgram finished!\n";  
    return 0;
}