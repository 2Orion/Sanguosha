# Meitu（美图）项目 MVVM 架构分析

> 项目路径：`example/Meitu/`
> 一个基于 FLTK 的简易图像查看器，完整演示了 MVVM 架构在 C++ 桌面应用中的实现。

---

## 1. 项目结构

```
Meitu/
├── code/
│   ├── main.cpp                 # 入口：创建 App → 显示窗口 → 进入事件循环
│   ├── app/
│   │   └── PrettyApp.h/cpp      # 组合根（Composition Root）：创建并绑定三层
│   ├── common/                  # MVVM 基础设施
│   │   ├── command_base.h       # ICommandBase 接口（View → ViewModel 的抽象）
│   │   ├── frame.h/cpp          # IPropertyNotification + PropertyTrigger（观察者模式）
│   │   └── property_ids.h       # 属性 ID 枚举
│   ├── model/
│   │   ├── ImageModel.h/cpp     # Model：图像数据 + 加载逻辑
│   │   └── base/                # 纯数据结构
│   ├── viewmodel/
│   │   ├── ImageViewModel.h/cpp # ViewModel：Model 数据 → View 可消费的格式
│   │   └── commands/            # Command 对象
│   └── view/
│       └── MainWindow.h/cpp     # View：FLTK 窗口、按钮、图片显示
```

**核心文件数**：14 个（头文件 + 实现）

---

## 2. 架构总览

```
┌─────────── PrettyApp（组合根）──────────────────────┐
│                                                       │
│  ┌─────────┐     PropertyTrigger     ┌───────────┐   │
│  │  Model   │─────── add/notify ────►│ ViewModel  │   │
│  │ImageModel│     IPropertyNotification              │   │
│  └─────────┘                         └──────┬──────┘   │
│                                              │          │
│                                   PropertyTrigger       │
│                                     add/notify          │
│                                              │          │
│                                              ▼          │
│  ┌─────────┐     ICommandBase     ┌───────────┐        │
│  │  View   │◄────── exec ────────│ ViewModel  │        │
│  │MainWnd  │                      │(commands)  │        │
│  └─────────┘                      └───────────┘        │
│                                                       │
└───────────────────────────────────────────────────────┘
```

### 依赖方向

```
View ──► ICommandBase(IPropertyNotification) ◄── ViewModel ──► Model
 │                                                        │
 └─────────── 同一进程，没有独立编译 ──────────────────────┘
```

与经典 MVVM 不同，此项目 **没有** 在 View 和 ViewModel 之间强制编译期隔离——ViewModel 直接 `#include "ImageModel.h"`，View 直接接受 ViewModel 暴露的指针。解耦是通过接口抽象（`ICommandBase` + `IPropertyNotification`）实现的。

---

## 3. 各层详解

### 3.1 Model 层（`ImageModel`）

```cpp
class ImageModel {
public:
    bool load(const char *file);           // 加载 PNG 文件
    const ImageDataT<ColorPixel> *get_data() const;  // 获取原始像素数据
    PropertyTrigger& get_trigger();         // 事件触发器
private:
    ImageDataT<ColorPixel> m_data;         // 原始像素数据
    PropertyTrigger m_trigger;              // 属性变更通知
};
```

**职责**：
- 管理图像的核心数据（像素缓冲区）
- 实现图像加载逻辑（`load()`）
- 加载完成后通过 `PropertyTrigger` 通知外部

**特点**：
- 纯数据 + 业务逻辑，不引用任何 UI 类型
- 通过 `PropertyTrigger::fire(PROP_ID_IMAGE)` 发出变更通知
- 不知道自己被谁监听

### 3.2 ViewModel 层（`ImageViewModel`）

```cpp
class ImageViewModel : public IPropertyNotification,          // 监听 Model
                       public LoadCommand<ImageViewModel>      // 暴露 Command
{
public:
    const Fl_RGB_Image*const* get_image() const;       // 暴露给 View 的显示数据
    ICommandBase* get_load_command();                   // 暴露给 View 的 Command
    IPropertyNotification* get_notification();          // 暴露给 Model 的通知接口
    PropertyTrigger& get_trigger();                     // 自己也有 trigger
    void set_image_model(ImageModel* m);                // 绑定 Model
    ImageModel* get_image_model() const;

private:
    void on_property_changed(uint32_t id) override;    // Model → ViewModel
    void convert_image_data(const ImageDataT<ColorPixel> *pd);  // 数据转换

    ImageModel *m_image_model;      // 持有 Model 指针
    Fl_RGB_Image *m_pimage;         // 转换后的显示用图像
    PropertyTrigger m_trigger;      // 自己的事件触发器
};
```

