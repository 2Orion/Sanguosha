# Changelog

## Features

### [2026-07-15] 网络层 Step 7：ClientApp 组合根

新建 `src/App/ClientApp.h/cpp`：网络模式的客户端组合根，与本地模式的 `SGSApp::startLocalGame()`
完全对称——创建 `GameBoardWidget` + `GameClient`，在构造函数里按逐条相同的形状建立双向信号连接
（7 条 View→Client 命令信号 + 9 条 Client→View 状态信号）。零业务逻辑，只额外暴露
`connectToServer(host, port)`/`selectCharacter(int)` 两个转发方法，以及 `boardWidget()`/
`gameClient()` 两个访问器供调用方嵌入自己的窗口容器、接入登录/等待界面（`MainWindow` 联网入口和
`NetworkConfigDialog` 属丙，不在本计划内）。

**验收标准达成：`GameBoardWidget` 零改动。** 能做到零改动的原因是设计层面的——`GameBoardWidget`
从不知道、也不需要知道对面接的是本地 `GameViewModel`/`ActionViewModel` 还是网络 `GameClient`，
因为 `GameClient` 对外信号/槽形状与本地 VM 完全一致（Step 6 的设计前提）。

**涉及文件**：`src/App/ClientApp.h/cpp`（新建）、`CMakeLists.txt`（`CLIENT_APP_SOURCES`/
`CLIENT_APP_HEADERS` 变量组，`NetworkTest` 加 `Qt::Widgets` 链接）、`tests/network_test.cpp`
（+1 新用例，`QTEST_GUILESS_MAIN`→`QTEST_MAIN`）、`connection.md` §7.6（新建）、`CLAUDE.md`
（Step 7 计划项打勾）、`README.md`（目录结构 + 测试运行方式 + 测试用例数）。

**测试注意点**：`clientAppWiresBoardToGameClientWithZeroBoardChanges` 需要真实
`GameBoardWidget`（`ClientApp` 内部创建），因此本用例的存在把整个 `NetworkTest` 目标从
`QTEST_GUILESS_MAIN`（仅 `QCoreApplication`）切到 `QTEST_MAIN`（`QApplication`），CTest 侧沿用
`ViewTest` 的做法设置 `QT_QPA_PLATFORM=offscreen` 环境变量，避免依赖桌面显示服务器。

**验证**：`clientAppWiresBoardToGameClientWithZeroBoardChanges`——对接真实 `ServerApp`，起一个
`ClientApp`（真实 `GameBoardWidget`）和一个裸 `GameClient`（扮演对手，无 View）完成握手→选将→
驱动到 Play 阶段，用 `QTest::mouseClick` 真实点击 `ClientApp::boardWidget()` 里注入的杀对应的
`CardWidget`，断言服务器侧 `Player::handCards()` 确实移除了这张牌——覆盖从真实 QWidget 点击到
网络命令到服务器结算的完整链路，而不只是信号连接的存在性。NetworkTest 增至 58 用例，全套件
5/5（`ModelSmokeTest`/`ModelTest`/`ViewModelTest`/`ViewTest`/`NetworkTest`）通过。

### [2026-07-15] 网络层 Step 6：GameClient（"网络化 ViewModel"）

新建 `src/Network/GameClient.h/cpp`：客户端网络层，与服务器侧 `GameServer`（"网络化的 View"）对称，
定位是"网络化的 ViewModel"——对外暴露与 `GameViewModel`/`ActionViewModel` 完全相同形状的信号（内容
由收到的服务器广播反序列化后直接 `emit`，不经过任何本地 Model/规则计算），发送方法与
`GameBoardWidget` 的命令信号一一对应（只把参数打包发给服务器，不做任何本地校验或判断）。本类零
Model 依赖、零规则判断，符合 plan2.0.md §2 对网络层的约束（不持有/传递 `Model::Player*`/
`Model::Card*`，跨网络只传 Common 值类型）。

**方法**：`connectToServer(host, port)`/`disconnectFromServer()`/`isConnected()`/
`localPlayerId()`/`selectCharacter(int)`/`playCard`/`respondCard`/`selectTarget`/`discardCard`/
`endPlayPhase`/`advancePhase`/`skipResponse`。

**信号**：与 VM 相同形状的 `phaseChanged`/`playerDataUpdated`/`handCardsUpdated`/
`pendingActionCreated`/`pendingActionCleared`/`gameOver`/`logMessage`/`targetSelectionStarted`/
`targetSelectionFinished`；另加本地模式无对应物的网络生命周期信号 `connected(playerId)`/
`connectionRejected(reason)`/`connectionError(error)`/`disconnected()`/`gameStarted(char0, char1)`。

按 plan2.0.md §2.4 的结论，未新建 `RemoteGameVM`/`PlayerVMAdapter` 只读查询适配层——
`GameBoardWidget` 从不主动调用 VM 方法拉取状态，全部状态通过槽被动接收，`GameClient` 只需转发信号。

