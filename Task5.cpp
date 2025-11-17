#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <random>
#include <fstream>
#include <mutex>

std::ofstream logfile;
std::mutex log_mutex;

void log(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    logfile << message << std::endl;
    std::cout << message << std::endl; 
}

struct Cell {
    std::atomic<int> value;
    std::atomic<long> version;
    
    Cell() : value(0), version(0) {}
};

class Snapshot {
private:
    std::vector<Cell> registers;
    int size;

public:
    Snapshot(int n) : size(n), registers(n) {}
    
    void update(int idx, int new_value, const std::string& thread_name) {
        long old_version = registers[idx].version.load(std::memory_order_relaxed);
        int old_value = registers[idx].value.load(std::memory_order_relaxed);
        
        registers[idx].value.store(new_value, std::memory_order_relaxed);
        registers[idx].version.store(old_version + 1, std::memory_order_release);
        
       // log("[" + thread_name + "] Регистр " + std::to_string(idx) + 
        //    " изменен: " + std::to_string(old_value) + "->" + std::to_string(new_value) + 
          //  " (v" + std::to_string(old_version) + "->v" + std::to_string(old_version + 1) + ")");
    }
    
    std::pair<int, long> collect(int idx) {
        int val = registers[idx].value.load(std::memory_order_relaxed);
        long ver = registers[idx].version.load(std::memory_order_acquire);
        return {val, ver};
    }
    
    std::vector<int> scan(int scan_number) {
        log("\n=== 🔍 СНИМОК #" + std::to_string(scan_number) + " ===");
        
        std::vector<std::pair<int, long>> old_copy;
        std::vector<std::pair<int, long>> new_copy;
        
        int attempts = 0;
        
        // Первое чтение всех регистров
        old_copy.resize(size);
        std::string first_read = "Первое чтение: [";
        for (int i = 0; i < size; i++) {
            old_copy[i] = collect(i);
            first_read += std::to_string(old_copy[i].first) + "(v" + std::to_string(old_copy[i].second) + ")";
            if (i < size - 1) first_read += ", ";
        }
        first_read += "]";
        log(first_read);
        
        while (true) {
            attempts++;
            
            // Второе чтение всех регистров
            new_copy.resize(size);
            std::string second_read = "Попытка " + std::to_string(attempts) + " чтение: [";
            for (int i = 0; i < size; i++) {
                new_copy[i] = collect(i);
                second_read += std::to_string(new_copy[i].first) + "(v" + std::to_string(new_copy[i].second) + ")";
                if (i < size - 1) second_read += ", ";
            }
            second_read += "]";
            log(second_read);
            
            // Проверяем, изменились ли версии
            bool consistent = true;
            std::string check_msg = "Сравнение: ";
            for (int i = 0; i < size; i++) {
                if (old_copy[i].second != new_copy[i].second) {
                    consistent = false;
                    check_msg += "Рег" + std::to_string(i) + "(v" + std::to_string(old_copy[i].second) + 
                                "->v" + std::to_string(new_copy[i].second) + ") ИЗМЕНИЛСЯ! ";
                    break;
                } else {
                    check_msg += "Рег" + std::to_string(i) + "(v" + std::to_string(old_copy[i].second) + ") ";
                }
            }
            log(check_msg);
            
            if (consistent) {
                std::string success_msg = "🎉 СНИМОК #" + std::to_string(scan_number) + 
                                        " СОГЛАСОВАН за " + std::to_string(attempts) + " попыток!";
                log(success_msg);
                
                std::vector<int> result(size);
                for (int i = 0; i < size; i++) {
                    result[i] = new_copy[i].first;
                }
                return result;
            }
            
            log("❌ Обнаружены изменения, повторяем снимок...");
            
            old_copy = new_copy;
        }
    }
    
    int getSize() const { return size; }
};

// Функция для потоков, которые обновляют регистры
void updater(Snapshot& snapshot, int thread_id, int updates_count) {
    std::string thread_name = "Updater-" + std::to_string(thread_id);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> value_dist(1, 100);
    std::uniform_int_distribution<int> index_dist(0, snapshot.getSize() - 1);
    
    for (int i = 0; i < updates_count; i++) {
        int idx = index_dist(gen);
        int value = value_dist(gen);
        snapshot.update(idx, value, thread_name);
        
        std::this_thread::sleep_for(std::chrono::microseconds(20));
    }
    
    log("[" + thread_name + "] ✅ ЗАВЕРШИЛ РАБОТУ");
}

// Функция для потока, который делает снимки
void scanner(Snapshot& snapshot, int scans_count) {
    for (int i = 0; i < scans_count; i++) {
        auto result = snapshot.scan(i + 1);
        
        // Печатаем снимок
        std::string snapshot_msg = "📸 СНИМОК #" + std::to_string(i + 1) + ": [";
        for (size_t j = 0; j < result.size(); j++) {
            snapshot_msg += std::to_string(result[j]);
            if (j < result.size() - 1) snapshot_msg += ", ";
        }
        snapshot_msg += "]";
        log(snapshot_msg);
        log("=========================================");
        
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
}

int main() {
    const int N = 5;
    const int UPDATER_THREADS = 8;
    const int UPDATES_PER_THREAD = 1000;
    const int SCANS_COUNT = 10;
    
    // Открываем файл для логов
    logfile.open("snapshot_log.txt");
    if (!logfile.is_open()) {
        std::cerr << "Не удалось открыть файл для логов!" << std::endl;
        return 1;
    }
    
    Snapshot snapshot(N);
    
    log("🚀 ЗАПУСК Lock-Free Snapshot с записью в файл");
    log("Регистров: " + std::to_string(N) + ", Потоков-обновителей: " + std::to_string(UPDATER_THREADS));
    log("Файл логов: snapshot_log.txt");
    log("=========================================");
    
    auto start_time = std::chrono::steady_clock::now();
    
    // Запускаем потоки-обновители
    std::vector<std::thread> updaters;
    for (int i = 0; i < UPDATER_THREADS; i++) {
        updaters.emplace_back(updater, std::ref(snapshot), i + 1, UPDATES_PER_THREAD);
    }
    
    // Запускаем поток-сканер
    std::thread scanner_thread(scanner, std::ref(snapshot), SCANS_COUNT);
    
    // Ждем завершения всех потоков
    for (auto& t : updaters) {
        t.join();
    }
    scanner_thread.join();
    
    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    
    log("=========================================");
    log("🎉 ТЕСТ ЗАВЕРШЕН за " + std::to_string(duration.count()) + " мс");
    log("Логи сохранены в snapshot_log.txt");
    
    logfile.close();
    
    std::cout << "\nПроверь файл snapshot_log.txt - там все подробные логи!" << std::endl;
    
    return 0;
}