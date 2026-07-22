/** @file inc/commu/commu_queue.hpp
 *  @brief Lock-free ring buffer for packet queueing
 *  @author hdkghc
 *  @version 0.1
 *
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  SPDX-FileCopyrightText: 2026 hdkghc <peitongxin@outlook.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef COMMU_QUEUE_HPP
#define COMMU_QUEUE_HPP

#include "commu_types.hpp"
#include <cstring>

namespace commu {

    /**
     * @brief Lock-free ring buffer for packet storage
     * 
     * Designed to be safe for use in I2C interrupt handlers and main loop.
     * Single-producer, single-consumer (SPSC) model.
     * 
     * @tparam QUEUE_SIZE Number of packets the queue can hold
     * 
     * @code
     * PacketQueue<8> queue;
     * uint8_t packet[MAX_PACKET];
     * size_t len = 42;
     * 
     * // In interrupt handler (producer):
     * queue.enqueue(packet, len);
     * 
     * // In main loop (consumer):
     * uint8_t out[MAX_PACKET];
     * size_t out_len;
     * if (queue.dequeue(out, out_len)) {
     *     // process packet
     * }
     * @endcode
     */
    template<size_t QUEUE_SIZE>
    class PacketQueue {
        public:
            PacketQueue() : head_(0), tail_(0) {}

            /**
             * @brief Enqueue a packet
             * @param data Pointer to packet data
             * @param len  Packet length (must be <= MAX_PACKET)
             * @return     true if enqueued, false if queue is full
             */
            bool enqueue(const uint8_t *data, size_t len) {
                if (len > MAX_PACKET) return false;

                int next = (tail_ + 1) % QUEUE_SIZE;
                if (next == head_) return false;  // Full

                memcpy(queue_[tail_], data, len);
                len_[tail_] = len;
                tail_ = next;
                return true;
            }

            /**
             * @brief Dequeue a packet
             * @param buf Output buffer (must be at least MAX_PACKET)
             * @param len Output: packet length
             * @return    true if dequeued, false if queue is empty
             */
            bool dequeue(uint8_t *buf, size_t &len) {
                if (head_ == tail_) return false;

                len = len_[head_];
                memcpy(buf, queue_[head_], len);
                head_ = (head_ + 1) % QUEUE_SIZE;
                return true;
            }

            /** @brief Check if queue is empty */
            bool is_empty() const { return head_ == tail_; }

            /** @brief Get current queue usage count */
            int count() const {
                if (tail_ >= head_) return tail_ - head_;
                return QUEUE_SIZE - head_ + tail_;
            }

        private:
            uint8_t queue_[QUEUE_SIZE][MAX_PACKET];
            size_t  len_[QUEUE_SIZE];
            volatile int head_;
            volatile int tail_;
    };

} // namespace commu

#endif // COMMU_QUEUE_HPP