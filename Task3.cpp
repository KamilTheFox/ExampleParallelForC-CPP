#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>
#include <queue>
#include <condition_variable>
#include <mutex>
#include <vector>
#include "LiveCounter.h"

class Pipeline 
{
private:
    std::queue<int> queue1, queue2, queue3;
    std::mutex mtx1, mtx2, mtx3;
    std::condition_variable cv1, cv2, cv3;
    std::atomic<bool> running{true};
    LiveCounter live_counter;
    
    std::atomic<int> current_value{0};
    std::atomic<int> stage1_value{0};
    std::atomic<int> stage2_value{0}; 
    std::atomic<int> stage3_value{0};

public:
    void writer() 
    {
        int value = 0;
        
        while (running.load(std::memory_order_acquire)) 
        {
            current_value.store(++value, std::memory_order_release);
            
            {
                std::unique_lock lock(mtx1);
                queue1.push(value);
                update_display();
            }
            cv1.notify_one();
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1500));
        }
    }
    
    void reader_square() 
    {
        while (running.load(std::memory_order_acquire)) 
        {
            int value;
            
            {
                std::unique_lock lock(mtx1);
                cv1.wait(lock, [this]() { return !queue1.empty() || !running; });
                if (!running && queue1.empty()) break;
                
                value = queue1.front();
                queue1.pop();
                stage1_value.store(value, std::memory_order_release);
                update_display();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            int result = value * value;
            
            {
                std::unique_lock lock(mtx2);
                queue2.push(result);
                stage1_value.store(0, std::memory_order_release);
                update_display();
            }
            cv2.notify_one();
        }
    }
    
    void reader_double() 
    {
        while (running.load(std::memory_order_acquire)) 
        {
            int value;
            
            {
                std::unique_lock lock(mtx2);
                cv2.wait(lock, [this]() { return !queue2.empty() || !running; });
                if (!running && queue2.empty()) break;
                
                value = queue2.front();
                queue2.pop();
                stage2_value.store(value, std::memory_order_release);
                update_display();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            int result = value * 2;
            
            {
                std::unique_lock lock(mtx3);
                queue3.push(result);
                stage2_value.store(0, std::memory_order_release);
                update_display();
            }
            cv3.notify_one();
        }
    }
    
    void reader_plus2() 
    {
        while (running.load(std::memory_order_acquire)) 
        {
            int value;
            
            {
                std::unique_lock lock(mtx3);
                cv3.wait(lock, [this]() { return !queue3.empty() || !running; });
                if (!running && queue3.empty()) break;
                
                value = queue3.front();
                queue3.pop();
                stage3_value.store(value, std::memory_order_release);
                update_display();
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            int result = value + 2;
            
            stage3_value.store(0, std::memory_order_release);
            live_counter.update("final", "[FINAL] Result: " + std::to_string(result));
        }
    }
    
    void update_display() 
    {
        int current = current_value.load(std::memory_order_acquire);
        int s1 = stage1_value.load(std::memory_order_acquire);
        int s2 = stage2_value.load(std::memory_order_acquire); 
        int s3 = stage3_value.load(std::memory_order_acquire);
        
        std::string writer_status = ">>> Writer: ";
        if (current > 0) {
            writer_status += "produced [" + std::to_string(current) + "]";
        } else {
            writer_status += "waiting...";
        }
        
        live_counter.update("writer", writer_status);
        live_counter.update("square", "[Stage1-Square] " + (s1 > 0 ? "processing [" + std::to_string(s1) + "]" : "waiting..."));
        live_counter.update("double", "[Stage2-Double] " + (s2 > 0 ? "processing [" + std::to_string(s2) + "]" : "waiting..."));
        live_counter.update("plus2", "[Stage3-Plus2]  " + (s3 > 0 ? "processing [" + std::to_string(s3) + "]" : "waiting..."));
    }
    
    void run() 
    {
        live_counter.init_display_pipeline();
        
        std::thread writer_thread(&Pipeline::writer, this);
        std::thread reader1_thread(&Pipeline::reader_square, this);
        std::thread reader2_thread(&Pipeline::reader_double, this);
        std::thread reader3_thread(&Pipeline::reader_plus2, this);
        
        std::this_thread::sleep_for(std::chrono::seconds(20));
        running.store(false, std::memory_order_release);
        
        cv1.notify_one();
        cv2.notify_one();
        cv3.notify_one();
        
        writer_thread.join();
        reader1_thread.join();
        reader2_thread.join();
        reader3_thread.join();
        
        std::cout << "\033[7;1H\nPipeline processing finished!\n";
    }
};

int main() 
{
    Pipeline pipeline;
    pipeline.run();
    return 0;
}