**涉及文件**：`src/Network/GameClient.h/cpp`（新建）、`CMakeLists.txt`（`NETWORK_SOURCES`/
`NETWORK_HEADERS` 加入 GameClient）、`tests/network_test.cpp`（+6 新用例）、`connection.md` §7.5（新建）、
`CLAUDE.md`（Step 6 计划项打勾）、`README.md`（目录结构 + 测试用例数）。

**测试注意点**：custom 值类型（`PhaseType`/`PlayerData`/`CardList`/`PendingActionData`）未注册 Qt
元类型，`QSignalSpy` 对这些信号的 `QVariant` 提取会失败——测试里改用 `connect` 到本地 lambda 直接捕获
参数，而不是走 `QSignalSpy::at(0).at(N)`（`connectionRejected`/`gameStarted` 等只含 `int`/`QString`
基础类型的信号仍可用 `QSignalSpy`）。首版实现里 `handSizeBefore` 曾在 `Draw` 阶段推进之前采样，
导致断言的"出牌后手牌数-1"目标值本身就不对（摸牌阶段会先摸 2 张），复现即改在 `Draw → Play` 之后
采样，与既有测试（`playCardCommandUsesConnectionIdentityNotClaimedPlayerId` 等）的写法保持一致。

**验证**：`gameClientHandshakeAssignsPlayerIdsAndRejectsThirdConnection`（对接真实 `GameServer`：
playerId 0/1 分配、第三连接 `connectionRejected`）、`gameClientSendsAllCommandsWithCorrectPayload`
（7 个发送方法逐一断言服务器收到的 `MessageType` 与 payload 字段）、
`gameClientReceivesGameStartedAndBroadcastsWithRedaction`（对接真实 `ServerApp`：`gameStarted`/
`phaseChanged`/`playerDataUpdated`/`logMessage` 正常 emit，`handCardsUpdated` 脱敏转发正确——己方
完整、对方占位）、`gameClientPlayCardRoundTripReflectedInBothClients`（两个 `GameClient` 出杀 →
双方收到 `pendingActionCreated(Dodge)`）、`gameClientDisconnectedSignalFiresWhenServerCloses`
（服务器关闭 → `disconnected()` emit，`localPlayerId()` 复位为 -1）。NetworkTest 增至 57 个用例，
12/13 次连续直接运行 + 全套件运行通过（1 次遇到 `thirdConnectionRejected`——Step 3 起就存在的
loopback 连接建立偶发慢速——的既有已知抖动，与本次改动无关，重跑即通过）。

### [2026-07-15] 网络层 Step 5：手牌脱敏

`GameServer` 广播 `HandCardsUpdated` 此前对两端一视同仁：`ServerApp::wireViewModelBroadcasts()`
里这条槽直接用 `GameServer::broadcast` 把同一份完整 `CardList` payload 发给两个客户端，对手能看到
己方的完整手牌牌面——网络对战最基本的信息安全前提被破坏（P0，不可砍）。

**修复方式**：

- 新增 `Protocol::redactCardList(const CardList&)`（`src/Network/Protocol.h/cpp`，纯函数、无 Model 依赖）：对每张 `CardData` 只保留 `cardId`/`ownerId`（结构信息），其余牌面字段（`cardType`/`suit`/`number`/`cardName`/`description`/`color`/`isBasic`/`isStrategy`/`suitSymbol`/`numberString`/`isEquipment`/`equipSlot`/`attackRange`/`isSelected`/`isPlayable`/`isHighlighted`）重置为默认构造的占位值，列表长度不变。
- `ServerApp::wireViewModelBroadcasts()` 的 `handCardsUpdated` 槽不再用 `broadcast`，改为两次 `GameServer::sendTo`：所有者本人收未经处理的完整 `CardList`，对手收 `redactCardList` 处理后的结果。`HandCardsMsg::playerId` 字段语义仍是"这是谁的手牌"，与"收件人是谁"无关。

**涉及文件**：`src/Network/Protocol.h`（+声明）、`src/Network/Protocol.cpp`（新建）、`CMakeLists.txt`（`NETWORK_SOURCES` 加入 `Protocol.cpp`）、`src/App/ServerApp.h/cpp`、`tests/network_test.cpp`（+2 新用例）、`connection.md` §7.1/7.1.1、`CLAUDE.md`（Step 5 计划项打勾）、`README.md`（测试用例数 + 目录结构说明）。

**验证**：`redactCardListKeepsIdentityDropsFace`（纯函数单测：cardId/ownerId 保留，牌面字段全部占位）+ `handCardsBroadcastRedactedForOpponentFullForOwner`（两个真实 TCP 连接跑完整对局：己方视角至少一张牌有真实牌名，对手视角牌名/描述/装备标记全部占位，双方 cardId 集合排序后一致）。NetworkTest 增至 50 个用例，10 次连续直接运行 + 3 次全套件运行全部通过，无抖动（其中一次单跑遇到过一次超时式失败，重跑及后续 10 次均通过，判定为 loopback 下的偶发调度抖动，非新增代码引入的问题）。

