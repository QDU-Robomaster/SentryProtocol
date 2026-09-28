# SentryProtocol

哨兵裁判系统决策发送模块。把上层通过 topic 发来的哨兵自主决策请求写入 `Referee` 的哨兵决策
数据（0x0120），并立即调用 `Referee::SendSentryPack()` 发送给裁判系统服务器。

- 构造时订阅 `referee_sentry_tp_name`（`Referee::RobotGameRefereePack`）。订阅按名称等待 topic
  出现，因此提供该 topic 的 `Referee` 实例必须先构造。
- 创建 5 个输入 topic 并注册同步回调，每收到一条消息就更新决策变量并发送一次哨兵包：

  | 默认 topic | 类型 | 处理 |
  |---|---|---|
  | `sentry_buy_bullet_num` | `uint16_t` | `SetNeedBullet()`：把数值（截断为 `uint8_t`）累加到兑换发弹量 |
  | `sentry_remote_buy_bullet_times` | `uint8_t` | `SetBulletRemote()`：远程兑换发弹量请求次数加 1，并累加兑换发弹量 |
  | `sentry_remote_buy_hp_times` | `uint8_t` | 非 0 时 `SetHPRemote()`：远程兑换血量请求次数加 1 |
  | `sentry_buy_resurrection` | `bool` | `SetRevivalRemote()`：设置是否兑换立即复活 |
  | `sentry_state` | `uint8_t` | `SetSwitchMode()`：切换姿态（1 进攻、2 防御、3 移动） |

- `OnMonitor()` 由生成的入口在监视循环中调用：取最新的裁判摘要，若 `max_hp != 0` 且
  `remain_hp == 0`（已阵亡），调用 `SetConfirmRevival(true)` 并发送哨兵包，因此阵亡期间每个
  监视周期都会发送一次确认复活。
- `SetSwitchMode(State)`：供其他代码直接切换姿态并发送哨兵包。

`referee` 为 `nullptr` 时所有发送操作都被跳过。

## 依赖

- `QDU-Robomaster/Referee`：提供哨兵决策数据、`SendSentryPack()` 和 `robot_game_ref` 摘要 topic。

无外部软件包依赖。

## 构造接口

```cpp
SentryProtocol(Referee* referee,
               const char* referee_sentry_tp_name = "robot_game_ref",
               const char* buy_bullet_topic_name = "sentry_buy_bullet_num",
               const char* remote_buy_bullet_times_topic_name = "sentry_remote_buy_bullet_times",
               const char* remote_buy_hp_times_topic_name = "sentry_remote_buy_hp_times",
               const char* buy_resurrection_topic_name = "sentry_buy_resurrection",
               const char* state_topic_name = "sentry_state");
```

依赖项：

- `referee`：指向 `Referee` 实例的指针。

配置：

- `referee_sentry_tp_name`：订阅的裁判摘要 topic，默认 `"robot_game_ref"`（`Referee` 的默认
  `referee_robot_game_tp_name`）。
- `buy_bullet_topic_name`：自主兑换发弹量 topic，默认 `"sentry_buy_bullet_num"`。
- `remote_buy_bullet_times_topic_name`：远程兑换发弹量 topic，默认 `"sentry_remote_buy_bullet_times"`。
- `remote_buy_hp_times_topic_name`：远程兑换血量 topic，默认 `"sentry_remote_buy_hp_times"`。
- `buy_resurrection_topic_name`：兑换立即复活 topic，默认 `"sentry_buy_resurrection"`。
- `state_topic_name`：切换姿态 topic，默认 `"sentry_state"`。

## 使用

```sh
xrobot module add QDU-Robomaster/SentryProtocol
xrobot setup
xrobot instance add QDU-Robomaster/SentryProtocol
```

`xrobot instance add` 在 `User/xrobot.yaml` 中写入一个实例，依赖项留空，默认值按源码写出；
把 `referee` 填为 Referee 实例的 `id`：

```yaml
modules:
  - module: QDU-Robomaster/SentryProtocol
    id: sentryprotocol_0
    args:
      - referee: referee
      - referee_sentry_tp_name: '"robot_game_ref"'
      - buy_bullet_topic_name: '"sentry_buy_bullet_num"'
      - remote_buy_bullet_times_topic_name: '"sentry_remote_buy_bullet_times"'
      - remote_buy_hp_times_topic_name: '"sentry_remote_buy_hp_times"'
      - buy_resurrection_topic_name: '"sentry_buy_resurrection"'
      - state_topic_name: '"sentry_state"'
```

`referee` 是 `QDU-Robomaster/Referee` 实例的 `id`，必须在 `modules:` 中排在本实例之前。本模块
不直接使用 BSP 对象，不需要额外的 `XR_REGISTER`。

填好后再次运行 `xrobot setup`，生成 `User/xrobot_main.hpp`。

`xrobot module show .`（在本仓库中）或 `xrobot module show Modules/QDU-Robomaster/SentryProtocol`
（在 BSP 中）打印当前的构造函数。
