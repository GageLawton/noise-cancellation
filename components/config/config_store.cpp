#include "anc/config_store.hpp"

#include <cstdio>
#include <cstring>
#include <memory>

#include "cJSON.h"
#include "esp_littlefs.h"
#include "esp_log.h"

namespace anc {
namespace {

constexpr const char* kTag            = "config";
constexpr const char* kMountPoint     = "/cfg";
constexpr const char* kPartitionLabel = "config";
constexpr const char* kConfigPath     = "/cfg/node.json";

// cJSON hands back raw owning pointers; these make the ownership
// automatic so no path through this file can leak on an early return.
struct CjsonDeleter {
    void operator()(cJSON* p) const { cJSON_Delete(p); }
};
using CjsonPtr = std::unique_ptr<cJSON, CjsonDeleter>;

struct CjsonStringDeleter {
    void operator()(char* p) const { cJSON_free(p); }
};
using CjsonStringPtr = std::unique_ptr<char, CjsonStringDeleter>;

struct FileDeleter {
    void operator()(FILE* p) const {
        if (p)
            fclose(p);
    }
};
using FilePtr = std::unique_ptr<FILE, FileDeleter>;

// --- read helpers: leave the destination untouched if absent/wrong type ---

void readString(const cJSON* obj, const char* key, char* dst, size_t dstLen) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsString(item) && item->valuestring != nullptr) {
        std::strncpy(dst, item->valuestring, dstLen - 1);
        dst[dstLen - 1] = '\0';
    }
}

void readFloat(const cJSON* obj, const char* key, float& dst) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item)) {
        dst = static_cast<float>(item->valuedouble);
    }
}

void readU16(const cJSON* obj, const char* key, uint16_t& dst) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item)) {
        dst = static_cast<uint16_t>(item->valuedouble);
    }
}

void readU32(const cJSON* obj, const char* key, uint32_t& dst) {
    const cJSON* item = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (cJSON_IsNumber(item)) {
        dst = static_cast<uint32_t>(item->valuedouble);
    }
}

// Parses the whole JSON document into `cfg`, starting from defaults so
// any field the file omits keeps its default value.
bool parseJson(const char* text, NodeConfig& cfg) {
    CjsonPtr root(cJSON_Parse(text));
    if (!root) {
        ESP_LOGW(kTag, "config JSON parse failed");
        return false;
    }

    cfg = NodeConfig::defaults();

    if (const cJSON* id = cJSON_GetObjectItemCaseSensitive(root.get(), "identity")) {
        readString(id, "node_name", cfg.nodeName, kNodeNameMaxLen);
    }

    if (const cJSON* dsp = cJSON_GetObjectItemCaseSensitive(root.get(), "dsp")) {
        readU16(dsp, "secondary_path_taps", cfg.secondaryPathTaps);
        readFloat(dsp, "normalized_step_size", cfg.normalizedStepSize);
        readU16(dsp, "dma_block_samples", cfg.dmaBlockSamples);
    }

    if (const cJSON* cal =
            cJSON_GetObjectItemCaseSensitive(root.get(), "calibration")) {
        readU32(cal, "probe_duration_ms", cfg.probeDurationMs);
        readFloat(cal, "correlation_threshold", cfg.correlationThreshold);
        readU32(cal, "correlation_window_ms", cfg.correlationWindowMs);
        readU32(cal, "backstop_interval_ms", cfg.backstopIntervalMs);
        readU32(cal, "cooldown_ms", cfg.cooldownMs);
    }

    if (const cJSON* trk = cJSON_GetObjectItemCaseSensitive(root.get(), "tracking")) {
        const cJSON* freqs = cJSON_GetObjectItemCaseSensitive(trk, "initial_freq_hz");
        if (cJSON_IsArray(freqs)) {
            int n = cJSON_GetArraySize(freqs);
            if (n > kMaxTones)
                n = kMaxTones;
            for (int i = 0; i < n; ++i) {
                const cJSON* f = cJSON_GetArrayItem(freqs, i);
                if (cJSON_IsNumber(f)) {
                    cfg.initialFreqHz[i] = static_cast<float>(f->valuedouble);
                }
            }
            if (n > 0)
                cfg.numTones = static_cast<uint8_t>(n);
        }
        readFloat(trk, "max_drift_rate_hz_per_sec", cfg.maxDriftRateHzPerSec);
        readFloat(trk, "watch_correlation_threshold", cfg.watchCorrelationThreshold);
    }

    return true;
}

