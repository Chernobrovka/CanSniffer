/*
 * SequenceManager.cpp
 *
 *  Created on: 12 янв. 2026 г.
 *      Author: Dmitry
 */

#include "SequenceManager.h"
#include <cstring>

SequenceManager::SequenceManager(CanSendCallback send_cb)
    : active_count_(0),
      send_callback_(send_cb) {
    for (auto& seq : sequences_) {
        seq.is_active = false;
    }
}

bool SequenceManager::startSequence(uint32_t id,
                      const uint8_t* data,
                      uint8_t dlc,
                      uint32_t count,
                      uint32_t interval_ms){
	if (dlc == 0 || dlc > 8) return false;

	if (interval_ms < MIN_INTERVAL_MS)
		interval_ms = MIN_INTERVAL_MS;

	if (active_count_ >= MAX_SEQUENCES){
		return false; // Достигнут лимит последовательностей // TODO добавить точный статус
	}

    // Проверяем, нет ли уже активной последовательности с таким ID
    Sequence* existing_seq = findSequenceById(id);
    if (existing_seq && existing_seq->is_active) {
        // Можно либо остановить старую, либо вернуть ошибку
        stopSequence(id);
    }

    // Находим свободный слот
    Sequence* seq = findFreeSlot();
    if (!seq) {
        return false;
    }

    // Инициализируем последовательность
    seq->id = id;
    seq->dlc = dlc;
    memcpy(seq->data, data, dlc);
    seq->total_count = count;
    seq->sent_count = 0;
    seq->interval_ms = interval_ms;
    seq->last_sent_time = 0;  // Будет установлено при первой отправке
    seq->is_extended = (id > 0x7FF);
    seq->is_active = true;
    seq->infinite = (count == 0) ? true : false; // если cnt == 0 это бесконечная оптравка

    active_count_++;

    return true;
}

bool SequenceManager::stopSequence(uint32_t id) {
    Sequence* seq = findSequenceById(id);
    if (!seq || !seq->is_active) {
        return false;
    }

    seq->is_active = false;
    active_count_--;

    return true;
}

void SequenceManager::stopAllSequences() {
    for (auto& seq : sequences_) {
        if (seq.is_active) {
            seq.is_active = false;
        }
    }
    active_count_ = 0;
}

void SequenceManager::update(uint32_t current_time_ms) {
    if (!send_callback_) {
        return;  // Нет callback для отправки
    }

    for (auto& seq : sequences_) {
        if (!seq.is_active) {
            continue;  // Пропускаем неактивные
        }

        // Проверяем, нужно ли отправить сообщение
        if (seq.last_sent_time == 0) {
            // Первая отправка - отправляем сразу
            sendMessage(seq, current_time_ms);
        } else {
            // Проверяем интервал
            uint32_t time_since_last = current_time_ms - seq.last_sent_time;

            if (time_since_last >= seq.interval_ms) {
                sendMessage(seq, current_time_ms);

                // Проверяем завершение конечной последовательности
                if (!seq.infinite && seq.sent_count >= seq.total_count) {
                    seq.is_active = false;
                    active_count_--;
                }
            }
        }
    }
}

SequenceManager::Sequence* SequenceManager::findSequenceById(uint32_t id) {
    for (auto& seq : sequences_) {
        if (seq.id == id) {
            return &seq;
        }
    }
    return nullptr;
}

SequenceManager::Sequence* SequenceManager::findFreeSlot() {
    for (auto& seq : sequences_) {
        if (!seq.is_active) {
            return &seq;
        }
    }
    return nullptr;
}

void SequenceManager::sendMessage(Sequence& seq, uint32_t current_time) {
    if (!send_callback_) {
        return;
    }

    send_callback_(seq.id, seq.is_extended, false, seq.data, seq.dlc);

    seq.sent_count++;
    seq.last_sent_time = current_time;
}