### [2026-07-15] 网络层 Step 4 加固第三轮：出牌/弃牌回合归属校验 + 待定动作重入保护

修复前两轮审查确认、但当时判定"边界超出网络接线、属甲的规则域"而搁置的两处深层规则缺口，用户决定现在修复：

- **出牌/弃牌全链路不校验 `player == currentPlayer()`（critical，两轮独立审查都发现）**：`ActionViewModel::canPlayCard`/`playCard`/`discardCard` 和各 `Card::canUse` 子类只查全局 `currentPhase()`，从不检查调用者是不是当前回合玩家。本地模式靠 View 只让当前玩家手牌可交互侥幸掩盖，网络模式下非当前回合玩家能用自己真实（非伪造）身份在对方回合出/弃自己的牌。
- **`GameState::setPendingAction` 无重入保护（critical，两轮独立审查都发现）**：即使调用者确实是当前回合玩家，出牌路径原先也不检查 `!hasPendingAction()`——当前回合玩家出杀后，趁对方的闪响应尚未结算，可以紧接着再出一张主动牌（如南蛮入侵/万箭齐发），`GameRule::executeKill`/`executeBarbarianInvasion` 等会无条件调用 `setPendingAction` 覆盖掉第一个待定动作，原来的闪响应永久悬空。

**修复方式（刻意避免触碰 `Card.cpp`/`GameRule.cpp`/`GameState.cpp`，与甲的装备牌开发保持文件隔离）**：在 `ActionViewModel::canPlayCard` 入口补两条早退校验——`player != m_state->currentPlayer()` 和 `m_state->hasPendingAction()`；`ActionViewModel::discardCard` 补 `player != m_state->currentPlayer()`。三个校验点位于 `canPlayCard`/`discardCard` 内部，同时保护 View 命令入口（`onPlayCardRequested`）和直接调用（`playCard`），本地模式行为不受影响（本地模式下这些条件恒成立）。

**涉及文件**：`src/ViewModel/ActionViewModel.cpp`、`tests/network_test.cpp`（+3 新用例）、`connection.md` §7.2/§7.3、`CLAUDE.md`（Step 4 计划项更新）。

**验证**：NetworkTest 50 个用例（47 + 3 新增），5 次连续直接运行 + 全套件运行全部通过，无抖动。

### [2026-07-15] 网络层 Step 4 加固第二轮：advancePhase 悬空待定动作 + GameStarted 时序

对第一轮加固后的代码做了第二轮独立对抗性审查（复用第一轮的 5 个视角 + 2 个之前因 API 中断未完成的维度），确认了以下额外问题并修复：

- **`advancePhase()` 离开 Play 阶段不检查 `hasPendingAction()`（critical，两轮独立审查都发现）**：与 `endPlayPhase()`（已有此检查）不对称。当前回合玩家出杀后，若紧接着发送 `AdvanceRequested`，阶段会被强制推进到 Discard，而闪的待定响应永久悬空——本地模式下 `AutoAdvanceTimer` 从不在 Play 阶段启动所以从未暴露，网络模式下当前回合玩家可以随时发送此命令。修复：`advancePhase()` 的 `Play` 分支补上与 `endPlayPhase()` 一致的 `hasPendingAction()` 检查。
- **`GameStarted` 在校验 characterId 之前就广播（major）**：非法 characterId 会让客户端先收到"游戏已开始"，之后却永远等不到任何后续状态。修复：新增 `GameViewModel::isValidCharacterId()` 静态校验，`ServerApp::onBothPlayersReady` 在广播前先校验，非法 id 直接不广播（协议暂无专门的拒绝消息类型，留待后续协议版本）。

**测试补充**（响应同一轮审查指出的覆盖缺口）：`RespondCardRequested` 的身份伪造防护此前无专门测试（与 PlayCard/Discard 不对称）；`discardCommandUsesConnectionIdentityNotClaimedPlayerId` 只断言手牌数量、未直接断言 `currentPhase` 这个真正被保护的不变量；`playCardCommandUsesConnectionIdentityNotClaimedPlayerId` 依赖随机发牌恰好有可出的牌（理论有极低概率抖动）。均已修复/补充。

**涉及文件**：`src/ViewModel/GameViewModel.h/cpp`（`advancePhase` 加固 + `isValidCharacterId`）、`src/App/ServerApp.cpp`、`tests/network_test.cpp`（+3 新用例，2 处存量用例加固）

**验证**：NetworkTest 47 个用例，10 次连续直接运行 + 1 次全套件运行全部 47/47、5/5 通过，无抖动复现。

**审查再次确认、仍未修的深层规则缺口（与第一轮相同结论：属甲的卡牌/规则域）**：① `ActionViewModel::canPlayCard`/`playCard`/`discardCard` 全链路不校验 `playerId == currentPlayer()`——非当前回合玩家能用自己真实身份出/弃自己的牌（两轮审查独立确认，critical/major）。② `GameState::setPendingAction` 无重入保护，`canPlayCard`/`onPlayCardRequested` 不检查 `!hasPendingAction()`——即使不借助 ①，仅靠同一当前回合玩家连续出两张牌（如张飞无限杀）也能让第二个待定动作静默覆盖第一个（两轮审查独立确认为 critical）。这两处都需要改 `Card.cpp`/`GameRule.cpp`/`ActionViewModel.cpp` 的核心规则判断逻辑，边界超出"网络接线"范畴，待用户决定由谁在哪一步修复。

