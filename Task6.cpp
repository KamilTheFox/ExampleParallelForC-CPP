#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <atomic>
#include <random>
#include <chrono>


class UnsafeCounter {
private:
    int value = 0;
    std::random_device rd;
    std::mt19937 gen;

public:
    void increment() {
        int local_value = value;
        
        std::uniform_int_distribution<int> delay_dist(1, 10);
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
        
        // Увеличиваем локальную копию
        local_value = local_value + 1;
        
        // Еще одна рандомная задержка
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_dist(gen)));
        
        // ❌ DATA RACE: записываем значение обратно
        value = local_value;
    }
    
    int getValue() const {
        return value;
    }
};

std::mutex mutex1, mutex2, mutex3;

void data_race_example() {
    std::cout << "=== ПРИМЕР 1: DATA RACE ===" << std::endl;
    
    UnsafeCounter counter;
    std::vector<std::thread> threads;
    
    // Запускаем 10 потоков, которые инкрементят счетчик
    for (int i = 0; i < 100; i++) {
        threads.emplace_back([&counter]() {
            for (int j = 0; j < 1000; j++) {
                counter.increment();
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    // ❌ Ожидаем 10000, но получаем меньше из-за data race
    std::cout << "Ожидаемое значение: 100000" << std::endl;
    std::cout << "Реальное значение: " << counter.getValue() << std::endl;
    std::cout << "Data race detected! Разница: " << 100000 - counter.getValue() << std::endl;
}

void deadlock_example() {
    std::cout << "\n=== ПРИМЕР 2: DEADLOCK ===" << std::endl;
    
    auto worker1 = []() {
        std::cout << "Поток 1: захватываю mutex1..." << std::endl;
        std::lock_guard<std::mutex> lock1(mutex1);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Поток 1: пытаюсь захватить mutex2..." << std::endl;
        std::lock_guard<std::mutex> lock2(mutex2); // ❌ DEADLOCK!
        std::cout << "Поток 1: завершил работу" << std::endl;
    };
    
    auto worker2 = []() {
        std::cout << "Поток 2: захватываю mutex2..." << std::endl;
        std::lock_guard<std::mutex> lock2(mutex2);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Поток 2: пытаюсь захватить mutex3..." << std::endl;
        std::lock_guard<std::mutex> lock3(mutex3); // ❌ DEADLOCK!
        std::cout << "Поток 2: завершил работу" << std::endl;
    };
    
    auto worker3 = []() {
        std::cout << "Поток 3: захватываю mutex3..." << std::endl;
        std::lock_guard<std::mutex> lock3(mutex3);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        std::cout << "Поток 3: пытаюсь захватить mutex1..." << std::endl;
        std::lock_guard<std::mutex> lock1(mutex1); // ❌ DEADLOCK!
        std::cout << "Поток 3: завершил работу" << std::endl;
    };
    
    std::thread t1(worker1);
    std::thread t2(worker2);
    std::thread t3(worker3);
    
    // Даем потокам время на deadlock
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    std::cout << "❌ DEADLOCK! Потоки зависли навсегда..." << std::endl;
    
    t1.detach();
    t2.detach();
    t3.detach();
}


class OverpoliteSystem {
private:
    std::atomic<bool> resource_available{true};
    std::atomic<int> attempts{0};

public:
    void worker(const std::string& name) {
        while (attempts.load() < 100) {
            // ❌ ПРОБЛЕМА: если ресурс доступен (true), мы должны его захватывать,
            // а не уступать! Сейчас логика перевернута.
            
            if (resource_available.load()) {
                // Пытаемся захватить ресурс
                bool expected = true;
                if (resource_available.compare_exchange_weak(expected, false)) {
                    std::cout << name << ": УСПЕХ! Захватил ресурс!" << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    resource_available.store(true);
                    return; // Успешно завершили
                }
                // Если не удалось захватить - кто-то опередил
            }
            
            // ❌ LIVELOCK: слишком "вежливы" - всегда уступаем
            std::cout << name << ": ресурс занят, уступаю другому... (попытка " << attempts.load() << ")" << std::endl;
            attempts.fetch_add(1);
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
            std::this_thread::yield();
        }
        
        std::cout << name << ": СДАЮСЬ! Livelock detected!" << std::endl;
    }
};

void livelock_example() {
    std::cout << "\n=== ПРИМЕР 3: LIVELOCK ===" << std::endl;
    std::cout << "Потоки слишком вежливы и постоянно уступают друг другу..." << std::endl;
    
    OverpoliteSystem system;
    
    std::thread t1([&system]() { system.worker("Поток 1"); });
    std::thread t2([&system]() { system.worker("Поток 2"); });
    std::thread t3([&system]() { system.worker("Поток 3"); });
    
    t1.join();
    t2.join();
    t3.join();
    
    std::cout << "❌ LIVELOCK: Потоки вежливо уступали, но работа не сделана!" << std::endl;
}

int main() {
    std::cout << "🚀 ДЕМОНСТРАЦИЯ ТИПИЧНЫХ ОШИБОК МНОГОПОТОЧНОСТИ" << std::endl;
    std::cout << "==============================================" << std::endl;
    
    // Запускаем примеры по одному
    data_race_example();
    
    livelock_example();
    
    std::cout << "\n==============================================" << std::endl;
    std::cout << "✅ Все примеры демонстрируют типичные ошибки!" << std::endl;
    std::cout << "Data Race - потерянные обновления" << std::endl;
    std::cout << "Deadlock - взаимные блокировки" << std::endl;  
    std::cout << "Livelock - работа есть, но прогресса нет" << std::endl;

    deadlock_example(); 
    
    return 0;
}