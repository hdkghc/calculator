/** @file inc/commu/file_transfer.hpp
 *  @brief Asynchronous file transfer using state machine
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
#include "fsl_common.h"
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
     * @brief Transfer state machine states
     */
    enum class TransferState : uint8_t {
        IDLE,
        WAIT_START,          ///< Waiting for file start command (receive only)
        SEND_META,           ///< Sending file metadata
        SEND_DATA,           ///< Sending file data chunks
        SEND_END,            ///< Sending end command
        RECV_DATA,           ///< Receiving file data chunks
        WAIT_END,            ///< Waiting for end command (receive only)
        COMPLETE,            ///< Transfer completed successfully
        ERROR                ///< Transfer failed
    };

    /**
     * @brief Transfer status (volatile for visibility across contexts)
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
     * @brief Asynchronous file transfer manager using state machine
     * 
     * Single-core design: call process() regularly in main loop to advance
     * the state machine. This eliminates the need for a second core.
     * 
     * @code
     * // In main():
     * FileTransfer ft(comm, fatfs);
     * 
     * while (true) {
     *     ft.process();
     *     // Update screen with ft.status().progress_percent
     *     delay_ms(10);
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
                : comm_(comm), fatfs_(fatfs), callback_(nullptr),
                state_(TransferState::IDLE),
                chunk_buffer_{0}, chunk_size_(0), chunk_offset_(0) {
                memset(&status_, 0, sizeof(status_));
                status_.direction = TransferDirection::IDLE;
            }

            /**
             * @brief Initialize transfer manager
             * Called once before any transfer (RT1011 is single core, no core init needed)
             */
            void init() {
                // Nothing special needed for single core
                // File system and communication should be initialized elsewhere
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

                // Store file handle for later use
                file_handle_ = file;

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
                state_ = TransferState::SEND_META;
                chunk_offset_ = 0;
                
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
                state_ = TransferState::WAIT_START;
                total_received_ = 0;
                meta_received_ = false;
                
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
             * @brief Process state machine - call this regularly in main loop
             * @return true if transfer is still active, false if idle or complete
             */
            bool process() {
                // Handle completion callbacks
                if (status_.is_complete || status_.has_error) {
                    if (callback_ && status_.is_active) {
                        callback_(status_.is_complete, status_.filename);
                    }
                    // Reset status after callback
                    if (status_.is_complete || status_.has_error) {
                        status_.is_active = false;
                        status_.direction = TransferDirection::IDLE;
                        state_ = TransferState::IDLE;
                    }
                    return false;
                }

                // Check cancellation
                if (status_.cancel_requested) {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                    return false;
                }

                // State machine execution
                switch (state_) {
                    case TransferState::SEND_META:
                        do_send_meta();
                        break;
                    case TransferState::SEND_DATA:
                        do_send_data();
                        break;
                    case TransferState::SEND_END:
                        do_send_end();
                        break;
                    case TransferState::WAIT_START:
                        do_wait_start();
                        break;
                    case TransferState::RECV_DATA:
                        do_recv_data();
                        break;
                    case TransferState::WAIT_END:
                        do_wait_end();
                        break;
                    case TransferState::IDLE:
                    case TransferState::COMPLETE:
                    case TransferState::ERROR:
                    default:
                        break;
                }

                return status_.is_active;
            }

            /**
             * @brief Check if a transfer is in progress
             */
            bool is_active() const { return status_.is_active; }

            /**
             * @brief Get current progress percentage (0-100)
             */
            uint32_t get_progress() const { return status_.progress_percent; }

        private:
            // ---- Send state machine functions ----

            /**
             * @brief Send file metadata (start packet)
             */
            void do_send_meta() {
                FileMeta meta;
                meta.total_size = status_.total_bytes;
                memset(meta.filename, 0, sizeof(meta.filename));
                strncpy((char*)meta.filename, status_.filename, sizeof(meta.filename) - 1);

                if (comm_.send_packet(CMD_FILE_START, (uint8_t*)&meta, sizeof(meta))) {
                    state_ = TransferState::SEND_DATA;
                    chunk_offset_ = 0;
                } else {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                }
            }

            /**
             * @brief Send file data in chunks
             */
            void do_send_data() {
                // Check connection
                if (!comm_.is_connected()) {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                    return;
                }

                // Check if all data sent
                if (status_.bytes_transferred >= status_.total_bytes) {
                    state_ = TransferState::SEND_END;
                    return;
                }

                // Read next chunk from SD card
                uint32_t remaining = status_.total_bytes - status_.bytes_transferred;
                chunk_size_ = (remaining < MAX_PAYLOAD) ? remaining : MAX_PAYLOAD;

                if (fatfs_->read(file_handle_, chunk_buffer_, chunk_size_) != chunk_size_) {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                    return;
                }

                // Send data packet
                if (comm_.send_packet(CMD_FILE_DATA, chunk_buffer_, chunk_size_)) {
                    status_.bytes_transferred += chunk_size_;
                    status_.progress_percent = (status_.bytes_transferred * 100) / status_.total_bytes;
                    // Stay in SEND_DATA state for next chunk
                } else {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                }
            }

            /**
             * @brief Send end command
             */
            void do_send_end() {
                if (comm_.send_packet(CMD_FILE_END, nullptr, 0)) {
                    status_.is_complete = true;
                    state_ = TransferState::COMPLETE;
                } else {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                }
            }

            // ---- Receive state machine functions ----

            /**
             * @brief Wait for start packet from master
             */
            void do_wait_start() {
                uint8_t cmd;
                uint8_t buf[MAX_PAYLOAD];
                size_t len;

                if (!comm_.recv_packet(cmd, buf, len, 50)) {
                    // No packet received, stay in this state
                    return;
                }

                if (cmd == CMD_FILE_START) {
                    FileMeta *meta = (FileMeta*)buf;
                    status_.total_bytes = meta->total_size;
                    meta_received_ = true;
                    total_received_ = 0;
                    state_ = TransferState::RECV_DATA;
                } else {
                    // Unexpected command, ignore or error
                    // For robustness, we could ignore and continue waiting
                }
            }

            /**
             * @brief Receive file data chunks
             */
            void do_recv_data() {
                if (!comm_.is_connected()) {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                    return;
                }

                uint8_t cmd;
                uint8_t buf[MAX_PAYLOAD];
                size_t len;

                if (!comm_.recv_packet(cmd, buf, len, 50)) {
                    // No packet, stay in this state
                    return;
                }

                if (cmd == CMD_FILE_DATA) {
                    // Write data to SD card
                    if (!fatfs_->append_file(status_.filename, buf, len)) {
                        status_.has_error = true;
                        state_ = TransferState::ERROR;
                        return;
                    }
                    total_received_ += len;
                    status_.bytes_transferred = total_received_;
                    if (status_.total_bytes > 0) {
                        status_.progress_percent = (total_received_ * 100) / status_.total_bytes;
                    }
                    // Stay in RECV_DATA state for next packet
                } else if (cmd == CMD_FILE_END) {
                    // Transfer complete
                    status_.is_complete = true;
                    state_ = TransferState::COMPLETE;
                } else {
                    // Unexpected command
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                }
            }

            /**
             * @brief Wait for end command (fallback if not received in RECV_DATA)
             */
            void do_wait_end() {
                uint8_t cmd;
                uint8_t buf[MAX_PAYLOAD];
                size_t len;

                if (!comm_.recv_packet(cmd, buf, len, 100)) {
                    // Timeout waiting for end
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                    return;
                }

                if (cmd == CMD_FILE_END) {
                    status_.is_complete = true;
                    state_ = TransferState::COMPLETE;
                } else {
                    status_.has_error = true;
                    state_ = TransferState::ERROR;
                }
            }

            // ---- Member variables ----

            AudioCommu &comm_;
            SDCard::FATFS *fatfs_;
            TransferCallback callback_;
            
            TransferStatus status_;
            TransferState state_;
            
            // File handle for ongoing operations
            SDCard::FATFS::File file_handle_;
            
            // Chunk buffer
            uint8_t chunk_buffer_[MAX_PAYLOAD];
            uint32_t chunk_size_;
            uint32_t chunk_offset_;
            
            // Receive state variables
            bool meta_received_;
            uint32_t total_received_;
    };

} // namespace commu

#endif // FILE_TRANSFER_HPP