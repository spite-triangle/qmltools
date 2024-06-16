#ifndef QSINGLETON_HPP
#define QSINGLETON_HPP

#include <mutex>
#include <QPointer>


template<class Type>
class QSingleton
{
public:
    using Ptr = QPointer<Type>; 
public:
    static Ptr Instance(){
        if(m_instance == nullptr){
            std::lock_guard<std::mutex> lock(m_mut);
            if(m_instance == nullptr){
                m_instance = new Type();
            }
        }
        return m_instance;
    }

    static void Release(){
        m_instance.clear();
    }

private:

    static Ptr m_instance;
    static std::mutex m_mut;
};

template<class Type>
typename QSingleton<Type>::Ptr  QSingleton<Type>::m_instance = nullptr;

template<class Type>
std::mutex QSingleton<Type>::m_mut;

#endif // QSINGLETON_HPP
