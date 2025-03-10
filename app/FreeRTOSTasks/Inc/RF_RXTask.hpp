#pragma once
#include "Task.hpp"
#include "at86rf215.hpp"
#include "etl/optional.h"
#include "main.h"
#include "TaskConfigs.hpp"

extern CRC_HandleTypeDef hcrc;

#define MIN_TC_SIZE 11
#define MAX_TC_SIZE 256

using namespace AT86RF215;

struct ParsedPacket {
    uint8_t packet_version_number;
    uint8_t packet_type;
    uint8_t secondary_header_flag;
    uint16_t application_process_ID;
};

inline uint8_t RX_BUFF[1024] __attribute__((section(".dtcmram_rx_buff"), aligned(4)));


class RF_RXTask : public Task {
public:
    RF_RXTask() : Task("RF RX TASK"){}
    void ensureRxMode();
    static bool verifyCRC(uint8_t* RX_BUFF, int32_t corrected_received_length);
    static ParsedPacket parsePacket(const uint8_t* RX_BUFF);
    [[noreturn]] void execute();
    void createTask() {
        this->taskHandle = xTaskCreateStatic(vClassTask<RF_RXTask>, this->TaskName,
                                             RF_RXTaskStack, this, RF_RXTaskPriority,
                                             this->taskStack, &(this->taskBuffer));
    }

private:
    constexpr static uint16_t TaskStackDepth = 10000;
    /// Frequency in kHz
    constexpr static uint32_t FrequencyUHFRX = 401000;
    AT86RF215::Error error = NO_ERRORS;
    StackType_t taskStack[RF_RXTaskStack]{};
};

inline etl::optional<RF_RXTask> rf_rxtask;