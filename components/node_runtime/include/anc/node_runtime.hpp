#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "anc/block_processor.hpp"
#include "anc/config_store.hpp"
#include "anc/fxlms_canceller.hpp"
#include "anc/fxlms_processor.hpp"
#include "anc/gain_ramp.hpp"
#include "anc/i2s_audio_io.hpp"
#include "anc/node_config.hpp"
#include "anc/node_state.hpp"
#include "anc/node_status.hpp"
#include "anc/probe_processor.hpp"
#include "anc/silence_processor.hpp"
#include "anc/spsc_ring_buffer.hpp"
#include "anc/tone_processor.hpp"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace anc {

// One mic sample and the speaker sample FilterTask produced alongside it,
// for AlignmentTask.
struct SamplePair {
    float mic;
    float synth;
};

// Owns the node's lifecycle: config, state machine, and task setup.
//
// Audio runs in its own task (see audioIoLoop). The DSP stages are filled
// in milestone by milestone -- see design doc section 10.4.
class NodeRuntime {
public:
    // Runs Boot -> InitHal -> ... and spawns tasks. Does not return
    // under normal operation.
    void start();

    const NodeConfig& config() const { return config_; }
    NodeStatusStore& status() { return status_; }

private:
    void transitionTo(NodeState next);

    // State handlers. Each returns the next state to enter.
    NodeState doBoot();
    NodeState doInitHal();
    NodeState doCalibrate();
    NodeState doRun();

    void spawnTasks();

    // Main-task helpers for driving FilterTask. Each returns false if
    // FilterTask didn't respond in time (it should within a few blocks).
    //
    // Makes p the active processor and waits until FilterTask has run a
    // whole block with it, so the old one is no longer in use.
    bool useProcessor(IBlockProcessor* p);
    // Sets the fade target and waits until the output gain gets there.
    bool fadeTo(float gain);

    // AudioIOTask body: read a mic block, hand it to FilterTask, play the
    // block FilterTask made earlier, repeat. The blocking I2S read paces
    // it at the sample rate.
    void audioIoLoop();

    // FilterTask body: wait for a full mic block, run the active
    // IBlockProcessor on it, queue the result for the speaker, repeat.
    void filterLoop();

    // TelemetryTask body: one status line a second.
    void telemetryLoop();

    // AlignmentTask body: drain {mic, synth} pairs from FilterTask. Core 1,
    // so nothing here can delay the audio path.
    void alignmentLoop();

    // Samples per audio block: 4 ms at 8 kHz, the same as the DMA block.
    static constexpr std::size_t kBlockSamples = 32;

    // How far FilterTask's output runs ahead of the speaker. Two blocks
    // means FilterTask can finish a block up to 4 ms late without a gap.
    // It is also part of the speaker-to-mic delay calibration measures,
    // so AudioIOTask holds it fixed (see audioIoLoop).
    static constexpr std::size_t kOutputLeadSamples = 2 * kBlockSamples;

    // Reads the factory-burned MAC and folds it into a 64-bit id.
    static uint64_t deriveNodeId();

    ConfigStore store_;
    NodeConfig config_{};
    NodeState state_ = NodeState::Boot;
    NodeStatusStore status_;
    uint64_t nodeId_ = 0;

    I2sAudioIo audio_;
    bool halReady_ = false;  // set once I2S is up and the tasks are running
    std::atomic<uint32_t> blocksProcessed_{0};  // audio blocks read and written

    // Mic samples, AudioIOTask -> FilterTask. 1023 usable slots, about
    // 128 ms of audio, so FilterTask can fall well behind before any drop.
    SpscRingBuffer<float, 1024> micRing_;
    SilenceProcessor silence_;
    // M3 test tone. Built in doInitHal() once the config is loaded, since
    // ToneProcessor takes its frequency and level in the constructor. RUN
    // plays the canceller now; for a bench test, swap &fxlms_ for &*tone_
    // in doRun().
    std::optional<ToneProcessor> tone_;

    // Output safety stage, after whatever processor runs: a fade, so
    // switching never clicks, then a hard limiter at output.ceiling.
    // ramp_ belongs to FilterTask alone. Other tasks ask for a new level
    // through gainTarget_, which FilterTask applies at the next block, so
    // GainRamp never sees two threads at once.
    GainRamp ramp_{400};  // 50 ms at 8 kHz
    std::atomic<float> gainTarget_{0.0f};
    std::atomic<float> outputGain_{0.0f};  // ramp_'s gain after the last block
    std::atomic<uint32_t> clipEvents_{0};  // samples the limiter clipped

    // M4 calibration: plays white noise and records what the mic hears.
    // 0.1 is quiet enough to be comfortable, and loud enough that 300 ms
    // of probe gives a clean estimate.
    static constexpr float kProbeAmplitude = 0.1f;
    ProbeProcessor probe_{kProbeAmplitude};
    // Set by FilterTask once the probe recording is full. The release /
    // acquire pair makes the finished recording visible to the main task.
    std::atomic<bool> probeDone_{false};

    // The measured speaker-to-mic impulse response, for the canceller.
    // Sized for the largest secondary_path_taps that validate() allows.
    static constexpr std::size_t kMaxSecondaryPathTaps = 1024;
    float secondaryPath_[kMaxSecondaryPathTaps]{};
    std::size_t secondaryPathTaps_ = 0;

    // M5: the hum canceller. The main task configures it only while it is
    // not the active processor; FilterTask runs it and reports on it.
    FxlmsCanceller canceller_{kSampleRateHz};
    FxlmsProcessor fxlms_{canceller_};
    std::atomic<float> residualDbfs_{-120.0f};  // what's left of the hum
    std::atomic<bool> converged_{false};
    // What FilterTask runs. Later milestones swap in the tone, the probe
    // and the canceller with a single store().
    std::atomic<IBlockProcessor*> processor_{&silence_};
    std::atomic<uint32_t> filterBlocks_{0};  // blocks FilterTask processed
    std::atomic<float> micDbfs_{-120.0f};    // level of the latest mic block
    // Speaker samples, FilterTask -> AudioIOTask.
    SpscRingBuffer<float, 1024> outRing_;
    // Speaker samples AudioIOTask had to fill with 0 because FilterTask's
    // output wasn't ready.
    std::atomic<uint32_t> underruns_{0};

    // FilterTask -> AlignmentTask (core 0 -> core 1). FilterTask never
    // waits on core 1: when the ring is full, the pair is dropped and
    // counted. 2047 usable slots, about 256 ms of audio.
    SpscRingBuffer<SamplePair, 2048> alignRing_;
    std::atomic<uint32_t> alignDrops_{0};
    std::atomic<uint32_t> alignSamples_{0};  // pairs AlignmentTask received
    // AudioIOTask wakes FilterTask with a task notification after each mic
    // block, so FilterTask starts at once instead of polling.
    TaskHandle_t filterTask_ = nullptr;
};

}  // namespace anc
