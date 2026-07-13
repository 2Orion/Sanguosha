#ifndef CORE_EVENT_H
#define CORE_EVENT_H

#include <functional>
#include <vector>
#include <algorithm>

/// 简单的事件监听器，替代 Qt 信号槽系统
/// 使用观察者模式实现

template<typename... Args>
class EventListener {
public:
    using Callback = std::function<void(Args...)>;
    using CallbackId = size_t;

    /// 连接回调函数，返回连接 ID
    CallbackId connect(Callback callback) {
        CallbackId id = nextId++;
        callbacks.push_back({id, callback});
        return id;
    }

    /// 断开连接
    void disconnect(CallbackId id) {
        callbacks.erase(
            std::remove_if(callbacks.begin(), callbacks.end(),
                [id](const CallbackPair& pair) { return pair.id == id; }),
            callbacks.end()
        );
    }

    /// 通知所有回调
    /// 复制回调列表后遍历，防止回调中调用 connect/disconnect 导致迭代器失效
    void notify(Args... args) {
        auto callbacksCopy = callbacks;
        for (const auto& pair : callbacksCopy) {
            pair.callback(args...);
        }
    }

    /// 清除所有回调
    void clear() {
        callbacks.clear();
    }

private:
    struct CallbackPair {
        CallbackId id;
        Callback callback;
    };
    CallbackId nextId = 1;
    std::vector<CallbackPair> callbacks;
};

#endif // CORE_EVENT_H
