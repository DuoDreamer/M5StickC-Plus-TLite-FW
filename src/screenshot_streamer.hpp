//! Copyright (c) M5Stack. All rights reserved.
//! Licensed under the MIT license.
//! See LICENSE file in the project root for full license information.

#include <stdint.h>
#include <stddef.h>
#include <atomic>

#include <M5GFX.h>
#include <WiFiClient.h>
#include "jpg/jpge.h"

class screenshot_streamer_t : public jpge::output_stream {
   public:
    // Number of browsers that can watch the stream at the same time (frames
    // are sent to them in turn). One: each viewer costs Wi-Fi/TCP buffers,
    // and with four the largest free heap block fell to ~6 KB, so /json's
    // ~5 KB reply buffer could no longer be allocated (abort).
    static constexpr const int viewer_max = 1;

   private:
    QueueHandle_t _queue_canvas;
    // New viewers (heap-allocated WiFiClient) handed from the web task to
    // streamTask, which then owns and deletes them.
    QueueHandle_t _queue_client;
    QueueHandle_t _queue_bufdata;

    // Viewers owned by streamTask, plus those waiting in _queue_client.
    std::atomic<int> _viewer_count{0};

    struct queue_ss_t {
        M5Canvas* canvas;
        uint16_t y;
    };

    struct queue_bufdata_t {
        const uint8_t* bufdata;
        size_t len;
        bool y0;
    };
    queue_bufdata_t _q_bufdata;

    uint16_t _y      = 0;
    uint16_t _width  = 0;
    uint16_t _height = 0;

    jpge::jpeg_encoder _jpeg_enc;

   public:
    screenshot_streamer_t(void);

    static void streamTask(void*);

    // Hands a connected client to streamTask, which keeps its own handle to
    // the socket, so the caller may stop and reuse its client right away.
    // Returns false when viewer_max viewers are already streaming.
    bool addViewer(const WiFiClient& client);
    bool hasFreeSlot(void) const {
        return _viewer_count < viewer_max;
    }

    // 新設
    bool initCapture(uint16_t width, uint16_t height);
    bool addQueue(M5Canvas* canvas, uint16_t y);

    enum process_result_t {
        pr_nothing,
        pr_error,
        pr_progress,
        pr_complete,
    };

    process_result_t processCapture(void);

    uint get_size(void) const override {
        return 0;
    }
    bool put_buf(const void* Pbuf, int len) override;
};

extern screenshot_streamer_t screenshot_holder;
