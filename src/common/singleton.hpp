#ifndef SINGLETON_HPP
#define SINGLETON_HPP

#include <mutex>
#include <memory>

template<class Type>
class Singleton
{
public:
    using Ptr = std::shared_ptr<Type>; 
public:
    static Ptr Instance(){
        if(m_instance == nullptr){
            std::lock_guard<std::mutex> lock(m_mut);
            if(m_instance == nullptr){
                m_instance = std::make_shared<Type>();
            }
        }
        return m_instance;
    }

    static void Release(){
        m_instance.reset(nullptr);
    }

private:

    static Ptr m_instance;
    static std::mutex m_mut;
};

template<class Type>
typename Singleton<Type>::Ptr  Singleton<Type>::m_instance = nullptr;

template<class Type>
std::mutex Singleton<Type>::m_mut;

#endif /* SINGLETON_HPP */
