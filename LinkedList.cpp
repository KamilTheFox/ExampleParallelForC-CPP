#include "LinkedList.h"
#include <iostream>

LinkedList::LinkedList() : head(nullptr) {}

LinkedList::~LinkedList() 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    Node* current = head;
    while (current != nullptr) 
    {
        Node* next = current->next;
        delete current;
        current = next;
    }
}

void LinkedList::insert(int value) 
{
    std::lock_guard<std::recursive_mutex> lock(mtx); 
    
    Node* newNode = new Node(value);
    newNode->next = head;
    head = newNode;
}

void LinkedList::insertAfter(int targetValue, int newValue) 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    
    Node* current = head;
    while (current != nullptr) 
    {
        if (current->value == targetValue) 
        {
            Node* newNode = new Node(newValue);
            newNode->next = current->next;
            current->next = newNode;
            return;
        }
        current = current->next;
    }
    
    // Если target не найден, вставляем в начало без рекурсивного вызова insert()
    insert(newValue);
}

bool LinkedList::remove(int value) 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    
    if (head == nullptr) return false;
    
    if (head->value == value) 
    {
        Node* temp = head;
        head = head->next;
        delete temp;
        return true;
    }
    
    Node* current = head;
    while (current->next != nullptr) 
    {
        if (current->next->value == value) 
        {
            Node* temp = current->next;
            current->next = current->next->next;
            delete temp;
            return true;
        }
        current = current->next;
    }
    
    return false;
}

bool LinkedList::find(int value) 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    
    Node* current = head;
    while (current != nullptr) 
    {
        if (current->value == value) 
        {
            return true;
        }
        current = current->next;
    }
    return false;
}

void LinkedList::print() const 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    
    std::cout << "List: ";
    Node* current = head;
    while (current != nullptr) 
    {
        std::cout << current->value;
        if (current->next != nullptr) 
        {
            std::cout << " -> ";
        }
        current = current->next;
    }
    std::cout << " -> NULL" << std::endl;
}

int LinkedList::size() const 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    
    int count = 0;
    Node* current = head;
    while (current != nullptr) 
    {
        count++;
        current = current->next;
    }
    return count;
}

bool LinkedList::empty() const 
{
    std::lock_guard<std::recursive_mutex> lock(mtx);
    return head == nullptr;
}