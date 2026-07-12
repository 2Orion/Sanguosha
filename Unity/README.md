# 三国杀 Unity 移植

从 C++ 控制台版完整移植到 Unity (C#) 的三国杀卡牌游戏。

## 架构

```
Assets/Scripts/
├── Model/              # 纯 C# 游戏逻辑（零 Unity 依赖）
│   ├── Signal.cs       # 类型安全事件系统
│   ├── Enums.cs        # 所有枚举定义
│   ├── Card.cs         # 卡牌基类 + 10 种子类
│   ├── Character.cs    # 武将（曹操、关羽、张飞、赵云）
│   ├── Player.cs       # 玩家（体力v、手牌、事件）
│   ├── CardManager.cs  # 牌堆管理（抽/弃/洗牌）
│   ├── GameState.cs    # 游戏核心状态
│   ├── GameRule.cs     # 游戏规则（伤害、濒死、AOE）
│   └── PendingActionInfo.cs  # 待定动作数据
├── ViewModel/          # MVVM 中间层（连接 Model 和 View）
│   ├── CardViewModel.cs
│   ├── PlayerViewModel.cs
│   ├── ActionViewModel.cs
│   └── GameViewModel.cs
└── View/               # Unity MonoBehaviour（UI 层）
    ├── GameView.cs          # 主游戏控制器
    ├── CardWidget.cs        # 单张卡牌 UI
    └── PlayerInfoWidget.cs  # 玩家信息 UI
```

## 快速开始

### 1. 创建 Unity 项目

- Unity 2021.3+ (LTS)
- 安装 **TextMeshPro** (Window → TextMeshPro → Import TMP Essentials)

### 2. 导入脚本

将 `Scripts/` 下所有文件拖入 Unity 项目的 `Assets/Scripts/` 目录。

### 3. 搭建场景

1. **创建 Canvas** — GameObject → UI → Canvas
2. **创建 GameManager** — 空 GameObject，挂载 `GameView.cs`
3. **创建卡牌预制体**：

```
Card (Prefab)
├── CardBg (Image)          — 卡牌背景
├── SuitText (TMP_Text)     — 花色符号
├── NumberText (TMP_Text)   — 点数
├── NameText (TMP_Text)     — 卡牌名
├── DescText (TMP_Text)     — 描述（可选）
├── SelectedMark (GameObject)   — 选中高亮
├── PlayableGlow (GameObject)   — 亮边
└── Button — 点击响应
```

4. **创建玩家信息预制体**：

```
PlayerInfo (Prefab)
├── NameText (TMP_Text)
├── CharacterText (TMP_Text)
├── SkillText (TMP_Text)
├── HpSlider (Slider)
├── HpText (TMP_Text)
├── CardCountText (TMP_Text)
└── TurnHighlight (GameObject)
```

5. **在 GameView 上拖拽绑定**：
   - `Player 1 Info` / `Player 2 Info` → PlayerInfoWidget 实例
   - `Hand Cards Parent` → 手牌区域的 Transform
   - `Card Prefab` → Card 预制体
   - 其他 UI 引用按需绑定

### 4. 运行

点击 Play，游戏自动开始。

## 与 C++ 原版的差异

| 项目 | C++ | C# (Unity) |
|------|-----|------------|
| 内存管理 | `unique_ptr` / `new` / `delete` | GC 自动管理 |
| 事件系统 | `EventListener<T>` (自定义) | `Signal<T>` (等效实现) |
| 集合 | `std::vector` | `List<T>` |
| 空值 | `nullptr` | `null` |
| 卡片创建 | `new KillCard(...)` | `new KillCard(...)` |
| 随机数 | `RandomUtils` | `System.Random` |

## 扩展指南

### 添加新武将

1. 在 `Character.cs` 中新建类继承 `Character`
2. 重写 `TriggerCondition()` / `TriggerSkill()` / `SkillTransformCard()`
3. 在 `GameViewModel.CreateCharacterById()` 中注册

### 添加新卡牌

1. 在 `Card.cs` 中新建类继承 `Card` 或 `StrategyCard`
2. 重写 `CanUse()` / `CanTarget()` / `Execute()`
3. 在 `CardManager.Initialize()` 中添加卡牌数量
4. 在 `CardManager.CreateCard()` 工厂方法中注册

### 添加网络联机

Model 层已完全分离（无 Unity 依赖），可以直接：
1. 服务器端复用 Model 层运行游戏逻辑
2. 客户端通过 WebSocket 接收 GameState 快照
3. ViewModel 层接收远程状态更新
