/** @file inc/commu/file_transfer.hpp
 *  @brief Asynchronous file transfer using dual-core + state machine
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

#ifndef FILE_TRANSFER_HPP
#define FILE_TRANSFER_HPP

#include <cstdint>
#include <cstring>
#include <functional>
#include "pico/multicore.h"
#include "commu/commu_audio.hpp"
#include "sdcard.hpp"

namespace commu {

    /**
     * @brief Transfer direction
     */
    enum class TransferDirection : uint8_t {
        IDLE,
        SEND,
        RECEIVE
    };

    /**
     * @brief Transfer status (shared between cores, volatile for visibility)
     */
    struct TransferStatus {
        volatile TransferDirection direction;  ///< Send or receive
        volatile uint32_t total_bytes;         ///< Total file size
        volatile uint32_t bytes_transferred;   ///< Bytes transferred so far
        volatile uint32_t progress_percent;    ///< 0-100
        volatile bool is_active;               ///< Transfer in progress
        volatile bool is_complete;             ///< Transfer finished successfully
        volatile bool has_error;               ///< Transfer failed
        volatile bool cancel_requested;        ///< User requested cancellation
        char filename[32];                     ///< 8.3 filename (max 31 chars)

        TransferStatus() {
            memset(this, 0, sizeof(*this));
        }
    };

    /**
     * @brief Callback type for transfer completion
     */
    using TransferCallback = std::function<void(bool success, const char *filename)>;

    /**
     * @brief Dual-core asynchronous file transfer manager
     * 
     * Core 0: main loop calls process() to update status
     * Core 1: runs transfer_worker() in background
     * 
     * @code
     * // In main():
     * FileTransfer ft(comm, fatfs);
     * ft.init_core1();
     * 
     * while (true) {
     *     ft.process();
     *     // Update screen with ft.status().progress_percent
     *     sleep_ms(10);
     * }
     * @endcode
     */
    class FileTransfer {
        public:
            /**
             * @brief Constructor
             * @param comm   Reference to AudioCommu instance
             * @param fatfs  Pointer to FATFS instance
             */
            FileTransfer(AudioCommu &comm, SDCard::FATFS *fatfs)
                : comm_(comm), fatfs_(fatfs), callback_(nullptr) {
                status_.direction = TransferDirection::IDLE;
                status_.is_active = false;
                status_.is_complete = false;
                status_.has_error = false;
                status_.cancel_requested = false;
                core1_ready_ = false;
            }

            /**
             * @brief Initialize core 1 to run transfer worker
             * Must be called once before any transfer
             */
            void init_core1() {
                if (!core1_ready_) {
                    multicore_launch_core1(transfer_worker_entry);
                    // Wait for core 1 to signal ready
                    while (!core1_ready_) {
                        tight_loop_contents();
                    }
                    core1_ready_ = true;
                }
            }

            /**
             * @brief Start sending a file asynchronously
             * @param filename 8.3 filename on local SD card
             * @param callback Optional callback on completion
             * @return true if transfer started
             */
            bool send_file(const char *filename, TransferCallback callback = nullptr) {
                if (!fatfs_ || !comm_.is_connected() || !comm_.is_master()) return false;
                if (status_.is_active) return false;

                // Check file exists and get size
                SDCard::FATFS::File file;
                if (!fatfs_->open(filename, file)) return false;

                // Set up status
                status_.direction = TransferDirection::SEND;
                status_.total_bytes = file.file_size;
                status_.bytes_transferred = 0;
                status_.progress_percent = 0;
                status_.is_active = true;
                status_.is_complete = false;
                status_.has_error = false;
                status_.cancel_requested = false;
                strncpy(status_.filename, filename, sizeof(status_.filename) - 1);
                status_.filename[sizeof(status_.filename) - 1] = '\0';

                callback_ = callback;

                // Signal core 1 to start working
                transfer_pending_ = true;
                return true;
            }

            /**
             * @brief Start receiving a file asynchronously
             * @param filename 8.3 filename to save on local SD card
             * @param callback Optional callback on completion
             * @return true if transfer started
             */
            bool recv_file(const char *filename, TransferCallback callback = nullptr) {
                if (!fatfs_ || !comm_.is_connected()) return false;
                if (status_.is_active) return false;

                status_.direction = TransferDirection::RECEIVE;
                status_.total_bytes = 0;
                status_.bytes_transferred = 0;
                status_.progress_percent = 0;
                status_.is_active = true;
                status_.is_complete = false;
                status_.has_error = false;
                status_.cancel_requested = false;
                strncpy(status_.filename, filename, sizeof(status_.filename) - 1);
                status_.filename[sizeof(status_.filename) - 1] = '\0';

                callback_ = callback;
                transfer_pending_ = true;
                return true;
            }

            /**
             * @brief Cancel ongoing transfer
             */
            void cancel() {
                status_.cancel_requested = true;
            }

            /**
             * @brief Get current transfer status (thread-safe read)
             */
            TransferStatus get_status() const {
                TransferStatus s;
                s.direction = status_.direction;
                s.total_bytes = status_.total_bytes;
                s.bytes_transferred = status_.bytes_transferred;
                s.progress_percent = status_.progress_percent;
                s.is_active = status_.is_active;
                s.is_complete = status_.is_complete;
                s.has_error = status_.has_error;
                s.cancel_requested = status_.cancel_requested;
                strcpy(s.filename, status_.filename);
                return s;
            }

            /**
             * @brief Process status updates on core 0
             * Call this in main loop to handle completion callbacks
             */
            void process() {
                // Check if transfer completed on core 1
                if (status_.is_complete || status_.has_error) {
                    if (callback_ && status_.is_active) {
                        callback_(status_.is_complete, status_.filename);
                    }
                    // Reset status (keep for one loop, then clear)
                    if (status_.is_complete || status_.has_error) {
                        status_.is_active = false;
                        status_.direction = TransferDirection::IDLE;
                    }
                }
            }

        private:
            /**
             * @brief Transfer worker running on core 1
             * 
             * This function runs in a loop, checking for pending transfers.
             * It reads/writes SD card and updates status_ for core 0 to read.
             */
            static void transfer_worker_entry() {
                // Signal that core 1 is ready
                core1_ready_ = true;

                // Wait for transfers to be triggered
                while (true) {
                    // Check if a transfer is pending
                    if (transfer_pending_) {
                        transfer_pending_ = false;

                        if (status_.direction == TransferDirection::SEND) {
                            do_send_worker();
                        } else if (status_.direction == TransferDirection::RECEIVE) {
                            do_recv_worker();
                        }
                    }

                    // Yield to other tasks
                    tight_loop_contents();
                    sleep_us(10);
                }
            }

            /**
             * @brief Send worker: read from SD card, send packets
             */
            static void do_send_worker() {
                SDCard::FATFS::File file;
                if (!fatfs_->open(status_.filename, file)) {
                    status_.has_error = true;
                    return;
                }

                // 1. Send file metadata (total size + filename)
                FileMeta meta;
                meta.total_size = status_.total_bytes;
                memset(meta.filename, 0, sizeof(meta.filename));
                strncpy((char*)meta.filename, status_.filename, sizeof(meta.filename) - 1);
                if (!comm_.send_packet(CMD_FILE_START, (uint8_t*)&meta, sizeof(meta))) {
                    status_.has_error = true;
                    return;
                }

                // 2. Send file data in chunks
                uint8_t buffer[MAX_PAYLOAD];
                uint32_t sent = 0;

                while (sent < status_.total_bytes) {
                    // Check cancellation
                    if (status_.cancel_requested) {
                        status_.has_error = true;
                        return;
                    }

                    // Check connection
                    if (!comm_.is_connected()) {
                        status_.has_error = true;
                        return;
                    }

                    uint32_t chunk = std::min<uint32_t>(MAX_PAYLOAD, status_.total_bytes - sent);
                    if (fatfs_->read(file, buffer, chunk) != chunk) {
                        status_.has_error = true;
                        return;
                    }

                    if (!comm_.send_packet(CMD_FILE_DATA, buffer, chunk)) {
                        status_.has_error = true;
                        return;
                    }

                    sent += chunk;
                    status_.bytes_transferred = sent;
                    status_.progress_percent = (sent * 100) / status_.total_bytes;
                }

                // 3. Send END packet
                if (!comm_.send_packet(CMD_FILE_END, nullptr, 0)) {
                    status_.has_error = true;
                    return;
                }

                status_.is_complete = true;
            }

            /**
             * @brief Receive worker: receive packets, write to SD card
             */
            static void do_recv_worker() {
                uint8_t cmd;
                uint8_t buf[MAX_PAYLOAD];
                size_t len;
                bool meta_received = false;
                uint32_t total_received = 0;

                while (true) {
                    // Check cancellation
                    if (status_.cancel_requested) {
                        status_.has_error = true;
                        return;
                    }

                    // Check connection
                    if (!comm_.is_connected()) {
                        status_.has_error = true;
                        return;
                    }

                    if (!comm_.recv_packet(cmd, buf, len, 100)) {
                        status_.has_error = true;
                        return;
                    }

                    if (cmd == CMD_FILE_START) {
                        // Receive metadata
                        FileMeta *meta = (FileMeta*)buf;
                        status_.total_bytes = meta->total_size;
                        meta_received = true;
                        // Filename already set by user
                        continue;
                    }

                    if (cmd == CMD_FILE_DATA) {
                        if (!meta_received) {
                            status_.has_error = true;
                            return;
                        }
                        if (!fatfs_->append_file(status_.filename, buf, len)) {
                            status_.has_error = true;
                            return;
                        }
                        total_received += len;
                        status_.bytes_transferred = total_received;
                        status_.progress_percent = (total_received * 100) / status_.total_bytes;
                        continue;
                    }

                    if (cmd == CMD_FILE_END) {
                        if (!meta_received) {
                            status_.has_error = true;
                            return;
                        }
                        status_.is_complete = true;
                        return;
                    }

                    // Unexpected command
                    status_.has_error = true;
                    return;
                }
            }

            // ---- Static members (shared between cores) ----
            static AudioCommu *comm_;
            static SDCard::FATFS *fatfs_;
            static TransferStatus status_;
            static TransferCallback callback_;
            static volatile bool transfer_pending_;
            static volatile bool core1_ready_;
    };

    // ---- Static member definitions ----
    AudioCommu* FileTransfer::comm_ = nullptr;
    SDCard::FATFS* FileTransfer::fatfs_ = nullptr;
    TransferStatus FileTransfer::status_;
    TransferCallback FileTransfer::callback_;
    volatile bool FileTransfer::transfer_pending_ = false;
    volatile bool FileTransfer::core1_ready_ = false;

} // namespace commu

#endif // FILE_TRANSFER_HPP