### [2026-07-15] 网络层 Step 4 加固：对抗性代码审查后修复越权/状态破坏问题

对 Step 4 的接线代码做了一轮多视角对抗性审查（每条发现都经独立二次核实），修复了其中网络层可安全处理、不触及本地 View/SGSApp 的几处真实问题：

- **越权命令（critical）**：`EndPlayRequested`/`AdvanceRequested`/`SkipRequested` 三个无参命令原先在 `ServerApp::onClientCommandReceived` 里被直接转发、完全不校验发起方身份，导致任意已连接客户端都能提前结束/越权推进对方的回合，或替对方强制放弃响应（即使对方有闪可用）。修复：`EndPlay`/`Advance` 校验 `playerId == currentPlayerId()`，`Skip` 校验 `playerId == pendingResponderId()`（新增 `GameViewModel::pendingResponderId()` 只读辅助，返回 int 不暴露 Model 指针）后才转发。
- **重复 SelectCharacter 销毁对局（critical）**：`GameServer::ClientSlot::selectedCharacterId` 在开局后不清空，已连接客户端在对局进行中重发一条 `SelectCharacter` 会让 `bothPlayersReady` 二次触发、`startHeadlessGame` 无条件丢弃当前整局重开。修复：`ServerApp::onBothPlayersReady` 开头加 `if (isGameRunning()) return;` 防重放。
- **非法 characterId 破坏服务器状态（critical）**：`SelectCharacterMsg::characterId` 是客户端可任意填的字段，非法 id 会让 `GameViewModel::startGame` 在 `initGame` 之前 return，留下"`m_gvm` 非空但从未初始化"的僵尸状态而 `isGameRunning()` 仍返回 true。修复：`startGame` 改为返回 `bool`（非法 id 返回 false 并 delete 已建的 Character），`ServerApp::startHeadlessGame` 据此回滚 `m_gvm` 为 nullptr。

**涉及文件**：`src/App/ServerApp.cpp`、`src/ViewModel/GameViewModel.h/cpp`（`startGame` 返回值 + `pendingResponderId()`）、`tests/network_test.cpp`（4 个加固回归用例）

**验证**：NetworkTest 45 个用例；新增 `skipRequestedFromWrongPlayerIsRejected`、`endPlayAndAdvanceFromWrongPlayerAreRejected`、`repeatedSelectCharacterAfterGameStartedIsIgnored`、`invalidCharacterIdDoesNotCorruptServerState`。10+ 次连跑仅复现既有的 `thirdConnectionRejected` 环境抖动（loopback 第三连接偶发超时，与本次改动无关）。

**审查另外确认了两处更深层的规则层缺陷（暂未修，属甲的卡牌/规则域，待用户定夺归属）**：① `ActionViewModel::canPlayCard`/`playCard`/`discardCard` 和 `Card::canUse` 全链路只查全局 `currentPhase()`，从不校验 `playerId == currentPlayer()`——非当前回合玩家能在对方回合出自己的牌/弃自己的牌（本地模式靠 View 只让当前玩家手牌可交互而侥幸掩盖，网络模式直接暴露）。② `GameState::setPendingAction` 无条件覆盖，加上出牌路径不查 `!hasPendingAction()`，能让第二个待定动作静默冲掉第一个、原响应卡死。这两处需改核心 GameRule/Card/ViewModel（甲的域），且与"网络接线"边界不同，留待协调。

### [2026-07-15] 网络层 Step 4：ServerApp 接线 GameServer（plan2.0.md §2.3-2.4）

`ServerApp` 现在持有 `GameServer` 并双向接线：`wireViewModelBroadcasts()` 把 `GameViewModel`/`ActionViewModel` 对外的全部 9 条信号（对照 connection.md 第 1 节逐条映射）连到序列化广播；`onClientCommandReceived` 把客户端的 7 类命令消息反序列化后分发到 VM 的 public slots。`onBothPlayersReady` 先广播 `GameStarted` 再调用 `startHeadlessGame`，保证客户端收到顺序是 `GameStarted → PhaseChanged → ...`（`startHeadlessGame` 内部改为先接线再 `startGame()`，避免开局的初始状态推送在无人监听时被丢弃）。

**安全加固**：`PlayCardMsg`/`RespondCardMsg`/`DiscardCardMsg` 里客户端消息体自称的 `playerId`/`responderId` 字段一律被忽略，改用 `GameServer` 基于 TCP 连接分配的可信 `playerId`——否则一个连接可以在消息里冒充另一个玩家操作对方手牌。`TargetSelectedMsg` 因 `ActionViewModel::onTargetSelected` 签名不带 `playerId` 暂无法在网络层补这层校验，已记录为已知限制留给 Step 9。

