#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: 哨兵自主决策发送模块：把 Topic 上的决策请求写入 Referee 的哨兵决策数据并发送给裁判系统服务器 / Sentry decision sender Module that writes decision requests from Topics into the Referee sentry decision data and sends it to the referee system server
depends:
- id: QDU-Robomaster/Referee
  ref: same-or-dev
=== END MANIFEST === */
// clang-format on

#include <cstdint>

#include "Referee.hpp"
#include "libxr_def.hpp"
#include "message.hpp"

/**
 * @brief 哨兵自主决策发送模块。
 *        Sentry decision sender Module.
 *
 * @details 订阅决策请求 Topic，写入 Referee 的哨兵决策数据（0x0120）
 *          并发送给裁判系统服务器。
 *          Subscribes to the decision request Topics, writes them into the sentry
 *          decision data (0x0120) of Referee and sends it to the referee system server.
 */
class SentryProtocol
{
 public:
  /**
   * @brief 哨兵姿态。
   *        Sentry posture.
   */
  using State = Referee::State;

  /**
   * @brief 构造 SentryProtocol，订阅裁判摘要 Topic 并创建、注册 5 个决策请求 Topic。
   *        Construct SentryProtocol, subscribe to the referee summary Topic and create
   *        and register the 5 decision request Topics.
   *
   * @param referee Referee 实例；为 `nullptr` 时跳过所有发送。
   *                Referee instance; all sending is skipped when `nullptr`.
   * @param referee_sentry_tp_name 订阅的裁判摘要 Topic 名称。
   *                               Name of the subscribed referee summary Topic.
   * @param buy_bullet_topic_name 自主兑换发弹量 Topic 名称。
   *                              Name of the autonomous projectile exchange Topic.
   * @param remote_buy_bullet_times_topic_name 远程兑换发弹量 Topic 名称。
   *                                           Name of the remote projectile exchange
   *                                           Topic.
   * @param remote_buy_hp_times_topic_name 远程兑换血量 Topic 名称。
   *                                       Name of the remote HP exchange Topic.
   * @param buy_resurrection_topic_name 兑换立即复活 Topic 名称。
   *                                    Name of the immediate revival exchange Topic.
   * @param state_topic_name 切换姿态 Topic 名称。
   *                         Name of the posture switch Topic.
   */
  SentryProtocol(
      Referee* referee,
      const char* referee_sentry_tp_name = "robot_game_ref",
      const char* buy_bullet_topic_name = "sentry_buy_bullet_num",
      const char* remote_buy_bullet_times_topic_name = "sentry_remote_buy_bullet_times",
      const char* remote_buy_hp_times_topic_name = "sentry_remote_buy_hp_times",
      const char* buy_resurrection_topic_name = "sentry_buy_resurrection",
      const char* state_topic_name = "sentry_state")
      : referee_(referee),
        referee_suber_(referee_sentry_tp_name),
        buy_bullet_topic_(LibXR::Topic::CreateTopic<uint16_t>(buy_bullet_topic_name)),
        remote_buy_bullet_times_topic_(
            LibXR::Topic::CreateTopic<uint8_t>(remote_buy_bullet_times_topic_name)),
        remote_buy_hp_times_topic_(
            LibXR::Topic::CreateTopic<uint8_t>(remote_buy_hp_times_topic_name)),
        buy_resurrection_topic_(
            LibXR::Topic::CreateTopic<bool>(buy_resurrection_topic_name)),
        state_topic_(LibXR::Topic::CreateTopic<uint8_t>(state_topic_name))
  {
    RegisterTopic<uint16_t, &SentryProtocol::OnBuyBulletTopic>(buy_bullet_topic_);
    RegisterTopic<uint8_t, &SentryProtocol::OnRemoteBuyBulletTopic>(
        remote_buy_bullet_times_topic_);
    RegisterTopic<uint8_t, &SentryProtocol::OnRemoteBuyHpTopic>(
        remote_buy_hp_times_topic_);
    RegisterTopic<bool, &SentryProtocol::OnBuyResurrectionTopic>(buy_resurrection_topic_);
    RegisterTopic<uint8_t, &SentryProtocol::OnStateTopic>(state_topic_);

    referee_suber_.StartWaiting();
  }

  /**
   * @brief 设置哨兵姿态并发送哨兵包。
   *        Set the sentry posture and send the sentry packet.
   *
   * @param state 目标姿态。
   *              Target posture.
   */
  void SetSwitchMode(State state)
  {
    if (referee_ == nullptr)
    {
      return;
    }

    referee_->SetSwitchMode(state);
    referee_->SendSentryPack();
  }

  /**
   * @brief 监视回调：更新裁判摘要，阵亡（`max_hp != 0` 且 `remain_hp == 0`）
   *        时设置确认复活并发送哨兵包。
   *        Monitor callback: update the referee summary and, when the robot is dead
   *        (`max_hp != 0` and `remain_hp == 0`), set the revival confirmation and send
   *        the sentry packet.
   */
  void OnMonitor()
  {
    if (referee_suber_.Available())
    {
      referee_pack_ = referee_suber_.GetData();
      referee_suber_.StartWaiting();
    }

    if (referee_ == nullptr)
    {
      return;
    }

    const bool IS_DEAD = referee_pack_.robot_status.max_hp != 0 &&
                         referee_pack_.robot_status.remain_hp == 0;
    if (IS_DEAD)
    {
      referee_->SetConfirmRevival(true);
      referee_->SendSentryPack();
    }
  }

 private:
  template <typename Data, void (SentryProtocol::*HANDLER)(Data)>
  void RegisterTopic(LibXR::Topic& topic)
  {
    auto callback = LibXR::Topic::Callback::Create(
        [](bool in_isr, SentryProtocol* self, const Data& data)
        {
          UNUSED(in_isr);
          (self->*HANDLER)(data);
        },
        this);
    topic.RegisterCallback(callback);
  }

  void OnBuyBulletTopic(uint16_t buy_bullet_num)
  {
    if (referee_ != nullptr)
    {
      referee_->SetNeedBullet(static_cast<uint8_t>(buy_bullet_num));
      referee_->SendSentryPack();
    }
  }

  void OnRemoteBuyBulletTopic(uint8_t bullet_number)
  {
    if (referee_ != nullptr)
    {
      referee_->SetBulletRemote(bullet_number);
      referee_->SendSentryPack();
    }
  }

  void OnRemoteBuyHpTopic(uint8_t buy_hp)
  {
    if (buy_hp != 0 && referee_ != nullptr)
    {
      referee_->SetHPRemote();
      referee_->SendSentryPack();
    }
  }

  void OnBuyResurrectionTopic(bool buy_resurrection)
  {
    if (referee_ != nullptr)
    {
      referee_->SetRevivalRemote(buy_resurrection);
      referee_->SendSentryPack();
    }
  }

  void OnStateTopic(uint8_t state)
  {
    if (referee_ != nullptr)
    {
      referee_->SetSwitchMode(static_cast<State>(state));
      referee_->SendSentryPack();
    }
  }

  Referee* referee_;
  LibXR::Topic::ASyncSubscriber<Referee::RobotGameRefereePack> referee_suber_;
  LibXR::Topic buy_bullet_topic_;
  LibXR::Topic remote_buy_bullet_times_topic_;
  LibXR::Topic remote_buy_hp_times_topic_;
  LibXR::Topic buy_resurrection_topic_;
  LibXR::Topic state_topic_;

  Referee::RobotGameRefereePack referee_pack_{};
};
