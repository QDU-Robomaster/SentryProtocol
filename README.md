# SentryProtocol

哨兵自主决策发送模块：把 Topic 上的决策请求写入 Referee 的哨兵决策数据并发送给裁判系统服务器 / Sentry decision sender Module that writes decision requests from Topics into the Referee sentry decision data and sends it to the referee system server

## 1. 模块作用 / Purpose

SentryProtocol 把上层经 Topic 发来的哨兵自主决策请求写入 `Referee` 的哨兵决策数据（0x0120），并立即调用 `Referee::SendSentryPack()` 发送给裁判系统服务器。

构造时，SentryProtocol 订阅 `referee_sentry_tp_name`（`Referee::RobotGameRefereePack`）。订阅按名称等待 Topic 出现，提供该 Topic 的 `Referee` 实例先于本实例构造。同时创建 5 个输入 Topic 并注册同步回调，每收到一条消息就更新决策变量并发送一次哨兵包：

| 默认 Topic | 类型 | 处理 |
| --- | --- | --- |
| `sentry_buy_bullet_num` | `uint16_t` | `SetNeedBullet()`：把数值（转换为 `uint8_t`）累加到兑换发弹量 |
| `sentry_remote_buy_bullet_times` | `uint8_t` | `SetBulletRemote()`：远程兑换发弹量请求次数加 1，并累加兑换发弹量 |
| `sentry_remote_buy_hp_times` | `uint8_t` | 非 0 时调用 `SetHPRemote()`：远程兑换血量请求次数加 1 |
| `sentry_buy_resurrection` | `bool` | `SetRevivalRemote()`：设置是否兑换立即复活 |
| `sentry_state` | `uint8_t` | `SetSwitchMode()`：切换姿态（1 进攻、2 防御、3 移动） |

`OnMonitor()` 由生成的主函数在监视循环中调用：取最新的裁判摘要，当 `max_hp != 0` 且 `remain_hp == 0`（已阵亡）时调用 `SetConfirmRevival(true)` 并发送哨兵包，因此阵亡期间每个监视周期发送一次确认复活。`SetSwitchMode(State)` 供其他代码直接切换姿态并发送哨兵包。`referee` 为 `nullptr` 时发送操作被跳过。

SentryProtocol writes the sentry autonomous decision requests that arrive over Topics into the sentry decision data (0x0120) of `Referee` and immediately calls `Referee::SendSentryPack()` to send it to the referee system server.

At construction, SentryProtocol subscribes to `referee_sentry_tp_name` (`Referee::RobotGameRefereePack`). The subscription waits for the Topic by name, so the `Referee` instance that provides the Topic is constructed before this instance. It also creates 5 input Topics and registers synchronous callbacks; each received message updates the decision variables and sends the sentry packet once:

| Default Topic | Type | Handling |
| --- | --- | --- |
| `sentry_buy_bullet_num` | `uint16_t` | `SetNeedBullet()`: adds the value (converted to `uint8_t`) to the projectile amount to exchange |
| `sentry_remote_buy_bullet_times` | `uint8_t` | `SetBulletRemote()`: increments the remote projectile exchange request count by 1 and adds to the amount to exchange |
| `sentry_remote_buy_hp_times` | `uint8_t` | When non-zero calls `SetHPRemote()`: increments the remote HP exchange request count by 1 |
| `sentry_buy_resurrection` | `bool` | `SetRevivalRemote()`: sets whether to exchange for an immediate revival |
| `sentry_state` | `uint8_t` | `SetSwitchMode()`: switches the posture (1 attack, 2 defend, 3 move) |

`OnMonitor()` is called by the generated main function in its monitor loop: it takes the latest referee summary and, when `max_hp != 0` and `remain_hp == 0` (the robot is dead), calls `SetConfirmRevival(true)` and sends the sentry packet, so a revival confirmation is sent once per monitor cycle while the robot is dead. `SetSwitchMode(State)` lets other code switch the posture and send the sentry packet directly. With `referee` set to `nullptr` the send operations are skipped.

## 2. 构造接口 / Constructor

```cpp
SentryProtocol(Referee* referee,
               const char* referee_sentry_tp_name = "robot_game_ref",
               const char* buy_bullet_topic_name = "sentry_buy_bullet_num",
               const char* remote_buy_bullet_times_topic_name = "sentry_remote_buy_bullet_times",
               const char* remote_buy_hp_times_topic_name = "sentry_remote_buy_hp_times",
               const char* buy_resurrection_topic_name = "sentry_buy_resurrection",
               const char* state_topic_name = "sentry_state");
```

依赖：

- `referee`：`Referee*`，Referee 实例的指针。

配置参数：