CjsonStringPtr serializeJson(const NodeConfig& cfg) {
    CjsonPtr root(cJSON_CreateObject());
    if (!root)
        return nullptr;

    cJSON* id = cJSON_AddObjectToObject(root.get(), "identity");
    cJSON_AddStringToObject(id, "node_name", cfg.nodeName);

    cJSON* dsp = cJSON_AddObjectToObject(root.get(), "dsp");
    cJSON_AddNumberToObject(dsp, "secondary_path_taps", cfg.secondaryPathTaps);
    cJSON_AddNumberToObject(dsp, "normalized_step_size", cfg.normalizedStepSize);
    cJSON_AddNumberToObject(dsp, "dma_block_samples", cfg.dmaBlockSamples);

    cJSON* cal = cJSON_AddObjectToObject(root.get(), "calibration");
    cJSON_AddNumberToObject(cal, "probe_duration_ms", cfg.probeDurationMs);
    cJSON_AddNumberToObject(cal, "correlation_threshold", cfg.correlationThreshold);
    cJSON_AddNumberToObject(cal, "correlation_window_ms", cfg.correlationWindowMs);
    cJSON_AddNumberToObject(cal, "backstop_interval_ms", cfg.backstopIntervalMs);
    cJSON_AddNumberToObject(cal, "cooldown_ms", cfg.cooldownMs);

    cJSON* trk   = cJSON_AddObjectToObject(root.get(), "tracking");
    cJSON* freqs = cJSON_AddArrayToObject(trk, "initial_freq_hz");
    for (int i = 0; i < cfg.numTones; ++i) {
        cJSON_AddItemToArray(freqs, cJSON_CreateNumber(cfg.initialFreqHz[i]));
    }
    cJSON_AddNumberToObject(trk, "max_drift_rate_hz_per_sec", cfg.maxDriftRateHzPerSec);
    cJSON_AddNumberToObject(trk, "watch_correlation_threshold",
                            cfg.watchCorrelationThreshold);

    return CjsonStringPtr(cJSON_Print(root.get()));
}

}  // namespace

bool ConfigStore::mount() {
    if (mounted_)
        return true;

    esp_vfs_littlefs_conf_t conf = {};
    conf.base_path               = kMountPoint;
    conf.partition_label         = kPartitionLabel;
    conf.format_if_mount_failed  = true;  // blank flash on first boot
    conf.dont_mount              = false;

    esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "littlefs mount failed: %s", esp_err_to_name(err));
        return false;
    }

    mounted_ = true;
    ESP_LOGI(kTag, "littlefs mounted at %s", kMountPoint);
    return true;
}

bool ConfigStore::load(NodeConfig& out) {
    if (!mounted_) {
        ESP_LOGE(kTag, "load before mount");
        return false;
    }

    // Any failure below is recoverable: fall back to defaults and
    // persist them, so the device always boots into a working state.
    bool usable = false;

    FilePtr f(fopen(kConfigPath, "rb"));
    if (f) {
        fseek(f.get(), 0, SEEK_END);
        long size = ftell(f.get());
        fseek(f.get(), 0, SEEK_SET);

        if (size > 0 && size < 8192) {
            std::unique_ptr<char[]> buf(new char[size + 1]);
            size_t read = fread(buf.get(), 1, static_cast<size_t>(size), f.get());
            buf[read]   = '\0';

            if (parseJson(buf.get(), out) && out.validate()) {
                usable = true;
            } else {
                ESP_LOGW(kTag, "config invalid, falling back to defaults");
            }
        } else {
            ESP_LOGW(kTag, "config size implausible (%ld), using defaults", size);
        }
    } else {
        ESP_LOGI(kTag, "no config file, writing defaults");
    }

    if (usable) {
        ESP_LOGI(kTag, "config loaded (node_name=%s)", out.nodeName);
        return true;
    }

    out = NodeConfig::defaults();
    save(out);  // best-effort; a write failure still leaves us runnable
    return true;
}

bool ConfigStore::save(const NodeConfig& cfg) {
    if (!mounted_) {
        ESP_LOGE(kTag, "save before mount");
        return false;
    }
    if (!cfg.validate()) {
        ESP_LOGE(kTag, "refusing to save invalid config");
        return false;
    }

    CjsonStringPtr text = serializeJson(cfg);
    if (!text) {
        ESP_LOGE(kTag, "config serialize failed");
        return false;
    }

    FilePtr f(fopen(kConfigPath, "wb"));
    if (!f) {
        ESP_LOGE(kTag, "cannot open %s for write", kConfigPath);
        return false;
    }

    size_t len     = std::strlen(text.get());
    size_t written = fwrite(text.get(), 1, len, f.get());
    if (written != len) {
        ESP_LOGE(kTag, "short write (%u of %u)", static_cast<unsigned>(written),
                 static_cast<unsigned>(len));
        return false;
    }

    ESP_LOGI(kTag, "config saved");
    return true;
}

}  // namespace anc