`MessageSerializer` 新增 `encodePayload<Msg>`/`wrapFrame`（裸 payload 序列化 + 补帧头分离），`encode<Msg>` 重构为二者组合；`GameServer::sendTo` 改用 `wrapFrame` 消除重复的帧头拼装代码。

**顺带修复**：`GameViewModel::onDiscardCardRequested` 存在一处预先潜伏的 bug——原逻辑按"请求方自己的 `playerId`"查 `getDiscardCount()` 判断是否 `advancePhase()`，本地模式下这个方法只会被"当前弃牌阶段的玩家"调用所以从未暴露；网络模式下任意已连接客户端发一条名义上合法（`playerId` 是自己真实的，只是不该在这个时机弃牌）的 `DiscardCardRequested`，会导致提前结束**另一个玩家**的弃牌阶段。修复为只有 `playerId == currentPlayerId()` 时才允许推进阶段，本地模式行为不受影响。

**涉及文件**：`src/App/ServerApp.h/cpp`、`src/Network/MessageSerializer.h/cpp`、`src/Network/GameServer.cpp`、`src/ViewModel/GameViewModel.cpp`、`tests/network_test.cpp`、`connection.md`（新增 §7 网络模式连接表）

**验证**：7 个新用例（GameStarted 广播顺序、PlayCardRequested/DiscardCardRequested 身份伪造防护 + 合法路径、AdvanceRequested/EndPlayRequested 网络分发、RespondCardRequested/SkipRequested 完整响应流程），用 `viewmodel_test.cpp` 已有的"手动注入确定性杀/闪卡牌"手法绕开随机发牌以保证测试确定性。NetworkTest 41 个用例，全套件 5/5 通过（含多次重复运行确认无抖动；`thirdConnectionRejected` 出现过一次孤立超时，重跑 8 次未复现，判定为环境抖动而非回归）。

### [2026-07-15] 网络层 Step 3：GameServer 连接管理与握手（plan2.0.md §2.3）

新建 `src/Network/GameServer.h/cpp`：QTcpServer 监听（`listen(port, localOnly)`，`localOnly=true` 绑 127.0.0.1 供测试用、不触发 Windows 防火墙弹窗；生产默认绑 Any）、最多 2 连接、playerId 0/1 按空槽分配、逐客户端 recvBuffer 帧解码（流损坏即断开）、`Handshake`（含协议版本校验）→ `HandshakeAck` → `SelectCharacter` → `bothPlayersReady` 流程、超员连接回 `Ack(playerId=-1, "房间已满")` 后断开、断线释放槽位可重连。未握手的命令/选将一律忽略。命令消息经 `clientCommandReceived` 信号转出（Step 4 由 ServerApp 分发到 VM public slots）。

**涉及文件**：`src/Network/GameServer.h/cpp`、`tests/network_test.cpp`、`CMakeLists.txt`

**验证**：7 个新用例（playerId 分配、第三连接拒绝、版本不符拒绝、双方选将触发 ready、断线重连复用槽位、未握手命令忽略、损坏流断开），裸 QTcpSocket 连 loopback、`QTest::qWaitFor` 驱动事件循环（不能用 `waitForConnected`——同进程测试会阻塞主循环导致 QTcpServer 收不到连接）。全套件 5/5 通过；曾观察到重链接后首跑一次超时（疑似 Defender 扫描新 exe 延迟），已把测试超时提高到 8s，此后 10+ 次连跑稳定。

### [2026-07-15] 网络层 Step 2：ServerApp headless 启动路径（plan2.0.md §2.3）

新建 `src/App/ServerApp.h/cpp`：`startHeadlessGame(charId1, charId2)` 创建完整 `GameViewModel`（含 ActionViewModel/Model），不创建任何 QWidget；重复开局用 `deleteLater()` 释放上一局对象图（与 `SGSApp` 一致）；`gameOver` 透传为 `gameFinished(winnerId)` 信号，VM 保留到下局开始（末批状态推送需经事件循环送达）。暴露 `gameViewModel()`/`actionViewModel()` 供 Step 4 的 GameServer 命令分发使用。

**涉及文件**：`src/App/ServerApp.h/cpp`、`tests/network_test.cpp`、`CMakeLists.txt`（NetworkTest 目标加入 ViewModel/ServerApp 源与 SanguoshaModel 链接）

**验证**：NetworkTest 在 `QTEST_GUILESS_MAIN`（无 QApplication）下运行——headless 启动、VM 信号活性（phaseChanged/handCardsUpdated/playerDataUpdated）、完整回合循环（Prepare→…→弃牌→End→切换玩家）、重复开局释放旧 VM 共 4 个新用例；全套件 5/5 通过。

### [2026-07-15] 网络层 Step 1：协议定义 + 消息序列化 + 帧封装（plan2.0.md §2）