**职责**：
- 监听 Model 的变更 → 转换数据 → 触发自己的通知
- 将 Model 的原始像素数据转换为 View 可以直接显示的 `Fl_RGB_Image*`
- 通过 `ICommandBase` 暴露操作给 View

#### 数据转换（核心职责）

```
Model 层：ImageDataT<ColorPixel>     ← 原始 RGB 像素数组
                    ↓
ViewModel：Fl_RGB_Image*              ← FLTK 可显示的图像格式
                    ↓
View 层：Fl_Box::image()              ← 直接显示
```

```cpp
void ImageViewModel::on_property_changed(uint32_t id) {
    switch (id) {
    case PROP_ID_IMAGE: {
        const ImageDataT<ColorPixel> *pd = m_image_model->get_data();
        convert_image_data(pd);          // Model 格式 → View 格式
    } break;
    }
    m_trigger.fire(id);                  // 转发通知给 View
}

void ImageViewModel::convert_image_data(const ImageDataT<ColorPixel> *pd) {
    delete m_pimage;
    m_pimage = ImageConvertHelper::to_fl_image(           // 转换
        pd->get_address(), pd->get_width(), pd->get_height(), sizeof(ColorPixel));
}
```

### 3.3 View 层（`MainWindow`）

```cpp
class MainWindow : public Fl_Window, public IPropertyNotification {
public:
    void set_image(const Fl_RGB_Image*const* pp);     // 接收 ViewModel 的数据指针
    void set_load_command(ICommandBase *p);             // 接收 Command
    IPropertyNotification* get_notification();          // 暴露给 ViewModel

protected:
    void on_property_changed(uint32_t id) override;    // ViewModel → View
    static void btn_load_cb(Fl_Widget *, void *);      // 按钮回调

    const Fl_RGB_Image*const* m_ppimage;               // 指向 ViewModel 的图像指针
    ICommandBase *m_load_command;                        // 指向 ViewModel 的 Command
};
```

**职责**：
- 绘制界面（窗口、按钮、图片框）
- 用户交互（按钮点击 → Command::exec）
- 响应 ViewModel 的通知更新 UI

#### View → ViewModel 通信：Command 模式

```cpp
// 按钮回调 — 通过 Command 对象间接调用 ViewModel
void MainWindow::btn_load_cb(Fl_Widget *, void *pv) {
    MainWindow *pThis = (MainWindow *)pv;
    Fl_Native_File_Chooser dlg;
    dlg.title("Pick a png file");
    dlg.filter("PNG\t*.png");
    if (dlg.show() == 0) {
        if (pThis->m_load_command != nullptr) {
            int result = pThis->m_load_command->exec(
                std::make_any<const char*>(dlg.filename()));
            if (result == 0) {
                fl_alert("Cannot open file!");
            }
        }
    }
}
```

**关键点**：View 持有 `ICommandBase*` 指针，调用 `exec(any)`。View **不知道** Command 的内部实现，也不持有 ViewModel 指针。

#### ViewModel → View 通信：PropertyTrigger 观察者

```cpp
void MainWindow::on_property_changed(uint32_t id) {
    switch (id) {
    case PROP_ID_IMAGE:
        m_box.image(const_cast<Fl_RGB_Image*>(*m_ppimage));  // 更新显示
        m_box.redraw();
        break;
    }
}
```

View 通过 `m_ppimage` 指针**实时读取** ViewModel 的最新图像数据，不需要额外的 getter 调用。

### 3.4 组合根（`PrettyApp`）

```cpp
class PrettyApp {
    MainWindow m_main_wnd;         // View
    ImageViewModel m_image_viewmodel;  // ViewModel
    ImageModel m_image_model;      // Model
};

PrettyApp::PrettyApp() : m_main_wnd(800, 700, "Mei Tu") {
    // 1. Model → ViewModel：ViewModel 监听 Model
    m_image_viewmodel.set_image_model(&m_image_model);
    m_image_model.get_trigger().add(m_image_viewmodel.get_notification());

    // 2. ViewModel → View：View 监听 ViewModel
    m_image_viewmodel.get_trigger().add(m_main_wnd.get_notification());

    // 3. ViewModel → View：View 获取 ViewModel 的数据指针
    m_main_wnd.set_image(m_image_viewmodel.get_image());

    // 4. View → ViewModel：View 获取 ViewModel 的 Command
    m_main_wnd.set_load_command(m_image_viewmodel.get_load_command());
}
```

