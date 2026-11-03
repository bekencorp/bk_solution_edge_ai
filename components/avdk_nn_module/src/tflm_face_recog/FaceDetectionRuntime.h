#pragma once

#include <stdint.h>

#include "AvdkMultiModel.h"
#include "tensorflow/lite/micro/micro_interpreter.h"

constexpr int kFaceDetectKeypointCount = 5;

struct FacePoint {
    float x;
    float y;
};

struct FaceDetection {
    float x1;
    float y1;
    float x2;
    float y2;
    float score;
    int order;
    FacePoint keypoints[kFaceDetectKeypointCount];
};

class FaceDetectionRuntime {
public:
    FaceDetectionRuntime();
    ~FaceDetectionRuntime();

    void setModelFilePath(const char *path);
    bool init();
    void deinit();
    tflite::MicroInterpreter *prepareInterpreter();
    void releaseInterpreter();
    void resetOutputSpecs();

    bool buildOutputSpecs(tflite::MicroInterpreter *interpreter);
    int invokeAndDecode(tflite::MicroInterpreter *interpreter, FaceDetection *out, int max_out);

    /* Timings of the most recent invokeAndDecode(), in microseconds, plus the
     * tensor-arena high-water mark seen by this model. Filled before the
     * per-run log lines are printed, so the values are stable for reporting. */
    unsigned long long lastInvokeUs() const { return last_invoke_us_; }
    unsigned long long lastPostUs() const { return last_post_us_; }
    uint32_t lastArenaUsed() const { return last_arena_used_; }

    /* Read-only model accessors used by the memory report. */
    uint8_t *modelData() { return model_.modelData(); }
    uint32_t modelDataSize() const { return model_.modelDataSize(); }

private:
    struct OutputSpec {
        int stride;
        int feature_count;
        TfLiteTensor *score;
        TfLiteTensor *bbox;
        TfLiteTensor *keypoints;
    };

    float tensorValue(const TfLiteTensor *tensor, int flat_index) const;
    float keypointTensorValue(const TfLiteTensor *tensor, int logical_idx, int channel, int stride) const;
    int decode(FaceDetection *candidates, int max_candidates);
    int nms(FaceDetection *candidates, int count, FaceDetection *out, int max_out);

    OutputSpec specs_[3];
    FaceDetection *candidates_;
    AvdkMultiModel model_;
    const char *model_file_path_;
    bool output_specs_ready_;
    unsigned long long last_invoke_us_;
    unsigned long long last_post_us_;
    uint32_t last_arena_used_;
};
