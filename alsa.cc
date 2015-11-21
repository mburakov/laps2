#include "resources.h"
#include "widget.h"
#include <alsa/asoundlib.h>

namespace {

const uint8_t* kVolumeLevel[] = {
  volume_00, volume_01, volume_02, volume_03, volume_04, volume_05
};

void AlsaCheck(int result, const std::string& message) {
  try {
    if (result < 0)
      throw std::runtime_error(snd_strerror(result));
  } catch (const std::exception&) {
    std::throw_with_nested(std::runtime_error(message));
  }
}

struct : public Widget {
  // TODO(Micha): Replace with RaiiFd
  std::vector<pollfd> pollfds_;
  snd_mixer_t* mixer_;
  snd_mixer_elem_t* master_;

  void Init(int argc, char** argv) override {
    // TODO(Micha): Handle arguments
    static const char kCard[] = "default";
    static const char kChan[] = "Master";

    snd_mixer_selem_id_t* sid;
    AlsaCheck(snd_mixer_open(&mixer_, 0), "Cannot open mixer");
    AlsaCheck(snd_mixer_attach(mixer_, kCard), "Cannot attach mixer");
    AlsaCheck(snd_mixer_selem_register(mixer_, nullptr, nullptr), "Cannot register selem");
    AlsaCheck(snd_mixer_load(mixer_), "Cannot load mixer");
    snd_mixer_selem_id_alloca(&sid);
    if (!sid) throw std::runtime_error("Cannot allocate selem id");
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, kChan);
    master_ = snd_mixer_find_selem(mixer_, sid);
    if (!master_) throw std::runtime_error("Cannot find master volume control");
    pollfds_.resize(snd_mixer_poll_descriptors_count(mixer_));
    AlsaCheck(snd_mixer_poll_descriptors(mixer_, pollfds_.data(), pollfds_.size()), "Cannot get poll descriptors");
  }

  const uint8_t* GetState() override {
    long min, max, cur;
    snd_mixer_selem_get_playback_volume_range(master_, &min, &max);
    snd_mixer_selem_get_playback_volume(master_, static_cast<snd_mixer_selem_channel_id_t>(0), &cur);
    auto vol_images = util::Length(kVolumeLevel);
    return kVolumeLevel[vol_images * cur / (max - min + 1)];
  }

  std::vector<pollfd> GetPollFds() override {
    return pollfds_;
  }

  void Activate() override {
  }

  void Handle(const pollfd&) override {
    snd_mixer_handle_events(mixer_);
  }
} __impl__;

}  // namespace
