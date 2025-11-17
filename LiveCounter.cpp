#include "LiveCounter.h"
#include <iostream>
#include <mutex>

void LiveCounter::update(const std::string& key, const std::string& message) 
{
    std::lock_guard lock(mtx);
    
    if (last_values[key] == message) 
    {
        counters[key]++;
    }
    else 
    {
        counters[key] = 1;
        last_values[key] = message;
    }
    
    refresh_line(key, message + " (Repeat: " + std::to_string(counters[key]) + ")");
}

void LiveCounter::refresh_line(const std::string& key, const std::string& full_message) 
{
    static std::map<std::string, int> line_positions = 
    {
        {"writer", 1},
        {"square", 2}, 
        {"double", 3},
        {"plus2", 4},
        {"final", 5}
    };
    
    int line = line_positions[key];
    std::cout << "\033[" << line << ";1H";
    std::cout << "\033[K";
    std::cout << full_message;
    std::cout.flush();
}

void LiveCounter::init_display() 
{
    std::cout << "\033[2J";
    std::cout << "\033[1;1H";
    for(int i = 0; i < 5; i++)
        std::cout << std::endl;
    std::cout << "=== RW-Lock Live Counters ===\n";
    std::cout << ">>> Writer: initializing...\n";
    std::cout << "[Square] initializing...\n"; 
    std::cout << "[Double] initializing...\n";
    std::cout << "[Plus2] initializing...\n";
    std::cout.flush();
}

void LiveCounter::init_display_pipeline() 
{
    std::cout << "\033[2J";
    std::cout << "\033[1;1H";
    for(int i = 0; i < 5; i++)
        std::cout << std::endl;
    std::cout << "=== Pipeline Processing (Condition Variables) ===\n";
    std::cout << ">>> Writer: initializing...\n";
    std::cout << "[Stage1-Square] initializing...\n"; 
    std::cout << "[Stage2-Double] initializing...\n";
    std::cout << "[Stage3-Plus2] initializing...\n";
    std::cout << "[FINAL] initializing...\n";
    std::cout.flush();
}