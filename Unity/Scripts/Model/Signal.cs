using System;
using System.Collections.Generic;

/// <summary>
/// 类型安全的信号/事件系统，支持连接、断开和清空。
/// 对应 C++ 的 EventListener&lt;T&gt;。
/// </summary>
public class Signal
{
    private int _nextId = 0;
    private readonly Dictionary<int, Action> _callbacks = new();

    public int Connect(Action callback)
    {
        int id = _nextId++;
        _callbacks[id] = callback;
        return id;
    }

    public void Disconnect(int id)
    {
        _callbacks.Remove(id);
    }

    public void Emit()
    {
        foreach (var cb in _callbacks.Values)
            cb?.Invoke();
    }

    public void Clear()
    {
        _callbacks.Clear();
    }

    public int ListenerCount => _callbacks.Count;
}

/// <summary>
/// 单参数信号
/// </summary>
public class Signal<T>
{
    private int _nextId = 0;
    private readonly Dictionary<int, Action<T>> _callbacks = new();

    public int Connect(Action<T> callback)
    {
        int id = _nextId++;
        _callbacks[id] = callback;
        return id;
    }

    public void Disconnect(int id)
    {
        _callbacks.Remove(id);
    }

    public void Emit(T arg)
    {
        foreach (var cb in _callbacks.Values)
            cb?.Invoke(arg);
    }

    public void Clear()
    {
        _callbacks.Clear();
    }

    public int ListenerCount => _callbacks.Count;
}

/// <summary>
/// 双参数信号
/// </summary>
public class Signal<T1, T2>
{
    private int _nextId = 0;
    private readonly Dictionary<int, Action<T1, T2>> _callbacks = new();

    public int Connect(Action<T1, T2> callback)
    {
        int id = _nextId++;
        _callbacks[id] = callback;
        return id;
    }

    public void Disconnect(int id)
    {
        _callbacks.Remove(id);
    }

    public void Emit(T1 arg1, T2 arg2)
    {
        foreach (var cb in _callbacks.Values)
            cb?.Invoke(arg1, arg2);
    }

    public void Clear()
    {
        _callbacks.Clear();
    }

    public int ListenerCount => _callbacks.Count;
}
