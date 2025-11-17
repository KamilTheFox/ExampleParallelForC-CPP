#include <iostream>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <omp.h>
#include <cmath>
#include <functional>

constexpr size_t N = 100000000;
constexpr int ITERATIONS = 3;

template<typename T>
void prevent_optimization(T& value) {
    asm volatile("" : "+r"(value));
}

class Timer {
private:
    std::chrono::high_resolution_clock::time_point start_time;
    
public:
    void start() {
        start_time = std::chrono::high_resolution_clock::now();
    }
    
    double elapsed() const {
        auto end_time = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(end_time - start_time).count();
    }
};

class Benchmark {
private:
    std::vector<double> data;
    std::vector<float> a, b;
    
public:
    Benchmark(size_t size) : data(size), a(size), b(size) {
        initialize_data();
    }
    
    void initialize_data() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<double> dist_double(0.0, 100.0);
        std::uniform_real_distribution<float> dist_float(0.0f, 10.0f);
        
        #pragma omp parallel
        {
            #pragma omp for schedule(static)
            for (size_t i = 0; i < data.size(); i++) {
                data[i] = dist_double(gen);
            }
            
            #pragma omp for schedule(static)
            for (size_t i = 0; i < a.size(); i++) {
                a[i] = dist_float(gen);
                b[i] = dist_float(gen);
            }
        }
    }
    
    // Универсальная функция бенчмаркинга с повторениями
    template<typename Func>
    double benchmark_function(Func&& func, const std::string& name = "") {
        double total_time = 0.0;
        double min_time = std::numeric_limits<double>::max();
        double max_time = 0.0;
        
        for (int i = 0; i < ITERATIONS; i++) {
            Timer timer;
            timer.start();
            auto result = func();
            prevent_optimization(result);
            double elapsed = timer.elapsed();
            
            total_time += elapsed;
            min_time = std::min(min_time, elapsed);
            max_time = std::max(max_time, elapsed);
        }
        
        double avg_time = total_time / ITERATIONS;
        
        if (!name.empty()) {
            std::cout << name << ": " << avg_time << " сек (min: " << min_time 
                      << ", max: " << max_time << ", diff: " << (max_time - min_time) << ")\n";
        }
        
        return avg_time;
    }
    
    // Методы суммирования
    double stl_sequential_sum() {
        double sum = std::accumulate(data.begin(), data.end(), 0.0);
        return sum;
    }
    
    double openmp_reduction_sum() {
        double sum = 0.0;
        #pragma omp parallel for schedule(static)
        // reduction(+:sum)
        for (size_t i = 0; i < data.size(); i++) {
            sum += data[i];
        }
        return sum;
    }
    
    double openmp_atomic_sum() {
        double sum = 0.0;
        #pragma omp parallel
        {
            double local_sum = 0.0;
            #pragma omp for schedule(static)
            for (size_t i = 0; i < data.size(); i++) {
                local_sum += data[i];
            }
            #pragma omp atomic
            sum += local_sum;
        }
        return sum;
    }
    
    double openmp_critical_sum() {
        double sum = 0.0;
        #pragma omp parallel
        {
            double local_sum = 0.0;
            #pragma omp for schedule(static)
            for (size_t i = 0; i < data.size(); i++) {
                local_sum += data[i];
            }
            #pragma omp critical
            {
                sum += local_sum;
            }
        }
        return sum;
    }
    
    // Векторизация
    void standard_vector_operation(std::vector<float>& result) {
        result.resize(a.size());
        #pragma omp parallel for schedule(static)
        for (size_t i = 0; i < a.size(); i++) {
            float temp1 = a[i] * b[i];
            float temp2 = a[i] + b[i];
            float temp3 = temp1 * temp2;
            float temp4 = temp3 - a[i];
            float temp5 = temp4 + b[i];
            result[i] = temp5 * 2.0f + 1.0f;
        }
    }
    
    void vectorized_operation(std::vector<float>& result) {
        result.resize(a.size());
        #pragma omp parallel for simd schedule(static)
        for (size_t i = 0; i < a.size(); i++) {
            float temp1 = a[i] * b[i];
            float temp2 = a[i] + b[i];
            float temp3 = temp1 * temp2;
            float temp4 = temp3 - a[i];
            float temp5 = temp4 + b[i];
            result[i] = temp5 * 2.0f + 1.0f;
        }
    }
    
    // Улучшенная проверка корректности
    bool verify_sums() {
        std::cout << "Проверка корректности суммирования...\n";
        
        // Вычисляем все суммы последовательно для точности
        double stl_sum = stl_sequential_sum();
        double reduction_sum = 0.0, atomic_sum = 0.0, critical_sum = 0.0;
        
        #pragma omp parallel
        {
            // Reduction
            #pragma omp single
            {
                reduction_sum = openmp_reduction_sum();
            }
            
            // Atomic
            #pragma omp single
            {
                atomic_sum = openmp_atomic_sum();
            }
            
            // Critical
            #pragma omp single
            {
                critical_sum = openmp_critical_sum();
            }
        }
        
        // Относительная проверка с учетом ошибок округления
        double max_val = std::max({std::abs(stl_sum), std::abs(reduction_sum), 
                                  std::abs(atomic_sum), std::abs(critical_sum)});
        double tolerance = max_val * 1e-12;
        
        bool reduction_ok = std::fabs(stl_sum - reduction_sum) < tolerance;
        bool atomic_ok = std::fabs(stl_sum - atomic_sum) < tolerance;
        bool critical_ok = std::fabs(stl_sum - critical_sum) < tolerance;
        
        if (!reduction_ok) {
            std::cout << "  Ошибка reduction: " << std::fabs(stl_sum - reduction_sum) 
                      << " > " << tolerance << std::endl;
        }
        if (!atomic_ok) {
            std::cout << "  Ошибка atomic: " << std::fabs(stl_sum - atomic_sum) 
                      << " > " << tolerance << std::endl;
        }
        if (!critical_ok) {
            std::cout << "  Ошибка critical: " << std::fabs(stl_sum - critical_sum) 
                      << " > " << tolerance << std::endl;
        }
        
        bool all_ok = reduction_ok && atomic_ok && critical_ok;
        std::cout << "  Результат: " << (all_ok ? "✓ ВСЕ КОРРЕКТНО" : "✗ ЕСТЬ ОШИБКИ") << std::endl;
        
        return all_ok;
    }
    
    bool verify_vectorization() {
        std::vector<float> result_std, result_vec;
        
        standard_vector_operation(result_std);
        vectorized_operation(result_vec);
        
        for (size_t i = 0; i < result_std.size(); i += 1000000) { // Проверяем каждую миллионную
            if (std::fabs(result_std[i] - result_vec[i]) > 1e-6f) {
                std::cout << "  Ошибка векторизации на элементе " << i << ": " 
                          << result_std[i] << " != " << result_vec[i] << std::endl;
                return false;
            }
        }
        
        std::cout << "  Векторизация: ✓ КОРРЕКТНА" << std::endl;
        return true;
    }
    
    void run_sum_benchmark() {
        std::cout << "\n🎯 ЗАДАНИЕ 1: Методы суммирования\n";
        std::cout << "=================================\n";
        
        // Сначала проверяем корректность
        verify_sums();
        
        std::cout << "\n📊 ЗАМЕРЫ ПРОИЗВОДИТЕЛЬНОСТИ (" << ITERATIONS << " повторений):\n";
        
        const std::vector<int> thread_counts = {1, 2, 4, 6}; // Используем 6 вместо 8
        
        for (int num_threads : thread_counts) {
            omp_set_num_threads(num_threads);
            std::cout << "\n--- ПОТОКОВ: " << num_threads << " ---\n";
            
            double stl_time = benchmark_function([this]() { return stl_sequential_sum(); }, "STL sequential    ");
            double reduction_time = benchmark_function([this]() { return openmp_reduction_sum(); }, "OpenMP reduction  ");
            double atomic_time = benchmark_function([this]() { return openmp_atomic_sum(); }, "OpenMP atomic     ");
            double critical_time = benchmark_function([this]() { return openmp_critical_sum(); }, "OpenMP critical   ");
            
            if (num_threads > 1) {
                double speedup = stl_time / reduction_time;
                std::cout << "Ускорение reduction: " << speedup << "x (";
                if (speedup > num_threads * 0.8) std::cout << "отлично";
                else if (speedup > num_threads * 0.6) std::cout << "хорошо";
                else std::cout << "плохо";
                std::cout << ")\n";
            }
        }
    }
    
    void run_vectorization_benchmark() {
        std::cout << "\n🎯 ЗАДАНИЕ 2: Векторизация\n";
        std::cout << "==========================\n";
        
        // Проверяем корректность
        verify_vectorization();
        
        std::cout << "\n📊 ЗАМЕРЫ ПРОИЗВОДИТЕЛЬНОСТИ (" << ITERATIONS << " повторений):\n";
        
        const std::vector<int> thread_counts = {1, 2, 4, 6};
        std::vector<float> result_std, result_vec;
        
        for (int num_threads : thread_counts) {
            omp_set_num_threads(num_threads);
            std::cout << "\n--- ПОТОКОВ: " << num_threads << " ---\n";
            
            double time_std = benchmark_function([this, &result_std]() { 
                standard_vector_operation(result_std); 
                return 0.0; // Возвращаем фиктивное значение
            }, "Стандартная      ");
            
            double time_vec = benchmark_function([this, &result_vec]() { 
                vectorized_operation(result_vec); 
                return 0.0;
            }, "Векторизованная  ");
            
            double speedup = time_std / time_vec;
            std::cout << "Ускорение: " << speedup << "x - ";
            
            if (speedup > 1.1) std::cout << "✅ отличный результат";
            else if (speedup > 1.01) std::cout << "⚠️ небольшое улучшение"; 
            else if (speedup >= 0.99) std::cout << "➖ без изменений";
            else std::cout << "❌ замедление";
            std::cout << std::endl;
        }
    }
    
    void print_system_info() {
        std::cout << "=== СИСТЕМНАЯ ИНФОРМАЦИЯ ===\n";
        std::cout << "Размер данных: " << N << " элементов\n";
        std::cout << "Повторений: " << ITERATIONS << " для каждого теста\n";
        std::cout << "Потоков OpenMP: " << omp_get_max_threads() << " (максимум)\n";
        std::cout << "Используемые потоки: 1, 2, 4, 6\n";
        #ifdef _OPENMP
        std::cout << "Версия OpenMP: " << _OPENMP << "\n";
        #endif
        std::cout << std::endl;
    }
};

int main() {
    std::cout << "=== УЛУЧШЕННЫЙ OPENMP BENCHMARK ===\n";
    std::cout << "    (6 потоков, " << ITERATIONS << " повторений)\n\n";
    
    try {
        Benchmark benchmark(N);
        benchmark.print_system_info();
        benchmark.run_sum_benchmark(); 
        benchmark.run_vectorization_benchmark();
        
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    std::cout << "\n🎉 Бенчмарк завершен!\n";
    return 0;
}