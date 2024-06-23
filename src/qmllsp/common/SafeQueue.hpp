#ifndef SAFEQUEUE_HPP
#define SAFEQUEUE_HPP

#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>


template <typename T>
class SafeQueue
{
public:
    using ElementPtr = std::shared_ptr<T>;

public:
    SafeQueue(size_t uCapacity = 1024)
        : m_uCapacity(uCapacity)
    {
    }

    ElementPtr waitPop(){
        std::unique_lock<std::mutex> lock(m_mut);
        while(m_queue.empty()){
            m_cond.wait(lock);
        }

        auto top = m_queue.front();
        m_queue.pop();
        m_cond.notify_all();
        return top;
    }

    void waitPush(const T & element){
        ElementPtr elem = std::make_shared<T>(element);

        std::unique_lock<std::mutex> lock(m_mut);
        while(m_queue.size() >= m_uCapacity){
            m_cond.wait(lock);
        }

        m_queue.push(elem);
        m_cond.notify_all();
    }

    void pushNull(){
        std::unique_lock<std::mutex> lock(m_mut);
        m_queue.push(ElementPtr());
        m_cond.notify_all();
    }

    bool isEmpty(){
        std::lock_guard<std::mutex> clsGuard(m_mut);
        return m_queue.empty();
    }

private:
    mutable std::mutex m_mut;
    std::condition_variable m_cond;
    std::queue<ElementPtr> m_queue;
    size_t m_uCapacity; 
};

#endif // SAFEQUEUE_HPP
