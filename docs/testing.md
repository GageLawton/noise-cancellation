# Host tests and fakes

How this project tests its code on a laptop, and how to add a test of your
own. For the commands that build and run the tests, see
[CONTRIBUTING.md](../CONTRIBUTING.md#running-the-host-tests).

## Why host tests

Most of the firmware (the DSP, calibration, alignment, config and transport
code) has no ESP-IDF dependency, so it builds on a laptop as well as on the
ESP32-S3. That means it can be tested there in a fraction of a second, with
no board attached, and the same tests run in CI on every pull request.
Host tests check the maths; the bench checks on real hardware check the
system ([design doc section 9.1](design.md#91-host-side-unit-testing-off-target)).

## Adding a test

Every `test/test_*.cpp` file becomes its own test program and its own
`ctest` entry automatically ([`test/CMakeLists.txt`](../test/CMakeLists.txt)
globs for them), so there is nothing to register. Tests use the
[Unity](https://github.com/ThrowTheSwitch/Unity) framework.

A minimal test file, trimmed from [`test_rms.cpp`](../test/test_rms.cpp):

```cpp
#include "anc/rms.hpp"
#include "unity.h"

void setUp() {}
void tearDown() {}

namespace {

void test_constant_half_amplitude() {
    const float samples[] = {0.5f, 0.5f, 0.5f, 0.5f};
    TEST_ASSERT_FLOAT_WITHIN(1e-6f, 0.5f, anc::rms(samples, 4));
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_constant_half_amplitude);
    return UNITY_END();
}
```

- `setUp` and `tearDown` run before and after every test. Unity needs them
  even when they are empty.
- Put the test functions in an unnamed `namespace` so two test files can use
  the same helper names.
- Each test needs a `RUN_TEST` line in `main`, or it never runs.
- The tests build with `-Wall -Wextra -Werror`, so a warning fails the build.
  Fix it rather than silencing it.

## Fakes

Code that needs hardware is written against a small interface, so a test
can swap in a pretend version that runs on the laptop. The fakes live in
[`test/fakes/`](../test/fakes/).

**[`fake_audio_io.hpp`](../test/fakes/fake_audio_io.hpp)** stands in for the
I2S microphone and amplifier (`anc::IAudioIo`). You give it the mic samples
it should "record", and it keeps every sample written to the "speaker" so
the test can check them with `written()`. See
[`test_fake_audio_io.cpp`](../test/test_fake_audio_io.cpp).

**[`fake_sync_tick.hpp`](../test/fakes/fake_sync_tick.hpp)** stands in for
the wire that will keep several boards in step (`anc::ISyncTick`). It counts
the ticks this node sends with `signals()`, and `deliverTick()` pretends a
tick arrived from another node. `wait()` never really waits, so tests stay
fast. See [`test_fake_sync_tick.cpp`](../test/test_fake_sync_tick.cpp).

**[`simulated_room.hpp`](../test/fakes/simulated_room.hpp)** stands in for
the room itself: up to four steady hums plus a fixed speaker-to-mic path
(an FIR filter). Each `step(speakerSample)` plays one sample and returns
what the mic hears, so the canceller and the calibration code can be
tested end to end. See `test_cancels_simulated_hum` in
[`test_fxlms_canceller.cpp`](../test/test_fxlms_canceller.cpp) and
[`test_calibration_pipeline.cpp`](../test/test_calibration_pipeline.cpp).

## Checks CI runs

Every push and pull request runs five jobs
([`.github/workflows/ci.yml`](../.github/workflows/ci.yml)):

- **Host tests** builds every `test_*.cpp` with `-Werror` and runs them with `ctest`.
- **Python tools** runs `ruff check tools` and `python -m pytest tools` on the scripts in `tools/`.
- **Coverage** builds the host tests with coverage turned on and fails if less than 85% of the lines in `components/` run during the tests, so new code needs tests.
- **Firmware build (ESP32-S3)** builds the real firmware with ESP-IDF v5.3. It is the only job that sees ESP-IDF, so it catches driver and component mistakes the host tests can't.
- **Format check** runs `clang-format --dry-run --Werror` over all C++ files. Run `./scripts/format.sh` before you push to fix formatting.
