# debugger

连接调试器

```cpp
QmlDebugConnectionManager::connectToServer
```

QmlDebug::QmlDebugConnectionManager 创建连接，给  QmlDebug::QmlDebugClient 使用

qmlengine.cpp 下的 QmlEnginePrivate : public QmlDebugClient 就是调试器实现