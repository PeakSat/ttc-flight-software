#include "CANParserTask.hpp"
#include "ApplicationLayer.hpp"
#include "CANGatekeeperTask.hpp"

#include <TCHandlingTask.hpp>
#include <TMHandlingTask.hpp>
#include <TPProtocol.hpp>
#include <eMMC.hpp>

void CANParserTask::execute() {

    uint32_t ulNotifiedValue;
    while (true) {

        xTaskNotifyWait(0, 0, &ulNotifiedValue, pdMS_TO_TICKS(1000));
        while (uxQueueMessagesWaiting(CANRxQueue)) {

            // Get packet from eMMC
            struct localPacketHandler CANPacketHandler;
            memoryQueueItemHandler dequeueHandler{};
            xQueueReceive(CANRxQueue, &dequeueHandler, portMAX_DELAY);


            if (uxQueueMessagesWaiting(CANRxQueue) == 0) {

                eMMC::getItemFromQueue(eMMC::memoryQueueMap[eMMC::testData], dequeueHandler, reinterpret_cast<uint8_t*>(&CANPacketHandler), dequeueHandler.size);
                LOG_DEBUG << "INCOMING CAN MESSAGE OF SIZE: " << CANPacketHandler.PacketSize;

                // Send ACK
                CAN::TPMessage ACKmessage = {{CAN::NodeID, CAN::NodeIDs::OBC, false}};
                ACKmessage.appendUint8(CAN::Application::MessageIDs::ACK);
                CAN::TPProtocol::createCANTPMessageNoRetransmit(ACKmessage, false);
                xTaskNotifyGive(canGatekeeperTask->taskHandle);

                // parse

                uint8_t messageID = static_cast<CAN::Application::MessageIDs>(CANPacketHandler.MessageID);
                if (messageID == CAN::Application::CCSDSPacket) {
                    TX_PACKET_HANDLER tm_handler{};
                    for (int i = 0; i < CANPacketHandler.PacketSize; i++) {
                        tm_handler.buf[i] = CANPacketHandler.Buffer[i];
                    }
                    tm_handler.data_length = CANPacketHandler.PacketSize;
                    xQueueSendToBack(TMQueue, &tm_handler, NULL);
                    if (tmhandlingTask->taskHandle != nullptr) {
                        xTaskNotifyIndexed(tmhandlingTask->taskHandle, NOTIFY_INDEX_RECEIVED_TM, TM_OBC_TM_HANDLING, eSetBits);
                    }
                } else {
                    CAN::TPMessage message;
                    message.appendUint8(CANPacketHandler.MessageID);
                    for (int i = 0; i < CANPacketHandler.PacketSize; i++) {
                        message.appendUint8(CANPacketHandler.Buffer[i]);
                    }
                    message.idInfo.sourceAddress = CAN::OBC;
                    CAN::TPProtocol::parseMessage(message);
                }

            } else {
                LOG_DEBUG << "Old message discarded";
            }
        }
    }
}