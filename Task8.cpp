#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>

const int N = 12000;

// Задание 1: Сравнение обхода по строкам и столбцам
void task1_row_major() {
    std::cout << "=== ЗАДАНИЕ 1: ПРОСТРАНСТВЕННАЯ ЛОКАЛЬНОСТЬ ===" << std::endl;
    
    std::vector<std::vector<int>> matrix(N, std::vector<int>(N, 1));
    
    auto start = std::chrono::high_resolution_clock::now();
    long long sum1 = 0;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            sum1 += matrix[i][j]; 
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto time1 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    start = std::chrono::high_resolution_clock::now();
    long long sum2 = 0;
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            sum2 += matrix[i][j]; 
        }
    }
    end = std::chrono::high_resolution_clock::now();
    auto time2 = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Обход по строкам: " << time1.count() << " ms, сумма = " << sum1 << std::endl;
    std::cout << "Обход по столбцам: " << time2.count() << " ms, сумма = " << sum2 << std::endl;
    std::cout << "Ускорение: " << (double)time2.count() / time1.count() << "x" << std::endl;
}

void task2_stride_access() {
    std::cout << "\n=== ВРЕМЯ НА ОДНО ОБРАЩЕНИЕ ===" << std::endl;
    
    const int SIZE = 64 * 1024 * 1024;
    std::vector<int> array(SIZE, 1);
    
    std::vector<int> strides = {1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024};
    
    std::cout << "Шаг\tнс/обращение" << std::endl;
    std::cout << "------------------" << std::endl;
    
    for (int stride : strides) {
        auto start = std::chrono::high_resolution_clock::now();
        
        const int TOTAL_ACCESSES = 100000000;
        int index = 0;
        
        for (int access = 0; access < TOTAL_ACCESSES; access++) {
            index = (index + stride) % SIZE;
            volatile int value = array[index];
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        double time_per_access = static_cast<double>(time_ns) / TOTAL_ACCESSES;
        
        std::cout << stride << "\t" << time_per_access << std::endl;
    }
}

struct DataBad {
    std::atomic<int> counter{0};
};

struct DataGood {
    std::atomic<int> counter{0};
    char padding[64 - sizeof(std::atomic<int>)]; 
};

template<typename DataType>
void false_sharing_test(const std::string& name, int num_threads) {
    std::vector<DataType> data(num_threads);
    std::vector<std::thread> threads;
    
    const int ITERATIONS = 100000000;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([&data, i, ITERATIONS]() {
            for (int j = 0; j < ITERATIONS; ++j) {
                data[i].counter.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << name << " с " << num_threads << " потоками: " << time.count() << " ms" << std::endl;
}

void task3_false_sharing() {
    std::cout << "\n=== ЗАДАНИЕ 3: FALSE SHARING ===" << std::endl;
    
    std::vector<int> thread_counts = {1, 2, 4, 8};
    
    for (int threads : thread_counts) {
        false_sharing_test<DataBad>("DataBad (false sharing)", threads);
        false_sharing_test<DataGood>("DataGood (no false sharing)", threads);
        std::cout << "---" << std::endl;
    }
    
}

int main() {
    std::cout << "🚀 ИССЛЕДОВАНИЕ ЛОКАЛЬНОСТИ ДАННЫХ И ПРОИЗВОДИТЕЛЬНОСТИ" << std::endl;
    std::cout << "=====================================================" << std::endl;
    
    // Задание 1: Пространственная локальность
   // task1_row_major();
    
    // Задание 2: Шаг доступа
   // task2_stride_access();
    
    // Задание 3: False sharing
    task3_false_sharing();
    
    std::cout << "\n=====================================================" << std::endl;
    std::cout << "✅ ВСЕ ЭКСПЕРИМЕНТЫ ЗАВЕРШЕНЫ!" << std::endl;
    std::cout << "Ключевые выводы:" << std::endl;
    std::cout << "1. Пространственная локальность ускоряет доступ в 2-10 раз" << std::endl;
    std::cout << "2. Меньший шаг = лучшее использование кэша" << std::endl;
    std::cout << "3. False sharing может убить многопоточное ускорение" << std::endl;
    
    return 0;
}