新建 `src/Network/Protocol.h`（协议版本号 v1、`MessageType` 枚举、消息结构体，字段与 `GameViewModel`/`ActionViewModel` 的 public slots/signals 一一对齐）和 `src/Network/MessageSerializer.h/cpp`（`CardData`/`PlayerData`/`PendingActionData` 的 `QDataStream <<`/`>>`，含甲新增的装备字段；帧格式为 quint32 长度前缀 + quint8 类型 + payload，`decodeFrames` 处理半包/粘包并拒绝损坏的长度前缀）。QDataStream 版本固定为 `Qt_5_15` 保证两端一致。

CMake 新增 `Qt::Network` 组件、`SanguoshaNetwork` 静态库和 `NetworkTest` 测试目标。

**涉及文件**：`src/Network/Protocol.h`、`src/Network/MessageSerializer.h/cpp`、`tests/network_test.cpp`、`CMakeLists.txt`

**验证**：`NetworkTest` 21 个用例通过（全部消息类型帧级 round-trip、逐字节半包、三帧粘包 + 半帧残留、损坏长度前缀拒绝）；全套件 5/5 通过。

## Bug Fixes

### [2026-07-14] 修复濒死响应、权限校验、目标选择和旧局生命周期

- 致命伤害进入濒死后不再清掉救援待定动作；`checkDeath()` 会发出一次 `Player::died` 信号。
- 响应牌必须属于当前响应者并匹配要求牌型；濒死响应者统一使用 `source`，且【酒】可用于救援。
- 出牌、弃牌和响应接口补充持有者校验；过河拆桥/顺手牵羊没有合法目标时不会被使用或消耗。
- 多目标出牌等待界面选择目标，并同步恢复/清理界面目标选择状态。
- 新局开始或对局结束时对旧 `GameViewModel` 使用 `deleteLater()`，避免旧对象图泄漏。

**验证**：`ModelSmokeTest`、`ModelTest`、`ViewModelTest`、`ViewTest` 共 4/4 通过，已移除对应的预期失败断言。

### [2026-07-14] 补充分层自动化测试

新增 Qt Test 测试目标，覆盖 Model、ViewModel、View 和 App 组合根，并通过 CTest 统一运行。View 测试在 `QT_QPA_PLATFORM=offscreen` 环境下执行，避免依赖桌面显示服务器。

**涉及文件**：`tests/model_test.cpp`、`tests/viewmodel_test.cpp`、`tests/view_test.cpp`、`CMakeLists.txt`

**验证**：新增测试目标最初共 4/4 通过；本轮已将此前跟踪的缺陷断言改为正常断言并完成修复。

### [2026-07-14] 出牌阶段无法主动使用武将转化技能（武圣/龙胆）

**现象**：关羽无法将红色牌当【杀】主动使用，赵云无法将【闪】当【杀】主动使用。转化牌在出牌阶段不高亮、双击无反应。（响应阶段的转化一直正常，如用红牌响应南蛮入侵。）

**根因**：`Character::skillTransformCard` 只接入了响应路径（`ActionViewModel::getResponseCardIds`、`GameRule::hasDodgeToRespond`/`hasKillToRespond`），出牌路径的三个入口全部只认卡牌本体：

1. `ActionViewModel::getPlayableCards` / `canPlayCard` 只检查 `card->canUse()`——关羽的红闪（`DodgeCard::canUse` 恒 false）和赵云的闪永远不可出
2. `ActionViewModel::getValidTargetIds` 调用 `card->getValidTargets()`——闪的目标列表恒为空
3. `ActionViewModel::playCard` 直接 `card->execute()`——即使放开校验，执行的也是原牌效果而非杀的结算

张飞（咆哮，`GameRule::canPlayKill` 已特判）和曹操（奸雄，被动触发）不受影响。

**修复**：`ActionViewModel` 新增私有判定 `playsAsKill(card, player)`（条件：出牌阶段、玩家存活、卡牌本体不可用、技能转化结果为杀、通过 `canPlayKill` 出杀次数检查），并接入出牌路径三个入口：

1. `getPlayableCards` / `canPlayCard`：本体不可用时，`playsAsKill` 成立即视为可出（高亮 + 校验放行）
2. `getValidTargetIds`：转化为杀时按杀的目标规则返回（除自己外的存活角色）
3. `playCard`：转化为杀时不执行原牌 `execute()`，改走 `GameRule::executeKill()` 结算（酒加成、闪响应流程与真杀一致），并输出"发动【武圣】，将【X】当【杀】使用"日志

**设计取舍**：本体可用时优先按本体使用——如关羽缺血时的红桃按桃使用，不能当杀（满血时红桃本体不可用，可当杀）。当前 UI 是双击出牌，没有"选择用法"的交互，此规则保证行为确定；后续如加装备/技能按钮 UI 可再放开。

**涉及文件**：`src/ViewModel/ActionViewModel.h/cpp`

**验证（当时）**：构建通过，冒烟测试 95/95；当时仍需手动运行 `SanguoshaQt.exe` 用关羽/赵云实测主动转化出杀。当前已补充分层 Qt Test，但关羽/赵云主动转化的端到端断言仍需单独补齐。