- `referee_sentry_tp_name`：订阅的裁判摘要 Topic，默认 `"robot_game_ref"`（`Referee` 的默认 `referee_robot_game_tp_name`）。
- `buy_bullet_topic_name`：自主兑换发弹量 Topic，默认 `"sentry_buy_bullet_num"`。
- `remote_buy_bullet_times_topic_name`：远程兑换发弹量 Topic，默认 `"sentry_remote_buy_bullet_times"`。
- `remote_buy_hp_times_topic_name`：远程兑换血量 Topic，默认 `"sentry_remote_buy_hp_times"`。
- `buy_resurrection_topic_name`：兑换立即复活 Topic，默认 `"sentry_buy_resurrection"`。
- `state_topic_name`：切换姿态 Topic，默认 `"sentry_state"`。

Dependencies:

- `referee`: `Referee*`, pointer to the Referee instance.

Configuration parameters:

- `referee_sentry_tp_name`: subscribed referee summary Topic, default `"robot_game_ref"` (the default `referee_robot_game_tp_name` of `Referee`).
- `buy_bullet_topic_name`: Topic for the autonomous projectile exchange, default `"sentry_buy_bullet_num"`.
- `remote_buy_bullet_times_topic_name`: Topic for the remote projectile exchange, default `"sentry_remote_buy_bullet_times"`.
- `remote_buy_hp_times_topic_name`: Topic for the remote HP exchange, default `"sentry_remote_buy_hp_times"`.
- `buy_resurrection_topic_name`: Topic for the immediate revival exchange, default `"sentry_buy_resurrection"`.
- `state_topic_name`: Topic for the posture switch, default `"sentry_state"`.

## 3. Topic

| Topic（默认名称） | 方向 | 类型 | 说明 |
| --- | --- | --- | --- |
| `referee_sentry_tp_name`（`robot_game_ref`） | 订阅 | `Referee::RobotGameRefereePack` | 裁判摘要，`OnMonitor()` 用于判断是否阵亡 |
| `buy_bullet_topic_name`（`sentry_buy_bullet_num`） | 创建并订阅 | `uint16_t` | 自主兑换发弹量 |
| `remote_buy_bullet_times_topic_name`（`sentry_remote_buy_bullet_times`） | 创建并订阅 | `uint8_t` | 远程兑换发弹量 |
| `remote_buy_hp_times_topic_name`（`sentry_remote_buy_hp_times`） | 创建并订阅 | `uint8_t` | 远程兑换血量 |
| `buy_resurrection_topic_name`（`sentry_buy_resurrection`） | 创建并订阅 | `bool` | 兑换立即复活 |
| `state_topic_name`（`sentry_state`） | 创建并订阅 | `uint8_t` | 切换姿态 |

| Topic (default name) | Direction | Type | Meaning |
| --- | --- | --- | --- |
| `referee_sentry_tp_name` (`robot_game_ref`) | Subscribe | `Referee::RobotGameRefereePack` | Referee summary, used by `OnMonitor()` to detect death |
| `buy_bullet_topic_name` (`sentry_buy_bullet_num`) | Create and subscribe | `uint16_t` | Autonomous projectile exchange |
| `remote_buy_bullet_times_topic_name` (`sentry_remote_buy_bullet_times`) | Create and subscribe | `uint8_t` | Remote projectile exchange |
| `remote_buy_hp_times_topic_name` (`sentry_remote_buy_hp_times`) | Create and subscribe | `uint8_t` | Remote HP exchange |
| `buy_resurrection_topic_name` (`sentry_buy_resurrection`) | Create and subscribe | `bool` | Immediate revival exchange |
| `state_topic_name` (`sentry_state`) | Create and subscribe | `uint8_t` | Posture switch |

## 4. 配置示例 / Configuration Example

`xrobot instance add QDU-Robomaster/SentryProtocol` 写入的实例，`referee` 填写为 `QDU-Robomaster/Referee` 实例，指针依赖写成 `'&id'`：

An instance written by `xrobot instance add QDU-Robomaster/SentryProtocol`, with `referee` set to a `QDU-Robomaster/Referee` instance, pointer dependencies written as `'&id'`:

```yaml
modules:
  - module: QDU-Robomaster/SentryProtocol
    id: SentryProtocol_0
    args:
      - referee: '&ref'
      - referee_sentry_tp_name: "robot_game_ref"
      - buy_bullet_topic_name: "sentry_buy_bullet_num"
      - remote_buy_bullet_times_topic_name: "sentry_remote_buy_bullet_times"
      - remote_buy_hp_times_topic_name: "sentry_remote_buy_hp_times"
      - buy_resurrection_topic_name: "sentry_buy_resurrection"
      - state_topic_name: "sentry_state"
```

被引用的 `Referee` 实例列在本实例之前。

The referenced `Referee` instance is listed before this instance.

## 5. 依赖与硬件 / Dependencies and Hardware

依赖：

- `QDU-Robomaster/Referee`：提供哨兵决策数据、`SendSentryPack()` 与 `robot_game_ref` 摘要 Topic。
- LibXR。

硬件：经 `Referee` 连接裁判系统的串口。

Dependencies:

- `QDU-Robomaster/Referee`: provides the sentry decision data, `SendSentryPack()` and the `robot_game_ref` summary Topic.
- LibXR.

Hardware: the UART to the referee system, provided through `Referee`.
