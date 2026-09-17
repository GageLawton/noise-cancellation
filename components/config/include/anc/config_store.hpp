#pragma once

#include "anc/node_config.hpp"

namespace anc {

// Persists NodeConfig as a JSON file on a LittleFS partition.
//
// Device-only: depends on ESP-IDF VFS and cJSON. Host tests cover
// NodeConfig itself (defaults/validate), not this storage layer.
class ConfigStore {
public:
    // Mounts the LittleFS partition. Must succeed before load/save.
    // Formats the partition on first boot if it is blank.
    bool mount();

    // Reads config from flash into `out`.
    //
    // On a missing, unparseable, or invalid file, fills `out` with
    // NodeConfig::defaults(), writes those defaults back to flash, and
    // still returns true -- the device always boots to a working state.
    // Returns false only if flash itself is unusable.
    bool load(NodeConfig& out);

    // Serializes `cfg` to JSON and writes it to flash.
    // Rejects a config that fails validate().
    bool save(const NodeConfig& cfg);

private:
    bool mounted_ = false;
};

}  // namespace anc
