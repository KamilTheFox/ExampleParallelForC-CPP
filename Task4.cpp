#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <random>
#include <string>
#include <mutex>
#include "LinkedList.h"

class ListTester {
private:
    LinkedList list;
    std::atomic<bool> running{true};
    std::atomic<int> total_operations{0};
    
    struct ThreadState {
        std::string status;
        std::string details;
        std::atomic<int> operations{0};
        std::mutex mtx;
    };
    
    std::vector<ThreadState> thread_states;
    std::mutex display_mtx;
    bool display_initialized{false};
    
public:
    ListTester() : thread_states(5) {}
    
    void clear_line(int line) {
        std::cout << "\033[" << line << ";1H";  // Перемещаем курсор на строку
        std::cout << "\033[K";                  // Очищаем строку
    }
    
    void write_line(int line, const std::string& text) {
        std::cout << "\033[" << line << ";1H";  // Перемещаем курсор на строку
        std::cout << text;                      // Выводим текст
        std::cout << "\033[K";                  // Очищаем остаток строки
    }
    
    void initialize_display() {
        std::cout << "\033[2J";  // Очищаем весь экран
        std::cout << "\033[1;1H"; // Курсор в начало
        
        std::cout << "=== Thread-Safe Linked List Test ===\n";
        std::cout << "Total operations: 0\n";
        std::cout << "List size: " << list.size() << "\n\n";
        
        std::cout << "=== WRITERS ===\n";
        for (int i = 0; i < 3; i++) {
            std::cout << "Writer " << i << ": WAITING | Initializing... | Ops: 0\n";
        }
        
        std::cout << "\n=== READERS ===\n";
        for (int i = 0; i < 2; i++) {
            std::cout << "Reader " << i << ": WAITING | Initializing... | Ops: 0\n";
        }
        
        std::cout << "\n=== LIST CONTENTS ===\n";
        list.print();
        
        std::cout << "\nTest running for 10 seconds...\n";
        std::cout.flush();
        
        display_initialized = true;
    }
    
    void update_display() {
        std::lock_guard lock(display_mtx);
        
        if (!display_initialized) {
            initialize_display();
            return;
        }
        
        clear_line(2);
        std::cout << "Total operations: " << total_operations.load();
        
        clear_line(3);
        std::cout << "List size: " << list.size();
        
        // Обновляем информацию о писателях (строки 6-8)
        for (int i = 0; i < 3; i++) {
            int line = 6 + i;
            clear_line(line);
            std::string status, details;
            {
                std::lock_guard lock(thread_states[i].mtx);
                status = thread_states[i].status;
                details = thread_states[i].details;
            }
            std::cout << "Writer " << i << ": " << status 
                    << " | " << details 
                    << " | Ops: " << thread_states[i].operations.load();
        }
        
        // Обновляем информацию о читателях (строки 11-12)
        for (int i = 0; i < 2; i++) {
            int line = 11 + i;
            clear_line(line);
            std::string status, details;
            {
                std::lock_guard lock(thread_states[i + 3].mtx);
                status = thread_states[i + 3].status;
                details = thread_states[i + 3].details;
            }
            std::cout << "Reader " << i << ": " << status 
                    << " | " << details 
                    << " | Ops: " << thread_states[i + 3].operations.load();
        }
        
        // Обновляем содержимое списка (строка 15)
        clear_line(15);
        std::cout << "List: ";
        // Для простоты показываем только размер, чтобы не блокировать список надолго
        std::cout << "[size: " << list.size() << "]";
        
        // Перемещаем курсор вниз чтобы не мешать выводу
        std::cout << "\033[20;1H";
        std::cout.flush();
    }
    
    void update_thread_state(int thread_id, const std::string& status, const std::string& details = "") {
        {
            std::lock_guard lock(thread_states[thread_id].mtx);
            thread_states[thread_id].status = status;
            thread_states[thread_id].details = details;
        }
        thread_states[thread_id].operations++;
        total_operations++;
        
        update_display();
    }
    
    void writer_thread(int id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> value_dis(1, 100);
        std::uniform_int_distribution<> op_dis(0, 2);
        std::uniform_int_distribution<> sleep_dis(200, 500);
        
        update_thread_state(id, "STARTED", "Writer initialized");
        
        while (running.load(std::memory_order_acquire)) {
            int value = value_dis(gen);
            int op = op_dis(gen);
            
            try {
                switch (op) {
                    case 0: {
                        update_thread_state(id, "INSERT", "Inserting " + std::to_string(value) + " at head");
                        list.insert(value);
                        update_thread_state(id, "WAITING", "Insert completed");
                        break;
                    }
                    case 1: {
                        int target = value_dis(gen) % 50 + 1;
                        update_thread_state(id, "INSERT_AFTER", "Inserting " + std::to_string(value) + " after " + std::to_string(target));
                        list.insertAfter(target, value);
                        update_thread_state(id, "WAITING", "InsertAfter completed");
                        break;
                    }
                    case 2: {
                        update_thread_state(id, "REMOVE", "Removing value " + std::to_string(value));
                        bool removed = list.remove(value);
                        update_thread_state(id, "WAITING", removed ? "Remove SUCCESS" : "Remove FAILED - not found");
                        break;
                    }
                }
            } catch (const std::exception& e) {
                update_thread_state(id, "ERROR", std::string("Exception: ") + e.what());
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dis(gen)));
        }
        
        update_thread_state(id, "FINISHED", "Writer completed work");
    }
    
    void reader_thread(int id) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> value_dis(1, 100);
        std::uniform_int_distribution<> sleep_dis(100, 300);
        
        int thread_index = id + 3;  // Readers are at indices 3 and 4
        update_thread_state(thread_index, "STARTED", "Reader initialized");
        
        while (running.load(std::memory_order_acquire)) {
            int value = value_dis(gen);
            
            try {
                update_thread_state(thread_index, "SEARCH", "Searching for " + std::to_string(value));
                bool found = list.find(value);
                update_thread_state(thread_index, "WAITING", "Search " + std::to_string(value) + " = " + (found ? "FOUND" : "NOT FOUND"));
            } catch (const std::exception& e) {
                update_thread_state(thread_index, "ERROR", std::string("Exception: ") + e.what());
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dis(gen)));
        }
        
        update_thread_state(thread_index, "FINISHED", "Reader completed work");
    }
    
    void run_test() {
        std::cout << "Initializing list with some values...\n";
        
        // Инициализируем список начальными значениями
        for (int i = 1; i <= 3; i++) {
            list.insert(i * 10);
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::cout << "Starting 3 writers and 2 readers for 10 seconds...\n";
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        initialize_display();
        
        std::vector<std::thread> threads;
        
        // Запускаем writer потоки
        for (int i = 0; i < 3; i++) {
            threads.emplace_back(&ListTester::writer_thread, this, i);
        }
        
        // Запускаем reader потоки
        for (int i = 0; i < 2; i++) {
            threads.emplace_back(&ListTester::reader_thread, this, i);
        }
        
        // Даем потокам поработать 10 секунд
        auto start_time = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(10)) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Останавливаем потоки
        running.store(false, std::memory_order_release);
        
        // Ждем завершения всех потоков
        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
        
        // Финальный результат
        std::cout << "\n\n=== FINAL RESULT ===\n";
        std::cout << "Total operations: " << total_operations.load() << std::endl;
        std::cout << "Final list size: " << list.size() << std::endl;
        list.print();
        std::cout << "Test completed successfully!\n";
    }
};

int main() {
    try {
        ListTester tester;
        tester.run_test();
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}