#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <semaphore>
#include "LiveCounter.h"

int shared_value = 0;
std::atomic<bool> running{true};
LiveCounter live_counter;

// Семафоры для синхронизации

std::counting_semaphore<3> reader_sem{0};    // Разрешения для читателей
std::counting_semaphore<1> writer_sem{1};    // Разрешение для писателя
std::atomic<int> readers_done{0};            // Счетчик завершивших читателей

void reader_square() 
{
    while (running.load(std::memory_order_acquire)) 
    {
        // Ждем разрешения от писателя
        reader_sem.acquire();
        
        // Читаем значение
        int value = shared_value;
        live_counter.update("square", "[Square] " + std::to_string(value) + "^2 = " + 
                                   std::to_string(value * value));
        
        readers_done++;
        
        if (readers_done == 3) {
            writer_sem.release();
        }
    }
}

void reader_double() 
{
    while (running.load(std::memory_order_acquire)) 
    {
        reader_sem.acquire();
        
        int value = shared_value;
        live_counter.update("double", "[Double] " + std::to_string(value) + " * 2 = " + 
                                   std::to_string(value * 2));
        
        readers_done++;
        if (readers_done == 3) {
            writer_sem.release();
        }
    }
}

void reader_plus2() 
{
    while (running.load(std::memory_order_acquire)) 
    {
        reader_sem.acquire();
        
        int value = shared_value;
        live_counter.update("plus2", "[Plus2] " + std::to_string(value) + " + 2 = " + 
                                  std::to_string(value + 2));
        
        readers_done++;
        if (readers_done == 3) {
            writer_sem.release();
        }
    }
}

void writer() 
{
    while (running.load(std::memory_order_acquire)) 
    {
        // Ждем своего разрешения
        writer_sem.acquire();
        
        if (!running) break;
        
        shared_value++;
        live_counter.update("writer", ">>> Writer: shared_value = " + std::to_string(shared_value));
        
        readers_done = 0;
        reader_sem.release(3); 
        
        // Небольшая задержка для наглядности
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
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
    
    writer_sem.release();
    reader_sem.release(3);
    
    writer_thread.join();
    reader1_thread.join();
    reader2_thread.join();
    reader3_thread.join();
    
    std::cout << "\033[10;1H\nProgram finished!\n";
    return 0;
}