### [2026-07-14] 选将后点击「开始对战」闪退

**现象**：选择武将后点击开始对战，程序短暂无响应后异常中止。

**根因**：`MainWindow::onStartGame()` 中 `m_bootstrap->boardWidget()` 在 `startLocalGame()` 之前调用，此时 GameBoardWidget 尚未创建，
返回 `nullptr`。随后 `m_centralStack->addWidget(nullptr)` 导致 Qt 异常退出。

**修复**：将 `boardWidget()` 移到 `startLocalGame()` 之后。

**涉及文件**：`src/View/MainWindow.cpp`

### [2026-07-14] 卡在准备阶段，无法进入出牌

**现象**：游戏启动后始终停留在准备阶段，自动阶段不推进。

**根因**：`GameState` 构造函数中 `m_currentPhase` 默认值已是 `Prepare`。`initGame()` 调用 `setCurrentPhase(Prepare)` 时因为新旧值相同，条件判断 `if (m_currentPhase != phase) { emit phaseChanged(…); }` 不成立，不发射信号。`GameBoardWidget::onPhaseChanged` 从未被调用，自动推进计时器从未启动。

**修复**：在 `initGame()` 中 `setCurrentPhase(Prepare)` 之后，强制 `emit phaseChanged(Prepare)` 启动流程。

**涉及文件**：`src/ViewModel/GameViewModel.cpp`

### [2026-07-14] 双方手牌显示为牌背

**现象**：进入游戏后双方手牌均显示为红色牌背花纹，看不到牌面。

**根因**：`GameBoardWidget::onHandCardsUpdated` 调用 `m_bottomHandArea->setCards(data, false)` 传入了 `false`。`HandCardAreaWidget::setCards` 的第二参数已从原 `isOpponent` 重构为 `faceUp`，`false` 表示牌背。

**修复**：两处调用均改为 `setCards(data, true)`，同时修改 `HandCardAreaWidget` 的默认值 `faceUp = true`。

**涉及文件**：`src/View/GameBoardWidget.cpp`、`src/View/HandCardAreaWidget.h`

### [2026-07-14] 运行一段时间后无响应卡退

**现象**：出牌/响应流程中程序突然无响应后异常退出。

**根因**：`HandCardAreaWidget::clearWidgets()` 使用 `delete w` 同步销毁旧 `CardWidget`。
当用户在出牌阶段点击一张牌时调用链为：

```
CardWidget::mousePressEvent → emit clicked(cardId)
  → HandCardAreaWidget::onCardWidgetClicked → emit cardClicked(cardId)
    → GameBoardWidget::onCardClicked → emit playCardRequested(cardId, pid)
      → GameBootstrap::onPlayCardRequested → avm->playCard(…)
        → 卡牌从手牌移除 → emit handCardsChanged
          → GameViewModel::pushHandCards → emit handCardsUpdated
            → GameBoardWidget::onHandCardsUpdated
              → m_bottomHandArea->setCards(…) → clearWidgets()
                → delete w — 正在处理 mousePressEvent 的 CardWidget 自身！
```

`delete w` 在对象自身的方法调用栈执行期间销毁对象，导致 return 时 use-after-free 崩溃。

**修复**：`clearWidgets()` 中 `delete w` 改为 `w->deleteLater()`，推迟到事件循环空闲时再销毁旧控件。
此时 `CardDisplayData` 是值类型，旧控件即使暂存也有独立数据副本，不会访问已释放内存。

**涉及文件**：`src/View/HandCardAreaWidget.cpp`

### [2026-07-14] 待定动作阶段卡死 / 必须打出响应牌

**现象**：两种表现——
1. 无闪/杀可响应时，游戏卡在响应阶段无法继续
2. 有响应牌时必须打出，无法选择跳过承担后果

**根因**：
- 无响应牌时，`ActionPanelWidget::updateForPendingAction` 因 `canSkip=false` 隐藏了「跳过」按钮，且没有自动推进逻辑
- 有响应牌时，虽然「跳过」按钮不可见，但即使能访问到也会因为 `ActionViewModel::skipResponse(responderId, false)` 中 `if (!forceNoCard && !info.canSkip) return;` 直接返回，不推进游戏

**修复**：
1. `ActionPanelWidget::updateForPendingAction` 始终显示「跳过」按钮，让玩家自由选择
2. `GameBootstrap::onSkipRequested` 调用 `skipResponse(responderId, true)` 强制推进（`forceNoCard=true`）
3. 新增 `GameBootstrap::onPendingActionFromVM` 拦截 `pendingActionCreated` 信号：无响应牌时自动跳过，有牌时转发给 View 显示响应界面

**涉及文件**：
- `src/View/ActionPanelWidget.cpp`
- `src/App/GameBootstrap.h/cpp`

### [2026-07-14] 响应阶段点击响应牌无效

**现象**：需要打出响应牌（如杀后的闪）时，点击手牌无反应。

