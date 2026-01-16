/*
 * SequenceManager.h
 *
 *  Created on: 12 янв. 2026 г.
 *      Author: Dmitry
 */

#ifndef SEQUENCEMANAGER_SEQUENCEMANAGER_H_
#define SEQUENCEMANAGER_SEQUENCEMANAGER_H_

#include <cstdint>
#include <cstddef>

class SequenceManager {
public:
    // Callback тип для embedded (без std::function)
    typedef void (*CanSendCallback)(uint32_t id, bool is_extended,
            						bool is_remote, uint8_t* data,
									uint8_t dlc);

    static constexpr size_t MAX_SEQUENCES = 4;
    static constexpr uint32_t MIN_INTERVAL_MS = 1;

    SequenceManager(CanSendCallback send_cb);

    bool startSequence(uint32_t id,
                      const uint8_t* data,
                      uint8_t dlc,
                      uint32_t count = 0,
                      uint32_t interval_ms = 100);

    bool stopSequence(uint32_t id);
    void stopAllSequences();

    void update(uint32_t current_time_ms);

    size_t getActiveCount() const { return active_count_; }

    void setSendCallback(CanSendCallback callback) {
        send_callback_ = callback;
    }

private:
    struct Sequence {
        uint32_t id;
        uint8_t data[8];
        uint8_t dlc;
        uint32_t total_count;
        uint32_t sent_count;
        uint32_t interval_ms;
        uint32_t last_sent_time;
        bool is_extended;
        bool is_active;
        bool infinite;

        Sequence() : id(0), dlc(0), total_count(0), sent_count(0),
                    interval_ms(0), last_sent_time(0),
                    is_extended(false), is_active(false), infinite(false) {}
    };

    Sequence sequences_[MAX_SEQUENCES];
    size_t active_count_;
    CanSendCallback send_callback_;

    Sequence* findSequenceById(uint32_t id);
    Sequence* findFreeSlot();
    void sendMessage(Sequence& seq, uint32_t current_time);
};



#endif /* SEQUENCEMANAGER_SEQUENCEMANAGER_H_ */
