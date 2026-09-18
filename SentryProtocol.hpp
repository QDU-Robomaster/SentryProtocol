#pragma once

// clang-format off
/* === MODULE MANIFEST V2 ===
module_description: 哨兵裁判系统决策发送模块
depends:
- id: QDU-Robomaster/Referee
  ref: same-or-dev
=== END MANIFEST === */
// clang-format on

#include <cstdint>

#include "Referee.hpp"
#include "libxr_def.hpp"
#include "message.hpp"

class SentryProtocol
{
 public:
  using State = Referee::State;

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
    RegisterTopic<&SentryProtocol::OnBuyBulletTopic>(buy_bullet_topic_);
    RegisterTopic<&SentryProtocol::OnRemoteBuyBulletTopic>(
        remote_buy_bullet_times_topic_);
    RegisterTopic<&SentryProtocol::OnRemoteBuyHpTopic>(remote_buy_hp_times_topic_);
    RegisterTopic<&SentryProtocol::OnBuyResurrectionTopic>(buy_resurrection_topic_);
    RegisterTopic<&SentryProtocol::OnStateTopic>(state_topic_);

    referee_suber_.StartWaiting();
  }

  void SetSwitchMode(State state)
  {
    if (referee_ == nullptr)
    {
      return;
    }

    referee_->SetSwitchMode(state);
    referee_->SendSentryPack();
  }

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
  template <void (SentryProtocol::*HANDLER)(LibXR::RawData&)>
  void RegisterTopic(LibXR::Topic& topic)
  {
    auto callback = LibXR::Topic::Callback::Create(
        [](bool in_isr, SentryProtocol* self, LibXR::RawData& raw_data)
        {
          UNUSED(in_isr);
          (self->*HANDLER)(raw_data);
        },
        this);
    topic.RegisterCallback(callback);
  }

  template <typename Data>
  static bool ReadTopicData(LibXR::RawData& raw_data, Data& data)
  {
    if (raw_data.size_ < sizeof(Data))
    {
      return false;
    }

    LibXR::Memory::FastCopy(&data, raw_data.addr_, sizeof(Data));
    return true;
  }

  void OnBuyBulletTopic(LibXR::RawData& raw_data)
  {
    uint16_t buy_bullet_num = 0;
    if (ReadTopicData(raw_data, buy_bullet_num) && referee_ != nullptr)
    {
      referee_->SetNeedBullet(static_cast<uint8_t>(buy_bullet_num));
      referee_->SendSentryPack();
    }
  }

  void OnRemoteBuyBulletTopic(LibXR::RawData& raw_data)
  {
    uint8_t bullet_number = 0;
    if (ReadTopicData(raw_data, bullet_number) && referee_ != nullptr)
    {
      referee_->SetBulletRemote(bullet_number);
      referee_->SendSentryPack();
    }
  }

  void OnRemoteBuyHpTopic(LibXR::RawData& raw_data)
  {
    uint8_t buy_hp = 0;
    if (ReadTopicData(raw_data, buy_hp) && buy_hp != 0 && referee_ != nullptr)
    {
      referee_->SetHPRemote();
      referee_->SendSentryPack();
    }
  }

  void OnBuyResurrectionTopic(LibXR::RawData& raw_data)
  {
    bool buy_resurrection = false;
    if (ReadTopicData(raw_data, buy_resurrection) && referee_ != nullptr)
    {
      referee_->SetRevivalRemote(buy_resurrection);
      referee_->SendSentryPack();
    }
  }

  void OnStateTopic(LibXR::RawData& raw_data)
  {
    uint8_t state = 0;
    if (ReadTopicData(raw_data, state) && referee_ != nullptr)
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