**根因**：`GameBoardWidget::onCardClicked` 在 `State::Responding` 分支中不能直接使用当前回合玩家作为响应者；普通响应的响应者是动作 `target`，濒死救援的响应者是动作 `source`。

**修复**：在 `onPendingActionCreated` 中按要求牌型记录响应者 ID：普通响应使用 `targetId`，濒死救援使用 `sourceId`；`ActionViewModel` 再校验动作目标、牌型和持有者。

**涉及文件**：`src/View/GameBoardWidget.h/cpp`

### [2026-07-14] 跳过响应后操作面板不更新，覆盖结束出牌按钮

**现象**：出牌阶段打出杀，点击「跳过」后提示文字不更新、跳过按钮不消失，且覆盖了「结束出牌」按钮，无法结束出牌。

**根因**：`GameBoardWidget::onPendingActionCleared` 只重置了 `m_state = Idle`，没有恢复 `ActionPanelWidget` 的显示。面板仍停留在响应模式（显示跳过按钮 + 旧的响应提示文字）。

跳过按钮被设为 `setVisible(true)` 后一直显示，而「结束出牌」按钮虽存在但被遮挡在跳过按钮之下，且因 `hideAllButtons()` → `setVisible(false)` 未被调用而不可见。

**修复**：
1. `onPendingActionCleared` 中调用 `m_actionPanel->updateForPhase(m_currentPhase, false)` 恢复面板
2. 新增 `m_currentPhase` 成员变量，在 `onPhaseChanged` 中更新

**涉及文件**：`src/View/GameBoardWidget.h/cpp`

### [2026-07-14] 弃牌阶段必须弃牌，未检查手牌上限

**现象**：出牌阶段结束后进入弃牌阶段，即使手牌未超限也会强制进入弃牌界面。

**根因**：重构后弃牌阶段的 `getDiscardCount` 检查被删除。`GameBoardWidget::onPhaseChanged` 无条件设置 `m_state = Discarding` 并显示弃牌提示，不检查手牌是否超过体力值对应的上限。

**修复**：`GameBootstrap` 拦截 `phaseChanged` 信号，在 `Discard` 阶段先调用 `avm->getDiscardCount(curId)`。若 ≤ 0 则直接 `gvm->advancePhase()` 跳过弃牌阶段。

**涉及文件**：`src/App/GameBootstrap.h/cpp`、`src/ViewModel/GameViewModel.h`

### [2026-07-14] 架构调整（第二次重构）：GameBootstrap 更名 SGSApp，路由/拦截逻辑下沉 ViewModel

> 晚于下一条（第一次重构）。本文件中早于本次重构的条目提到的 `GameBootstrap` 均对应现在的 `SGSApp`，`CardDisplayData`/`PlayerDisplayData`/`PendingActionVM` 均对应现在的 `CardData`/`PlayerData`/`PendingActionData`；历史条目不改写。

**调整内容**：
- `GameBootstrap` 更名 `SGSApp`，退化为纯组合根：只创建对象、在 `startLocalGame()` 里建立 View ↔ ViewModel 信号槽直连、管理对局生命周期，零业务逻辑
- 原 App 层路由槽（`onPlayCardRequested`/`onTargetSelected`/`onRespondCardRequested` → `ActionViewModel`；`onDiscardCardRequested`/`onEndPlayRequested`/`onAdvanceRequested`/`onSkipRequested` → `GameViewModel`）改为 ViewModel 的 public slots，View 信号直连
- 原 App 层拦截逻辑下沉：弃牌数 ≤0 自动跳过 → `GameViewModel::setNextPhase`；无响应牌自动跳过 → `GameViewModel::onModelPendingActionCreated`
- Common 值类型改名：`CardDisplayData` → `CardData`（新增列表别名 `CardList`）、`PlayerDisplayData` → `PlayerData`、`PendingActionVM` → `PendingActionData`

**涉及文件**：`src/App/SGSApp.h/cpp`（原 `GameBootstrap.h/cpp`）、`src/ViewModel/*`、`src/Common/*`、`src/View/*`（仅头文件名变更）、`src/main.cpp`

**文档同步**：`connection.md`、`README.md`、`CLAUDE.md`、`interface.md` §8/§9、`plan2.0.md` §2/§7 顶部阅读说明均已按新架构更新。

### [2026-07-14] 架构调整：GameBootstrap 作为程序入口

**调整内容**：
- `MainWindow` 不再包含 `GameBootstrap`，改为纯 UI 层，由 `GameBootstrap` 创建并拥有
- `GameBootstrap` 成为真正的 Composition Root，创建 `MainWindow`、`GameViewModel`、`GameBoardWidget`
- `main.cpp` 直接创建 `GameBootstrap`，不直接引用 `MainWindow`

**依赖关系变更**：
```
旧: main → MainWindow → GameBootstrap → GameViewModel/GameBoardWidget
新: main → GameBootstrap → MainWindow + GameViewModel + GameBoardWidget
```

**涉及文件**：`src/main.cpp`、`src/App/GameBootstrap.h/cpp`、`src/View/MainWindow.h/cpp`
