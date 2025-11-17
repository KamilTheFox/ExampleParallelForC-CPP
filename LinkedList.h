#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include <mutex>

class LinkedList 
{
private:
    struct Node 
    {
        int value;
        Node* next;
        Node(int val) : value(val), next(nullptr) {}
    };
    
    Node* head;
    mutable std::recursive_mutex mtx;

public:
    LinkedList();
    ~LinkedList();
    
    void insert(int value);                    // Вставка в начало
    void insertAfter(int targetValue, int newValue); // Вставка после target
    bool remove(int value);                    // Удаление по значению
    bool find(int value);                      // Поиск элемента

    void print() const;                        // Вывод списка
    int size() const;                          // Размер списка
    bool empty() const;                        // Проверка на пустоту
};

#endif