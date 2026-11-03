#pragma once

#include <stddef.h>
#include <stdint.h>

#include "AvdkMultiModel.h"
#include "FaceRecognitionModel.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

class FaceVerifyRuntime {
public:
    FaceVerifyRuntime();
    ~FaceVerifyRuntime();

    void setModelFilePath(const char *path);
    bool init();
    void deinit();
    void releaseInterpreter();

    bool extract(const uint8_t *rgb_image,
                 size_t image_len,
                 FaceVerifyResult *result);

    /* Timings of the most recent extract(), in microseconds. prepare covers
     * MicroInterpreter construction + AllocateTensors, i.e. the model switch-in
     * cost. arenaUsed is the arena high-water mark for this model. */
    unsigned long long lastPrepareUs() const { return last_prepare_us_; }
    unsigned long long lastInputUs() const { return last_input_us_; }
    unsigned long long lastInvokeUs() const { return last_invoke_us_; }
    unsigned long long lastPostUs() const { return last_post_us_; }
    uint32_t lastArenaUsed() const { return last_arena_used_; }

    /* Read-only model accessors used by the memory report. */
    uint8_t *modelData() { return model_.modelData(); }
    uint32_t modelDataSize() const { return model_.modelDataSize(); }

private:
    bool initInterpreter();
    bool copyOutputEmbedding(const TfLiteTensor *output, FaceVerifyResult *result);
    int tensorElementCount(const TfLiteTensor *tensor) const;

    const char *model_file_path_;
    AvdkMultiModel model_;
    unsigned long long last_prepare_us_;
    unsigned long long last_input_us_;
    unsigned long long last_invoke_us_;
    unsigned long long last_post_us_;
    uint32_t last_arena_used_;
};