**PrettyApp 作为组合根承担了全部的依赖注入和绑定**。三层在 PrettyApp 的构造函数中通过接口指针连接，之后各层只通过接口通信，互不了解具体实现。

---

## 4. 通信机制分析

### 4.1 三种通信路径

#### 路径 A：View → ViewModel（Command 模式）

```
View::btn_load_cb
  → m_load_command->exec(filename)          // ICommandBase 接口
    → LoadCommand<ImageVM>::exec(filename)
      → static_cast<T*>(this)->get_image_model()->load(filename)  // CRTP
```

**特点**：
- View 持有 `ICommandBase*`，不知道具体 Command 类
- 使用 `std::any` 传递参数，类型擦除
- 使用 CRTP（`LoadCommand<ImageViewModel>`）避免虚函数开销

#### 路径 B：Model → ViewModel（观察者模式）

```
Model::load()
  → m_trigger.fire(PROP_ID_IMAGE)
    → IPropertyNotification::on_property_changed(PROP_ID_IMAGE)
      → ImageViewModel::on_property_changed(PROP_ID_IMAGE)
        → convert_image_data(pd)        // 数据转换
        → m_trigger.fire(PROP_ID_IMAGE) // 转发
```

**特点**：
- 使用整数 ID（`PROP_ID_IMAGE`）标识不同属性，而非类型安全的 EventListener
- 一个统一的 `on_property_changed(uint32_t)` 回调处理所有变化
- `PropertyTrigger` 内部使用 `vector<IPropertyNotification*>` 管理监听者

#### 路径 C：ViewModel → View（观察者模式）

```
ImageViewModel::on_property_changed
  → m_trigger.fire(PROP_ID_IMAGE)
    → MainWindow::on_property_changed(PROP_ID_IMAGE)
      → m_box.image(*m_ppimage) + redraw()
```

**特点**：
- 与路径 B 使用完全相同的机制（`PropertyTrigger` + `IPropertyNotification`）
- View 通过指针 `m_ppimage` 直接读取 ViewModel 的最新数据，无需额外查询

### 4.2 事件机制实现细节

```cpp
// IPropertyNotification — 观察者接口
class IPropertyNotification {
public:
    virtual void on_property_changed(uint32_t id) = 0;
};

// PropertyTrigger — 事件源
class PropertyTrigger {
public:
    uintptr_t add(IPropertyNotification *pn);     // 注册观察者
    void remove(uintptr_t cookie);                 // 注销
    void fire(uint32_t id);                         // 通知所有观察者
private:
    std::vector<IPropertyNotification *> m_vec_nf;  // 观察者列表
};
```

`fire` 遍历整个列表，逐个调用 `on_property_changed`。这是一种**同步广播**——所有观察者在 `fire` 返回之前都执行完毕。

### 4.3 Command 实现细节

```cpp
// ICommandBase — 命令接口
class ICommandBase {
public:
    virtual int exec(const std::any& v) = 0;  // std::any 实现类型擦除
};

// CRTP 实现具体命令
template <class T>
class LoadCommand : public ICommandBase {
    int exec(const std::any& v) override {
        T* pT = static_cast<T*>(this);                 // 向上转型到派生类
        if (!pT->get_image_model()->load(
                std::any_cast<const char*>(v)))
            return 0;
        return 1;
    }
};
```

**CRTP（奇异递归模板模式）**：
- `LoadCommand<ImageViewModel>` 继承 `ICommandBase`
- `ImageViewModel` 多重继承 `LoadCommand<ImageViewModel>`
- Command 的 `exec()` 通过 `static_cast<T*>(this)` 访问 `ImageViewModel` 的方法
- 避免了为每个命令单独写一个转发函数的 boilerplate

---

## 5. 解耦手段总结

### 5.1 使用了哪些解耦手段

| 手段 | 应用 | 解耦效果 |
|------|------|---------|
| **接口抽象** | `ICommandBase`、`IPropertyNotification` | View 和 ViewModel 之间只依赖接口，不依赖具体类 |
| **Command 模式** | `LoadCommand<ImageViewModel>` | View 不调用 ViewModel 的方法，而是调用 Command |
| **观察者模式** | `PropertyTrigger` | Model 和 ViewModel 都是事件源，通过回调通知变化 |
| **CRTP** | `LoadCommand<T>` | 命令实现无需为每个 ViewModel 类型写适配代码 |
| **std::any 类型擦除** | `exec(const std::any&)` | Command 参数类型在运行时决定，编译期不耦合 |
| **组合根** | `PrettyApp` | 所有依赖集中在一点绑定，各层不知晓彼此的创建过程 |
| **数据指针传递** | `get_image()` 返回指针 | View 直接读取 ViewModel 的数据，而非每次调用 getter |

