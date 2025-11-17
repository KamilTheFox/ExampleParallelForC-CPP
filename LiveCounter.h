#ifndef LIVECOUNTER_H
#define LIVECOUNTER_H

#include <string>
#include <map>
#include <mutex>

class LiveCounter 
{
private:
    std::map<std::string, int> counters;
    std::map<std::string, std::string> last_values;
    std::mutex mtx;
    
public:
    void update(const std::string& key, const std::string& message);
    void refresh_line(const std::string& key, const std::string& full_message);
    void init_display();
    void init_display_pipeline(); 
};

#endif