### 5.2 解耦的代价

| 代价 | 表现 |
|------|------|
| **运行时类型不安全** | `std::any_cast` 失败抛异常，无编译期检查 |
| **整数 ID 无类型约束** | `PROP_ID_IMAGE` 拼写错误无法被编译器发现 |
| **单事件通道** | 所有属性变更共用一个 `on_property_changed`，需要 switch-case 分发 |
| **CRTP 增加了继承复杂度** | ViewModel 需要多重继承，模版错误信息难读 |

---

## 6. 与本项目的对比

| 对比维度 | Meitu（示例） | Sanguosha（本项目） |
|----------|-------------|-------------------|
| **UI 框架** | FLTK（C++ GUI 库） | Qt 6 Widgets |
| **Model → VM** | `PropertyTrigger` + 整数 ID | 类型安全 `EventListener<Args...>` |
| **VM → View** | 同上 | 同上 + `Qt::QueuedConnection` |
| **View → VM** | `ICommandBase::exec(any)` | 直接方法调用（`avm->playCard(...)`） |
| **参数类型** | 类型擦除（`std::any`） | 类型安全（`int`, `PhaseType`） |
| **数据绑定** | 暴露指针，View 直接读取 | 通过方法查询 + ViewModel 创建临时对象 |
| **事件参数** | 统一 `uint32_t id`，自行 switch | 每个事件有独立类型参数 |
| **Command 模式** | ✅ 使用 | ❌ 未使用，直调 VM 方法 |
| **组合根** | ✅ `PrettyApp` 构造函数 | ⚠️ `MainWindow::onStartGame()` |

### 关键差异分析

**1. Command 模式的使用**

Meitu 使用了 Command 模式（`ICommandBase`），而本项目直接调用 ViewModel 方法。两种方式各有优劣：

```
Meitu 方式：                   本项目方式：
Command::exec(any)              avm->playCard(cardId, playerId, targets)
  ├── 更符合严格 MVVM            ├── 更直接、更易读
  ├── 支持 Undo/Redo 扩展        ├── 类型安全（编译期检查参数）
  └── 更好的 View → VM 解耦      └── 更少 boilerplate 代码
```

对于卡片游戏这种操作类型可穷举且不需要 Undo 的场景，直接方法调用是合理的选择。

**2. 事件机制的类型安全**

```
Meitu：                        本项目：
PropertyTrigger::fire(uint32_t) EventListener<PhaseType>::notify(PhaseType)
  ├── 统一 ID，简单                ├── 类型安全，编译期检查
  ├── switch-case 分发麻烦         ├── 每个事件独立通道，无需分发
  └── ID 冲突需人工管理            └── 新增事件不需要改已有代码
```

本项目使用模板参数 `EventListener<Args...>` 为每个事件提供了独立的类型信息。这是一种编译期开销换取运行时安全的设计。

**3. View 对 ViewModel 的依赖方式**

```
Meitu：View 不知道 ViewModel 类名   本项目：View 持有 GameViewModel*
  只通过 ICommandBase +               直接调用方法、读取属性
  IPropertyNotification 接口通信
```

本项目 View 对 ViewModel 的耦合更紧，但获得了更好的 IDE 支持和编译期检查。对于一个已知的、不变的 ViewModel，这种耦合是可控的。

---

## 7. 结论

Meitu 项目展示了一套 **轻量级 MVVM 架构**：

- **优点**：接口清晰、依赖方向单向、Command 模式实现了 View → ViewModel 的解耦
- **设计哲学**：使用接口抽象代替编译期隔离，在 C++ 的类型系统之上用 `std::any` 和整数 ID 实现运行时多态
- **与本项目的关系**：Meitu 展示了更"教科书式"的 MVVM（有 Command 模式），而本项目在保持 MVVM 核心原则的基础上，根据游戏应用的特点做了简化（跳过 Command 模式、使用类型安全的事件通道）。两者都是合理的 MVVM 实践，只是在不同约束条件（框架、应用类型、复杂度）下的不同权衡。
