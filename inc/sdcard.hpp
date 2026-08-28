/** @file /inc/sdcard.hpp
 *  @brief Ultra-fast software SPI SD card interface with FAT16/FAT32 support
 *  @author hdkghc
 *  @version 0.1
 *
 *  @details This driver uses direct register manipulation for maximum performance.
 *           The SPI bit-banging loop is hand-optimized. All GPIO operations
 *           bypass HAL for speed. Supports SDHC/SDXC cards (>2GB).
 *
 *  Pin assignments (GPIO2):
 *  - MISO: GPIO_SD_12 (GPIO2, Pin 12) - Input
 *  - MOSI: GPIO_SD_00 (GPIO2, Pin 0)  - Output
 *  - SCK:  GPIO_SD_13 (GPIO2, Pin 13) - Output
 *  - CS:   GPIO_SD_05 (GPIO2, Pin 5)  - Output
 *
 *  @example
 *  @code
 *  SDCard::SDCardIO card;
 *  SDCard::FATFS fatfs(card);
 *
 *  if (card.init() && fatfs.mount()) {
 *      std::string content;
 *      if (fatfs.read_file("README.TXT", content)) {
 *          printf("File: %s\n", content.c_str());
 *      }
 *      fatfs.rename_file("OLD.TXT", "NEW.TXT");
 *      fatfs.mkdir("DATA");
 *      fatfs.format("MYCARD");
 *  }
 *  @endcode
 *
 *  Copyright (C) 2026 hdkghc (peitongxin@outlook.com)
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

#ifndef SDCARD_HPP
#define SDCARD_HPP

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>
#include <cctype>

#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_device_registers.h"
#include "fsl_iomuxc.h"

namespace SDCard {

    // ================================================================
    // Constants
    // ================================================================

    /**
     * @brief SD card SPI command codes
     */
    enum SDCommand : uint8_t {
        CMD0    = 0x00,     /**< GO_IDLE_STATE - Reset card to idle */
        CMD8    = 0x08,     /**< SEND_IF_COND - Check voltage range */
        CMD9    = 0x09,     /**< SEND_CSD - Read CSD register */
        CMD17   = 0x11,     /**< READ_SINGLE_BLOCK - Read one block */
        CMD24   = 0x18,     /**< WRITE_BLOCK - Write one block */
        CMD55   = 0x37,     /**< APP_CMD - Prefix for ACMD */
        ACMD41  = 0x29,     /**< SD_SEND_OP_COND - Initialize card */
        CMD58   = 0x3A,     /**< READ_OCR - Read OCR register */
        CMD59   = 0x3B,     /**< CRC_ON_OFF - Enable/disable CRC */
    };

    /**
     * @brief SD card response tokens
     */
    enum SDResponse : uint8_t {
        R1_IDLE         = 0x01,     /**< Card in idle state */
        R1_ILLEGAL      = 0x04,     /**< Illegal command */
        R1_CRC_ERROR    = 0x08,     /**< CRC error */
        R1_ERASE_ERROR  = 0x10,     /**< Erase error */
        R1_ADDRESS_ERROR= 0x20,     /**< Address error */
        R1_PARAM_ERROR  = 0x40,     /**< Parameter error */
        DATA_START_TOKEN= 0xFE,     /**< Start of data block */
        DATA_RESPONSE   = 0x05,     /**< Data accepted (bits 0-4 = 00101) */
    };

    /** @brief Maximum retries for command/block operations */
    static constexpr int MAX_RETRIES = 3;

    /** @brief Maximum clusters to traverse (safety limit) */
    static constexpr uint32_t MAX_CLUSTER_TRAVERSE = 0x1000000;

    /** @brief SD card block size in bytes */
    static constexpr int BLOCK_SIZE = 512;

    // ================================================================
    // GPIO2 Register Definitions
    // ================================================================
    // GPIO2 Base Address: 0x4200_0000 (per RM Table 12-1)
    // Register offsets:
    //   0x00: DR (Data Register)
    //   0x04: GDIR (Direction Register)
    //   0x08: PSR (Pad Status Register)
    //   0x84: DR_SET - Write 1 to set bits (atomic)
    //   0x88: DR_CLEAR - Write 1 to clear bits (atomic)
    //   0x8C: DR_TOGGLE - Write 1 to toggle bits (atomic)
    // ================================================================
    static constexpr uint32_t GPIO2_BASE     = 0x42000000;
    static constexpr uint32_t GPIO2_DR       = GPIO2_BASE + 0x00;
    static constexpr uint32_t GPIO2_GDIR     = GPIO2_BASE + 0x04;
    static constexpr uint32_t GPIO2_PSR      = GPIO2_BASE + 0x08;
    static constexpr uint32_t GPIO2_DR_SET   = GPIO2_BASE + 0x84;
    static constexpr uint32_t GPIO2_DR_CLEAR = GPIO2_BASE + 0x88;
    static constexpr uint32_t GPIO2_DR_TOGGLE= GPIO2_BASE + 0x8C;

    // ================================================================
    // IOMUXC Pin Mux Macros (GPIO2 mode)
    // ================================================================
    #ifdef IOMUXC_GPIO_SD_00_GPIO2_IO00
        #define SD_PIN_MOSI  IOMUXC_GPIO_SD_00_GPIO2_IO00
    #else
        #define SD_PIN_MOSI  0x4200U
    #endif

    #ifdef IOMUXC_GPIO_SD_05_GPIO2_IO05
        #define SD_PIN_CS    IOMUXC_GPIO_SD_05_GPIO2_IO05
    #else
        #define SD_PIN_CS    0x4205U
    #endif

    #ifdef IOMUXC_GPIO_SD_12_GPIO2_IO12
        #define SD_PIN_MISO  IOMUXC_GPIO_SD_12_GPIO2_IO12
    #else
        #define SD_PIN_MISO  0x420CU
    #endif

    #ifdef IOMUXC_GPIO_SD_13_GPIO2_IO13
        #define SD_PIN_SCK   IOMUXC_GPIO_SD_13_GPIO2_IO13
    #else
        #define SD_PIN_SCK   0x420DU
    #endif

    // ================================================================
    // SDCardIO - Ultra-Fast Software SPI
    // ================================================================

    /**
     * @brief Ultra-fast software SPI SD card block driver
     *
     * @details Performance-critical code uses direct register manipulation.
     *          All pins are on GPIO2 port for single-port optimization.
     *          Uses DR_SET/DR_CLEAR/DR_TOGGLE for atomic operations.
     *          Supports SDHC (high capacity) cards automatically.
     */
    class SDCardIO {
        public:
            SDCardIO(uint8_t miso = 12, uint8_t mosi = 0,
                     uint8_t sck = 13, uint8_t cs = 5)
                : m_miso(miso), m_mosi(mosi), m_sck(sck), m_cs(cs),
                  m_miso_mask(1UL << miso), m_mosi_mask(1UL << mosi),
                  m_sck_mask(1UL << sck), m_cs_mask(1UL << cs),
                  m_mounted(false), m_last_alloc_cluster(2), m_is_sdhc(false),
                  m_total_sectors(0) {
                // Cache register pointers for performance
                m_psr  = (volatile uint32_t*)(GPIO2_PSR);
                m_set  = (volatile uint32_t*)(GPIO2_DR_SET);
                m_clr  = (volatile uint32_t*)(GPIO2_DR_CLEAR);
                m_tgl  = (volatile uint32_t*)(GPIO2_DR_TOGGLE);
                m_dr   = (volatile uint32_t*)(GPIO2_DR);
                m_gdir = (volatile uint32_t*)(GPIO2_GDIR);
            }

            /**
             * @brief Initialize SD card with ultra-fast SPI
             * @param freq  Requested frequency in Hz (ignored for software SPI)
             * @return true on success, false on failure
             */
            bool init(uint32_t freq = 400 * 1000) {
                (void)freq;

                // ============================================================
                // Step 0: Configure IOMUXC for GPIO mode
                // ============================================================
                #ifdef IOMUXC_GPIO_SD_00_GPIO2_IO00
                    IOMUXC_SetPinMux(SD_PIN_MOSI, 0U);
                #endif
                #ifdef IOMUXC_GPIO_SD_05_GPIO2_IO05
                    IOMUXC_SetPinMux(SD_PIN_CS, 0U);
                #endif
                #ifdef IOMUXC_GPIO_SD_12_GPIO2_IO12
                    IOMUXC_SetPinMux(SD_PIN_MISO, 0U);
                #endif
                #ifdef IOMUXC_GPIO_SD_13_GPIO2_IO13
                    IOMUXC_SetPinMux(SD_PIN_SCK, 0U);
                #endif

                // ============================================================
                // Step 1: Enable GPIO2 clock
                // ============================================================
                CLOCK_EnableClock(kCLOCK_Gpio2);

                // ============================================================
                // Step 2: Configure GPIO2 direction register
                // ============================================================
                // MOSI, SCK, CS are outputs; MISO is input (default 0)
                // ============================================================
                *m_gdir |= (m_mosi_mask | m_sck_mask | m_cs_mask);

                // ============================================================
                // Step 3: Set initial pin states
                // ============================================================
                // CS=HIGH (de-asserted), MOSI=HIGH (idle), SCK=LOW (idle)
                // ============================================================
                *m_set = (m_cs_mask | m_mosi_mask | m_sck_mask);
                *m_clr = m_sck_mask;

                // ============================================================
                // Step 4: Send 80 clock pulses to wake up SD card
                // ============================================================
                for (int i = 0; i < 10; ++i) {
                    fast_spi_xfer(0xFF);
                }

                // ============================================================
                // Step 5: CMD0 - GO_IDLE_STATE
                // ============================================================
                int retry;
                for (retry = 0; retry < 20; ++retry) {
                    if (send_cmd(CMD0, 0) == R1_IDLE) {
                        break;
                    }
                    delay_ms(10);
                }
                if (retry == 20) {
                    return false;
                }

                // ============================================================
                // Step 6: CMD8 - SEND_IF_COND
                // ============================================================
                if (send_cmd(CMD8, 0x1AA) != R1_IDLE) {
                    return false;
                }

                // Read 4-byte R7 response
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);

                // ============================================================
                // Step 7: ACMD41 - SD_SEND_OP_COND
                // ============================================================
                for (retry = 0; retry < 500; ++retry) {
                    send_cmd(CMD55, 0);
                    uint8_t r1 = send_cmd(ACMD41, 0x40000000);
                    if (r1 == 0x00) {
                        break;
                    }
                    delay_ms(10);
                }
                if (retry == 500) {
                    return false;
                }

                // ============================================================
                // Step 8: CMD58 - READ_OCR
                // ============================================================
                if (send_cmd(CMD58, 0) == 0x00) {
                    uint32_t ocr = 0;
                    ocr |= (uint32_t)fast_spi_xfer(0xFF) << 24;
                    ocr |= (uint32_t)fast_spi_xfer(0xFF) << 16;
                    ocr |= (uint32_t)fast_spi_xfer(0xFF) << 8;
                    ocr |= (uint32_t)fast_spi_xfer(0xFF);
                    // CCS = bit 30 of OCR
                    m_is_sdhc = (ocr & 0x40000000) != 0;
                } else {
                    // Fallback: assume SDSC if CMD58 fails
                    m_is_sdhc = false;
                }

                // ============================================================
                // Step 9: CMD9 - SEND_CSD (get card capacity)
                // ============================================================
                get_card_capacity();

                // ============================================================
                // Step 10: CMD59 - Disable CRC
                // ============================================================
                send_cmd(CMD59, 0);

                m_mounted = true;
                m_last_alloc_cluster = 2;
                return true;
            }

            /** @brief Check if card is mounted */
            bool is_mounted() const {
                return m_mounted;
            }

            /** @brief Check if card is SDHC/SDXC (block addressing) */
            bool is_sdhc() const {
                return m_is_sdhc;
            }

            /** @brief Get total sectors from CSD */
            uint32_t total_sectors() const {
                return m_total_sectors;
            }

            /**
             * @brief Convert block address to SD card address
             * @param block  Logical block address (LBA)
             * @return Physical address for SD command
             */
            uint32_t block_to_addr(uint32_t block) const {
                return m_is_sdhc ? block : (block << 9);
            }

            /**
             * @brief Read one block (512 bytes) from SD card with retry
             * @param block   Block number (LBA address)
             * @param buffer  512-byte buffer to store data
             * @return true on success, false on failure
             */
            bool read_block(uint32_t block, uint8_t *buffer) {
                if (!m_mounted) {
                    return false;
                }

                for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
                    if (read_block_internal(block, buffer)) {
                        return true;
                    }
                    for (volatile int i = 0; i < 1000; ++i) __NOP();
                }
                return false;
            }

            /**
             * @brief Write one block (512 bytes) to SD card with retry
             * @param block   Block number (LBA address)
             * @param buffer  512-byte buffer containing data to write
             * @return true on success, false on failure
             */
            bool write_block(uint32_t block, const uint8_t *buffer) {
                if (!m_mounted) {
                    return false;
                }

                for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
                    if (write_block_internal(block, buffer)) {
                        return true;
                    }
                    for (volatile int i = 0; i < 1000; ++i) __NOP();
                }
                return false;
            }

            void unmount() {
                CS_HIGH();
                m_mounted = false;
            }

            uint32_t get_last_alloc_cluster() const {
                return m_last_alloc_cluster;
            }

            void set_last_alloc_cluster(uint32_t cluster) {
                m_last_alloc_cluster = cluster;
            }

        private:
            // Pin configuration (GPIO2)
            uint8_t  m_miso;
            uint8_t  m_mosi;
            uint8_t  m_sck;
            uint8_t  m_cs;

            uint32_t m_miso_mask;
            uint32_t m_mosi_mask;
            uint32_t m_sck_mask;
            uint32_t m_cs_mask;

            bool     m_mounted;
            uint32_t m_last_alloc_cluster;
            bool     m_is_sdhc;
            uint32_t m_total_sectors;

            // Cached register pointers
            volatile uint32_t *m_psr;
            volatile uint32_t *m_set;
            volatile uint32_t *m_clr;
            volatile uint32_t *m_tgl;
            volatile uint32_t *m_dr;
            volatile uint32_t *m_gdir;

            /**
             * @brief Read CSD register to get card capacity
             * @return true on success
             */
            bool get_card_capacity() {
                if (send_cmd(CMD9, 0) != 0x00) {
                    return false;
                }

                // Wait for data start token
                uint8_t token;
                int timeout = 10000;
                do {
                    token = fast_spi_xfer(0xFF);
                    --timeout;
                } while (token != DATA_START_TOKEN && timeout > 0);

                if (token != DATA_START_TOKEN) {
                    return false;
                }

                // Read 16-byte CSD
                uint8_t csd[16];
                fast_spi_read(csd, 16);

                // CSD version 2.0 (SDHC/SDXC)
                if ((csd[0] >> 6) == 1) {
                    // Version 2.0: C_SIZE is 22 bits (bits 69-48 of CSD)
                    uint32_t c_size = 0;
                    c_size |= ((uint32_t)(csd[7] & 0x3F)) << 16;
                    c_size |= ((uint32_t)csd[8]) << 8;
                    c_size |= ((uint32_t)csd[9]);
                    m_total_sectors = (c_size + 1) * 1024;
                } else {
                    // Version 1.0: C_SIZE is 12 bits (bits 73-62)
                    uint32_t c_size = 0;
                    c_size |= ((uint32_t)(csd[6] & 0x03)) << 10;
                    c_size |= ((uint32_t)csd[7]) << 2;
                    c_size |= ((uint32_t)(csd[8] & 0xC0)) >> 6;
                    uint32_t read_bl_len = csd[5] & 0x0F;
                    uint32_t block_len = 1 << read_bl_len;
                    uint32_t mult = ((csd[9] & 0x03) << 1) | ((csd[10] & 0x80) >> 7);
                    uint32_t block_count = (c_size + 1) << (mult + 2);
                    m_total_sectors = (block_count * block_len) / 512;
                }

                // Discard CRC
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);
                CS_HIGH();
                fast_spi_xfer(0xFF);

                return true;
            }

            // ================================================================
            // Pin Control Functions (Atomic)
            // ================================================================

            __attribute__((always_inline))
            inline void CS_LOW() {
                *m_clr = m_cs_mask;
            }

            __attribute__((always_inline))
            inline void CS_HIGH() {
                *m_set = m_cs_mask;
            }

            __attribute__((always_inline))
            inline void SCK_HIGH() {
                *m_set = m_sck_mask;
            }

            __attribute__((always_inline))
            inline void SCK_LOW() {
                *m_clr = m_sck_mask;
            }

            __attribute__((always_inline))
            inline void MOSI_HIGH() {
                *m_set = m_mosi_mask;
            }

            __attribute__((always_inline))
            inline void MOSI_LOW() {
                *m_clr = m_mosi_mask;
            }

            __attribute__((always_inline))
            inline uint32_t MISO_READ() {
                return (*m_psr & m_miso_mask) ? 1 : 0;
            }

            __attribute__((always_inline))
            inline void MOSI_WRITE(uint32_t bit) {
                if (bit) {
                    *m_set = m_mosi_mask;
                } else {
                    *m_clr = m_mosi_mask;
                }
            }

            // ================================================================
            // Fast SPI Transfer (Core Performance Function)
            // ================================================================

            /**
             * @brief Ultra-fast software SPI transfer one byte
             * @param tx  Byte to transmit
             * @return    Byte received
             *
             * @details Fully unrolled for speed. Uses DR_SET/DR_CLEAR for
             *          atomic pin control. 4 NOPs for signal settling.
             */
            __attribute__((always_inline))
            inline uint8_t fast_spi_xfer(uint8_t tx) {
                uint32_t rx = 0;

                // Bit 7 (MSB)
                if (tx & 0x80) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x80; }
                *m_clr = m_sck_mask;

                // Bit 6
                if (tx & 0x40) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x40; }
                *m_clr = m_sck_mask;

                // Bit 5
                if (tx & 0x20) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x20; }
                *m_clr = m_sck_mask;

                // Bit 4
                if (tx & 0x10) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x10; }
                *m_clr = m_sck_mask;

                // Bit 3
                if (tx & 0x08) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x08; }
                *m_clr = m_sck_mask;

                // Bit 2
                if (tx & 0x04) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x04; }
                *m_clr = m_sck_mask;

                // Bit 1
                if (tx & 0x02) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x02; }
                *m_clr = m_sck_mask;

                // Bit 0 (LSB)
                if (tx & 0x01) { *m_set = m_mosi_mask; } else { *m_clr = m_mosi_mask; }
                *m_set = m_sck_mask;
                __NOP(); __NOP(); __NOP(); __NOP();
                if (*m_psr & m_miso_mask) { rx |= 0x01; }
                *m_clr = m_sck_mask;

                return (uint8_t)rx;
            }

            __attribute__((always_inline))
            inline void fast_spi_write(const uint8_t *data, size_t len) {
                for (size_t i = 0; i < len; ++i) {
                    fast_spi_xfer(data[i]);
                }
            }

            __attribute__((always_inline))
            inline void fast_spi_read(uint8_t *data, size_t len) {
                for (size_t i = 0; i < len; ++i) {
                    data[i] = fast_spi_xfer(0xFF);
                }
            }

            // ================================================================
            // SD Card Command Interface
            // ================================================================

            /**
             * @brief Send a command to the SD card
             * @param cmd           Command code
             * @param arg           32-bit argument
             * @param keep_cs_low   If true and command succeeds, CS stays low
             * @return              R1 response byte
             */
            inline uint8_t send_cmd(uint8_t cmd, uint32_t arg, bool keep_cs_low = false) {
                uint8_t buf[6];
                buf[0] = cmd | 0x40;
                buf[1] = (arg >> 24) & 0xFF;
                buf[2] = (arg >> 16) & 0xFF;
                buf[3] = (arg >> 8) & 0xFF;
                buf[4] = (arg) & 0xFF;
                buf[5] = (cmd == CMD0) ? 0x95 : 0xFF;

                CS_LOW();
                fast_spi_xfer(0xFF);
                fast_spi_write(buf, 6);

                // Wait for R1 response (MSB = 0)
                uint8_t resp;
                int timeout = 8;
                do {
                    resp = fast_spi_xfer(0xFF);
                    --timeout;
                } while ((resp & 0x80) && timeout > 0);

                bool success = (resp == 0x00) || (resp == R1_IDLE);
                if (!keep_cs_low || !success) {
                    CS_HIGH();
                    fast_spi_xfer(0xFF);
                }

                return resp;
            }

            // ================================================================
            // Internal Read/Write
            // ================================================================

            bool read_block_internal(uint32_t block, uint8_t *buffer) {
                uint32_t addr = m_is_sdhc ? block : (block << 9);

                if (send_cmd(CMD17, addr, true) != 0x00) {
                    return false;
                }

                uint8_t token;
                int timeout = 10000;
                do {
                    token = fast_spi_xfer(0xFF);
                    --timeout;
                } while (token != DATA_START_TOKEN && timeout > 0);

                if (token != DATA_START_TOKEN) {
                    CS_HIGH();
                    return false;
                }

                fast_spi_read(buffer, BLOCK_SIZE);
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);

                CS_HIGH();
                fast_spi_xfer(0xFF);
                return true;
            }

            bool write_block_internal(uint32_t block, const uint8_t *buffer) {
                uint32_t addr = m_is_sdhc ? block : (block << 9);

                if (send_cmd(CMD24, addr, true) != 0x00) {
                    return false;
                }

                fast_spi_xfer(DATA_START_TOKEN);
                fast_spi_write(buffer, BLOCK_SIZE);
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);

                uint8_t resp = fast_spi_xfer(0xFF);
                CS_HIGH();

                if ((resp & 0x1F) != DATA_RESPONSE) {
                    return false;
                }

                int timeout = 50000;
                CS_LOW();
                do {
                    resp = fast_spi_xfer(0xFF);
                    --timeout;
                } while (resp != 0xFF && timeout > 0);
                CS_HIGH();

                return (resp == 0xFF);
            }

            // ================================================================
            // Delay Utilities
            // ================================================================

            static inline void delay_ms(uint32_t ms) {
                uint32_t cpu_freq = CLOCK_GetFreq((clock_name_t)0);
                if (cpu_freq == 0) cpu_freq = 500000000;
                volatile uint64_t count = ((uint64_t)cpu_freq / 1000) * ms / 3;
                while (count--) {
                    __NOP();
                }
            }
    };

    // ================================================================
    // FAT16/FAT32 Filesystem
    // ================================================================

    /**
     * @brief FAT16/FAT32 filesystem driver with full file operations
     *
     * @details Supports FAT16 and FAT32, 8.3 filenames only.
     *          Provides: mount, open, read, write, append, delete,
     *          rename, mkdir, rmdir, list_root, format, file_exists.
     */
    class FATFS {
        public:
            // ================================================================
            // FAT BPB Offsets
            // ================================================================
            static constexpr uint16_t BS_BYTSPERSEC = 11;
            static constexpr uint16_t BS_SECPERCLUS = 13;
            static constexpr uint16_t BS_RSVDSECCNT = 14;
            static constexpr uint16_t BS_NUMFATS    = 16;
            static constexpr uint16_t BS_ROOTENTCNT = 17;
            static constexpr uint16_t BS_TOTSEC16   = 19;
            static constexpr uint16_t BS_FATSZ16    = 22;
            static constexpr uint16_t BS_TOTSEC32   = 32;
            static constexpr uint16_t BS_FATSZ32    = 36;
            static constexpr uint16_t BS_ROOTCLUS   = 44;
            static constexpr uint16_t BS_FSINFO     = 48;
            static constexpr uint16_t BS_BKBOOTSEC  = 50;
            static constexpr uint16_t BS_VOLUME_ID  = 67;
            static constexpr uint16_t BS_VOLUME_LAB= 71;
            static constexpr uint16_t BS_FS_TYPE   = 82;

            // ================================================================
            // Directory Entry Offsets
            // ================================================================
            static constexpr uint16_t DIR_NAME      = 0;
            static constexpr uint16_t DIR_ATTR      = 11;
            static constexpr uint16_t DIR_FSTCLUSHI = 20;
            static constexpr uint16_t DIR_FSTCLUSLO = 26;
            static constexpr uint16_t DIR_FILESIZE  = 28;

            // ================================================================
            // File Attributes
            // ================================================================
            static constexpr uint8_t ATTR_READ_ONLY = 0x01;
            static constexpr uint8_t ATTR_HIDDEN    = 0x02;
            static constexpr uint8_t ATTR_SYSTEM    = 0x04;
            static constexpr uint8_t ATTR_VOLUME_ID = 0x08;
            static constexpr uint8_t ATTR_DIRECTORY = 0x10;
            static constexpr uint8_t ATTR_ARCHIVE   = 0x20;
            static constexpr uint8_t ATTR_LONG_NAME = 0x0F;

            // ================================================================
            // Cluster Constants
            // ================================================================
            static constexpr uint32_t CLUSTER_FREE = 0x00000000;

            /**
             * @brief File handle structure
             */
            struct File {
                uint32_t start_cluster;
                uint32_t current_cluster;
                uint32_t file_size;
                uint32_t position;
            };

            /**
             * @brief Directory entry information
             */
            struct DirEntry {
                char     name[13];
                uint32_t size;
                uint32_t cluster;
                uint8_t  attr;
                bool     is_directory;
            };

            /**
             * @brief Constructor
             * @param card_ref  Reference to SDCardIO instance
             */
            FATFS(SDCardIO &card_ref)
                : m_card(card_ref)
                , m_block_buf{0}
                , m_bytes_per_sec(512)
                , m_sec_per_clus(1)
                , m_fat_start(0)
                , m_fat_size(0)
                , m_root_start(0)
                , m_data_start(0)
                , m_root_cluster(0)
                , m_total_sectors(0)
                , m_is_fat32(false)
                , m_mounted(false) {}

            /**
             * @brief Mount the filesystem
             * @return true on success, false on failure
             */
            bool mount() {
                if (!m_card.is_mounted()) {
                    return false;
                }

                if (!m_card.read_block(0, m_block_buf)) {
                    return false;
                }

                // Check boot sector signature
                if (m_block_buf[510] != 0x55 || m_block_buf[511] != 0xAA) {
                    return false;
                }

                // Parse BPB
                m_bytes_per_sec = read16(m_block_buf, BS_BYTSPERSEC);
                m_sec_per_clus  = m_block_buf[BS_SECPERCLUS];
                uint16_t rsvd_sec     = read16(m_block_buf, BS_RSVDSECCNT);
                uint8_t  num_fats     = m_block_buf[BS_NUMFATS];
                uint16_t root_entries = read16(m_block_buf, BS_ROOTENTCNT);

                uint32_t fatsz16 = read16(m_block_buf, BS_FATSZ16);
                if (fatsz16 != 0) {
                    m_fat_size  = fatsz16;
                    m_total_sectors = read16(m_block_buf, BS_TOTSEC16);
                    if (m_total_sectors == 0) {
                        m_total_sectors = read32(m_block_buf, BS_TOTSEC32);
                    }
                    m_is_fat32 = false;
                } else {
                    m_fat_size  = read32(m_block_buf, BS_FATSZ32);
                    m_total_sectors = read32(m_block_buf, BS_TOTSEC32);
                    m_is_fat32 = true;
                }

                m_fat_start = rsvd_sec;
                uint32_t root_dir_sectors = ((root_entries * 32) + (m_bytes_per_sec - 1))
                                            / m_bytes_per_sec;
                m_root_start = m_fat_start + num_fats * m_fat_size;
                m_data_start = m_root_start + root_dir_sectors;

                if (m_is_fat32) {
                    m_root_cluster = read32(m_block_buf, BS_ROOTCLUS);
                }

                m_mounted = true;
                m_card.set_last_alloc_cluster(2);
                return true;
            }

            /** @brief Check if filesystem is mounted */
            bool is_mounted() const {
                return m_mounted;
            }

            /**
             * @brief Check if a file exists
             * @param name  Filename (8.3 format)
             * @return true if file exists
             */
            bool file_exists(const char *name) {
                if (!m_mounted || !name) return false;
                uint32_t sector, offset;
                return find_dir_entry(name, sector, offset, false);
            }

            /**
             * @brief Open a file
             * @param name  Filename (8.3 format)
             * @param file  File handle to fill
             * @return true on success, false if file not found
             */
            bool open(const char *name, File &file) {
                if (!m_mounted || !name) {
                    return false;
                }

                uint32_t sector, offset;
                if (!find_dir_entry(name, sector, offset, false)) {
                    return false;
                }

                uint8_t *entry = m_block_buf + offset;
                file.start_cluster = read16(entry, DIR_FSTCLUSLO);
                if (m_is_fat32) {
                    file.start_cluster |= (read16(entry, DIR_FSTCLUSHI) << 16);
                }
                file.current_cluster = file.start_cluster;
                file.file_size = read32(entry, DIR_FILESIZE);
                file.position = 0;
                return true;
            }

            /**
             * @brief Read data from an open file
             * @param file  File handle (updated)
             * @param buf   Buffer to store read data
             * @param size  Number of bytes to read
             * @return      Number of bytes actually read
             */
            uint32_t read(File &file, uint8_t *buf, uint32_t size) {
                if (!m_mounted || file.position >= file.file_size) {
                    return 0;
                }

                uint32_t remaining = file.file_size - file.position;
                if (size > remaining) {
                    size = remaining;
                }

                uint32_t read_total = 0;
                uint8_t  temp[512];
                uint32_t bytes_per_cluster = (uint32_t)m_sec_per_clus * m_bytes_per_sec;

                while (read_total < size) {
                    uint32_t cluster_offset = file.position % bytes_per_cluster;
                    uint32_t sector_offset  = cluster_offset / m_bytes_per_sec;
                    uint16_t byte_offset    = cluster_offset % m_bytes_per_sec;

                    uint32_t sector = cluster_to_sector(file.current_cluster) + sector_offset;
                    if (!m_card.read_block(sector, temp)) {
                        break;
                    }

                    uint16_t chunk = m_bytes_per_sec - byte_offset;
                    if (chunk > (size - read_total)) {
                        chunk = size - read_total;
                    }

                    memcpy(buf + read_total, temp + byte_offset, chunk);
                    read_total += chunk;
                    file.position += chunk;

                    if (file.position >= file.file_size) {
                        break;
                    }

                    // Advance to next cluster if at cluster boundary
                    if (file.position > 0 && (file.position % bytes_per_cluster) == 0) {
                        uint32_t next = next_cluster(file.current_cluster);
                        if (is_eoc(next)) {
                            break;
                        }
                        file.current_cluster = next;
                    }
                }

                return read_total;
            }

            /**
             * @brief Read entire file into vector
             * @param name  Filename (8.3 format)
             * @param data  Vector to store file data
             * @return true on success
             */
            bool read_file(const char *name, std::vector<uint8_t> &data) {
                File file;
                if (!open(name, file)) {
                    return false;
                }
                data.resize(file.file_size);
                uint32_t got = read(file, data.data(), file.file_size);
                data.resize(got);
                return got > 0;
            }

            /**
             * @brief Read entire file into string
             * @param name  Filename (8.3 format)
             * @param str   String to store file data
             * @return true on success
             */
            bool read_file(const char *name, std::string &str) {
                std::vector<uint8_t> data;
                if (!read_file(name, data)) {
                    return false;
                }
                str.assign((const char*)data.data(), data.size());
                return true;
            }

            /**
             * @brief Write data to file (creates or overwrites)
             * @param name  Filename (8.3 format)
             * @param data  Data to write
             * @param size  Number of bytes
             * @return true on success
             */
            bool write_file(const char *name, const uint8_t *data, uint32_t size) {
                if (!m_mounted || !name || !data || size == 0) {
                    return false;
                }

                // Delete existing file first
                uint32_t sector, offset;
                File existing;
                uint32_t old_cluster = 0;
                bool has_existing = false;

                if (find_dir_entry(name, sector, offset, false)) {
                    has_existing = true;
                    uint8_t *entry = m_block_buf + offset;
                    old_cluster = read16(entry, DIR_FSTCLUSLO);
                    if (m_is_fat32) {
                        old_cluster |= (read16(entry, DIR_FSTCLUSHI) << 16);
                    }
                }

                // Allocate new clusters and write data first
                uint32_t cluster = alloc_cluster();
                if (cluster == 0) {
                    return false;
                }

                uint32_t written = 0;
                uint32_t cur_cluster = cluster;
                uint8_t temp[512];

                while (written < size) {
                    uint32_t base_sector = cluster_to_sector(cur_cluster);

                    for (uint32_t sec = 0; sec < m_sec_per_clus && written < size; ++sec) {
                        memset(temp, 0, 512);
                        uint16_t chunk = m_bytes_per_sec;
                        if (chunk > (size - written)) {
                            chunk = size - written;
                        }
                        memcpy(temp, data + written, chunk);
                        if (!m_card.write_block(base_sector + sec, temp)) {
                            release_cluster_chain(cluster);
                            return false;
                        }
                        written += chunk;
                    }

                    if (written < size) {
                        uint32_t next = alloc_cluster();
                        if (next == 0) {
                            release_cluster_chain(cluster);
                            return false;
                        }
                        if (!set_cluster(cur_cluster, next)) {
                            release_cluster_chain(cluster);
                            return false;
                        }
                        cur_cluster = next;
                    }
                }

                // Release old clusters
                if (has_existing && old_cluster != 0 && !is_eoc(old_cluster)) {
                    release_cluster_chain(old_cluster);
                    m_card.set_last_alloc_cluster(2);
                }

                // Create/update directory entry
                if (!find_dir_entry(name, sector, offset, true)) {
                    release_cluster_chain(cluster);
                    return false;
                }

                if (!m_card.read_block(sector, m_block_buf)) {
                    release_cluster_chain(cluster);
                    return false;
                }

                memset(m_block_buf + offset, 0, 32);
                make_83_name(name, m_block_buf + offset + DIR_NAME);
                m_block_buf[offset + DIR_ATTR] = ATTR_ARCHIVE;
                write16(m_block_buf, offset + DIR_FSTCLUSLO, cluster & 0xFFFF);
                if (m_is_fat32) {
                    write16(m_block_buf, offset + DIR_FSTCLUSHI, (cluster >> 16) & 0xFFFF);
                }
                write32(m_block_buf, offset + DIR_FILESIZE, size);

                return m_card.write_block(sector, m_block_buf);
            }

            /**
             * @brief Write string to file
             * @param name  Filename (8.3 format)
             * @param str   String to write
             * @return true on success
             */
            bool write_file(const char *name, const std::string &str) {
                return write_file(name, (const uint8_t*)str.c_str(), str.size());
            }

            /**
             * @brief Append data to existing file
             * @param name  Filename (8.3 format)
             * @param data  Data to append
             * @param size  Number of bytes
             * @return true on success
             */
            bool append_file(const char *name, const uint8_t *data, uint32_t size) {
                if (!m_mounted || !name || !data || size == 0) {
                    return false;
                }

                uint32_t sector, offset;
                if (!find_dir_entry(name, sector, offset, false)) {
                    return write_file(name, data, size);
                }

                uint8_t *entry = m_block_buf + offset;
                uint32_t start_cluster = read16(entry, DIR_FSTCLUSLO);
                if (m_is_fat32) {
                    start_cluster |= (read16(entry, DIR_FSTCLUSHI) << 16);
                }
                uint32_t file_size = read32(entry, DIR_FILESIZE);

                // Find last cluster
                uint32_t cur_cluster = start_cluster;
                uint32_t next;
                uint32_t traverse_count = 0;
                while (true) {
                    next = next_cluster(cur_cluster);
                    if (is_eoc(next)) {
                        break;
                    }
                    cur_cluster = next;
                    if (++traverse_count > MAX_CLUSTER_TRAVERSE) {
                        return false;
                    }
                }

                // Handle empty file
                if (file_size == 0) {
                    if (cur_cluster == 0 || is_eoc(cur_cluster)) {
                        cur_cluster = alloc_cluster();
                        if (cur_cluster == 0) {
                            return false;
                        }
                        if (!m_card.read_block(sector, m_block_buf)) {
                            return false;
                        }
                        uint8_t *dir_entry = m_block_buf + offset;
                        write16(dir_entry, DIR_FSTCLUSLO, cur_cluster & 0xFFFF);
                        if (m_is_fat32) {
                            write16(dir_entry, DIR_FSTCLUSHI, (cur_cluster >> 16) & 0xFFFF);
                        }
                        if (!m_card.write_block(sector, m_block_buf)) {
                            return false;
                        }
                    }
                }

                uint32_t written = 0;
                uint32_t position = file_size;
                uint8_t temp[512];
                uint32_t bytes_per_cluster = (uint32_t)m_sec_per_clus * m_bytes_per_sec;

                while (written < size) {
                    if (position > 0 && (position % bytes_per_cluster) == 0) {
                        uint32_t new_cluster = alloc_cluster();
                        if (new_cluster == 0) {
                            return false;
                        }
                        if (!set_cluster(cur_cluster, new_cluster)) {
                            return false;
                        }
                        cur_cluster = new_cluster;
                    }

                    uint32_t cluster_offset = position % bytes_per_cluster;
                    uint32_t sector_offset  = cluster_offset / m_bytes_per_sec;
                    uint16_t byte_offset    = cluster_offset % m_bytes_per_sec;
                    uint32_t write_sector = cluster_to_sector(cur_cluster) + sector_offset;

                    if (byte_offset != 0) {
                        if (!m_card.read_block(write_sector, temp)) {
                            return false;
                        }
                    } else {
                        memset(temp, 0, 512);
                    }

                    uint16_t chunk = m_bytes_per_sec - byte_offset;
                    if (chunk > (size - written)) {
                        chunk = size - written;
                    }
                    memcpy(temp + byte_offset, data + written, chunk);
                    if (!m_card.write_block(write_sector, temp)) {
                        return false;
                    }

                    written += chunk;
                    position += chunk;
                }

                // Update file size
                if (!m_card.read_block(sector, m_block_buf)) {
                    return false;
                }
                write32(m_block_buf, offset + DIR_FILESIZE, position);
                return m_card.write_block(sector, m_block_buf);
            }

            /**
             * @brief Append string to existing file
             * @param name  Filename (8.3 format)
             * @param str   String to append
             * @return true on success
             */
            bool append_file(const char *name, const std::string &str) {
                return append_file(name, (const uint8_t*)str.c_str(), str.size());
            }

            /**
             * @brief Delete a file
             * @param name  Filename (8.3 format)
             * @return true on success
             */
            bool delete_file(const char *name) {
                if (!m_mounted || !name) {
                    return false;
                }

                uint32_t sector, offset;
                if (!find_dir_entry(name, sector, offset, false)) {
                    return false;
                }

                uint8_t *entry = m_block_buf + offset;
                uint32_t cluster = read16(entry, DIR_FSTCLUSLO);
                if (m_is_fat32) {
                    cluster |= (read16(entry, DIR_FSTCLUSHI) << 16);
                }

                // Clean up LFN entries
                uint16_t lfn_off = offset;
                while (lfn_off >= 32) {
                    lfn_off -= 32;
                    uint8_t first = m_block_buf[lfn_off];
                    if (first == 0x00 || first == 0xE5) {
                        break;
                    }
                    uint8_t attr = m_block_buf[lfn_off + DIR_ATTR];
                    if (attr == ATTR_LONG_NAME) {
                        m_block_buf[lfn_off] = 0xE5;
                    } else {
                        break;
                    }
                }

                // Release cluster chain
                release_cluster_chain(cluster);
                m_card.set_last_alloc_cluster(2);

                // Mark directory entry as deleted
                m_block_buf[offset] = 0xE5;
                return m_card.write_block(sector, m_block_buf);
            }

            /**
             * @brief Rename a file
             * @param old_name  Current filename (8.3 format)
             * @param new_name  New filename (8.3 format)
             * @return true on success
             */
            bool rename_file(const char *old_name, const char *new_name) {
                if (!m_mounted || !old_name || !new_name) {
                    return false;
                }

                // Check if new name already exists
                if (file_exists(new_name)) {
                    return false;
                }

                uint32_t old_sector = 0, old_offset = 0;
                if (!find_dir_entry(old_name, old_sector, old_offset, false)) {
                    return false;
                }

                // Save old entry data
                if (!m_card.read_block(old_sector, m_block_buf)) {
                    return false;
                }
                uint8_t old_entry[32];
                memcpy(old_entry, m_block_buf + old_offset, 32);

                // Find or create new entry slot
                uint32_t new_sector = 0, new_offset = 0;
                if (!find_dir_entry(new_name, new_sector, new_offset, true)) {
                    return false;
                }

                // Read new sector
                if (!m_card.read_block(new_sector, m_block_buf)) {
                    return false;
                }

                // Write new entry
                memset(m_block_buf + new_offset, 0, 32);
                make_83_name(new_name, m_block_buf + new_offset + DIR_NAME);
                m_block_buf[new_offset + DIR_ATTR] = old_entry[DIR_ATTR];
                memcpy(m_block_buf + new_offset + DIR_FSTCLUSLO,
                       old_entry + DIR_FSTCLUSLO, 4);
                if (m_is_fat32) {
                    memcpy(m_block_buf + new_offset + DIR_FSTCLUSHI,
                           old_entry + DIR_FSTCLUSHI, 2);
                }
                write32(m_block_buf, new_offset + DIR_FILESIZE,
                        read32(old_entry, DIR_FILESIZE));

                if (!m_card.write_block(new_sector, m_block_buf)) {
                    return false;
                }

                // Read old sector again and delete old entry
                if (!m_card.read_block(old_sector, m_block_buf)) {
                    return false;
                }
                m_block_buf[old_offset] = 0xE5;
                return m_card.write_block(old_sector, m_block_buf);
            }

            /**
             * @brief Create a directory
             * @param name  Directory name (8.3 format)
             * @return true on success
             */
            bool mkdir(const char *name) {
                if (!m_mounted || !name) {
                    return false;
                }

                if (file_exists(name)) {
                    return false;
                }

                uint32_t cluster = alloc_cluster();
                if (cluster == 0) {
                    return false;
                }

                uint32_t base_sector = cluster_to_sector(cluster);
                uint32_t bytes_per_cluster = (uint32_t)m_sec_per_clus * m_bytes_per_sec;

                // Zero first sector
                memset(m_block_buf, 0, m_bytes_per_sec);

                // First entry: "." (current directory)
                memset(m_block_buf, ' ', 11);
                m_block_buf[0] = '.';
                m_block_buf[DIR_ATTR] = ATTR_DIRECTORY;
                write16(m_block_buf, DIR_FSTCLUSLO, cluster & 0xFFFF);
                if (m_is_fat32) {
                    write16(m_block_buf, DIR_FSTCLUSHI, (cluster >> 16) & 0xFFFF);
                }

                // Second entry: ".." (parent directory)
                memset(m_block_buf + 32, ' ', 11);
                m_block_buf[32] = '.';
                m_block_buf[33] = '.';
                m_block_buf[32 + DIR_ATTR] = ATTR_DIRECTORY;
                write16(m_block_buf, 32 + DIR_FSTCLUSLO,
                        m_is_fat32 ? m_root_cluster : 0);
                if (m_is_fat32) {
                    write16(m_block_buf, 32 + DIR_FSTCLUSHI,
                            (m_root_cluster >> 16) & 0xFFFF);
                }

                // Write first sector
                if (!m_card.write_block(base_sector, m_block_buf)) {
                    release_cluster_chain(cluster);
                    return false;
                }

                // Clear remaining sectors in cluster
                memset(m_block_buf, 0, m_bytes_per_sec);
                for (uint32_t sec = 1; sec < m_sec_per_clus; ++sec) {
                    if (!m_card.write_block(base_sector + sec, m_block_buf)) {
                        release_cluster_chain(cluster);
                        return false;
                    }
                }

                // Create directory entry in root
                uint32_t dir_sector = 0, dir_offset = 0;
                if (!find_dir_entry(name, dir_sector, dir_offset, true)) {
                    release_cluster_chain(cluster);
                    return false;
                }

                if (!m_card.read_block(dir_sector, m_block_buf)) {
                    release_cluster_chain(cluster);
                    return false;
                }

                memset(m_block_buf + dir_offset, 0, 32);
                make_83_name(name, m_block_buf + dir_offset + DIR_NAME);
                m_block_buf[dir_offset + DIR_ATTR] = ATTR_DIRECTORY;
                write16(m_block_buf, dir_offset + DIR_FSTCLUSLO, cluster & 0xFFFF);
                if (m_is_fat32) {
                    write16(m_block_buf, dir_offset + DIR_FSTCLUSHI, (cluster >> 16) & 0xFFFF);
                }
                write32(m_block_buf, dir_offset + DIR_FILESIZE, 0);

                return m_card.write_block(dir_sector, m_block_buf);
            }

            /**
             * @brief Delete an empty directory
             * @param name  Directory name (8.3 format)
             * @return true on success
             */
            bool rmdir(const char *name) {
                if (!m_mounted || !name) {
                    return false;
                }

                if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
                    return false;
                }

                uint32_t sector, offset;
                if (!find_dir_entry(name, sector, offset, false)) {
                    return false;
                }

                uint8_t *entry = m_block_buf + offset;
                if ((entry[DIR_ATTR] & ATTR_DIRECTORY) == 0) {
                    return false;
                }

                uint32_t cluster = read16(entry, DIR_FSTCLUSLO);
                if (m_is_fat32) {
                    cluster |= (read16(entry, DIR_FSTCLUSHI) << 16);
                }

                // Check if directory is empty
                if (!is_directory_empty(cluster)) {
                    return false;
                }

                release_cluster_chain(cluster);
                m_card.set_last_alloc_cluster(2);

                // Delete directory entry
                if (!m_card.read_block(sector, m_block_buf)) {
                    return false;
                }
                m_block_buf[offset] = 0xE5;
                return m_card.write_block(sector, m_block_buf);
            }

            /**
             * @brief Check if a directory is empty (only "." and "..")
             * @param cluster  Directory cluster
             * @return true if empty
             */
            bool is_directory_empty(uint32_t cluster) {
                uint32_t base_sector = cluster_to_sector(cluster);

                for (uint32_t sec = 0; sec < m_sec_per_clus; ++sec) {
                    if (!m_card.read_block(base_sector + sec, m_block_buf)) {
                        return false;
                    }

                    uint16_t start_off = (sec == 0) ? 64 : 0;  // Skip "." and ".." in first sector
                    for (uint16_t off = start_off; off < m_bytes_per_sec; off += 32) {
                        uint8_t first = m_block_buf[off];
                        if (first != 0x00 && first != 0xE5) {
                            return false;  // Non-empty
                        }
                    }
                }

                return true;
            }

            /**
             * @brief List root directory entries
             * @param entries  Vector to store directory entries
             * @return Number of entries found
             */
            size_t list_root(std::vector<DirEntry> &entries) {
                entries.clear();
                if (!m_mounted) {
                    return 0;
                }

                uint32_t cluster = m_is_fat32 ? m_root_cluster : 0;
                uint32_t traverse_count = 0;

                while (true) {
                    uint32_t base_sector = m_is_fat32 ? cluster_to_sector(cluster) : m_root_start;

                    for (uint32_t sec = 0; sec < m_sec_per_clus; ++sec) {
                        if (!m_card.read_block(base_sector + sec, m_block_buf)) {
                            return entries.size();
                        }

                        for (uint16_t off = 0; off < m_bytes_per_sec; off += 32) {
                            uint8_t first = m_block_buf[off];
                            if (first == 0x00) {
                                return entries.size();
                            }
                            if (first == 0xE5) {
                                continue;
                            }

                            uint8_t attr = m_block_buf[off + DIR_ATTR];
                            if (attr == ATTR_LONG_NAME) {
                                continue;
                            }

                            DirEntry entry;
                            int pos = 0;
                            for (int i = 0; i < 8 && m_block_buf[off + i] != ' '; ++i) {
                                entry.name[pos++] = m_block_buf[off + i];
                            }
                            if (m_block_buf[off + 8] != ' ') {
                                entry.name[pos++] = '.';
                                for (int i = 8; i < 11 && m_block_buf[off + i] != ' '; ++i) {
                                    entry.name[pos++] = m_block_buf[off + i];
                                }
                            }
                            entry.name[pos] = '\0';

                            entry.size = read32(m_block_buf, off + DIR_FILESIZE);
                            entry.attr = attr;
                            entry.is_directory = (attr & ATTR_DIRECTORY) != 0;
                            entry.cluster = read16(m_block_buf, off + DIR_FSTCLUSLO);
                            if (m_is_fat32) {
                                entry.cluster |= (read16(m_block_buf, off + DIR_FSTCLUSHI) << 16);
                            }

                            entries.push_back(entry);
                        }
                    }

                    if (!m_is_fat32) {
                        break;
                    }
                    cluster = next_cluster(cluster);
                    if (is_eoc(cluster) || ++traverse_count > MAX_CLUSTER_TRAVERSE) {
                        break;
                    }
                }

                return entries.size();
            }

            /**
             * @brief Format SD card as FAT32/FAT16
             * @param label  Volume label (11 characters, optional)
             * @return true on success
             * @warning THIS WILL ERASE ALL DATA ON THE CARD!
             */
            bool format(const char *label = nullptr) {
                if (!m_card.is_mounted()) {
                    return false;
                }

                // Unmount first
                m_mounted = false;

                uint32_t total_sectors = m_card.total_sectors();
                if (total_sectors == 0) {
                    total_sectors = 2000000;  // Fallback for unknown cards
                }

                // Use FAT32 for cards > 65536 sectors (~32MB)
                bool use_fat32 = (total_sectors > 65536);
                // Don't set m_is_fat32 here - will be set after successful format

                uint32_t sec_per_clus = 8;
                if (use_fat32) {
                    if (total_sectors > 0x800000) sec_per_clus = 16;
                    if (total_sectors > 0x1000000) sec_per_clus = 32;
                    if (total_sectors > 0x2000000) sec_per_clus = 64;
                } else {
                    if (total_sectors > 0x10000) sec_per_clus = 16;
                    else if (total_sectors > 0x8000) sec_per_clus = 8;
                    else sec_per_clus = 4;
                }

                uint32_t rsvd_sec = use_fat32 ? 32 : 1;
                uint32_t num_fats = 2;
                uint32_t root_entries = use_fat32 ? 0 : 512;
                uint32_t fatsz = 0;

                if (use_fat32) {
                    // FAT32: calculate FAT size from cluster count
                    uint32_t data_clusters = (total_sectors - rsvd_sec) / sec_per_clus;
                    fatsz = (data_clusters * 4 + 511) / 512 + 1;
                    if (fatsz < 32) fatsz = 32;
                } else {
                    // FAT16: calculate FAT size
                    uint32_t root_sectors = (root_entries * 32 + 511) / 512;
                    uint32_t data_sectors = total_sectors - rsvd_sec - num_fats * fatsz - root_sectors;
                    uint32_t data_clusters = data_sectors / sec_per_clus;
                    fatsz = (data_clusters * 2 + 511) / 512 + 1;
                    if (fatsz < 1) fatsz = 1;
                }

                // Build boot sector
                memset(m_block_buf, 0, 512);
                m_block_buf[0] = 0xEB;
                m_block_buf[1] = 0x58;
                m_block_buf[2] = 0x90;

                const char *oem = "MSWIN4.1";
                memcpy(m_block_buf + 3, oem, 8);

                write16(m_block_buf, BS_BYTSPERSEC, 512);
                m_block_buf[BS_SECPERCLUS] = sec_per_clus;
                write16(m_block_buf, BS_RSVDSECCNT, rsvd_sec);
                m_block_buf[BS_NUMFATS] = num_fats;
                write16(m_block_buf, BS_ROOTENTCNT, root_entries);
                write16(m_block_buf, BS_TOTSEC16, (total_sectors < 0xFFFF) ? total_sectors : 0);
                m_block_buf[21] = 0xF8;

                if (use_fat32) {
                    write16(m_block_buf, BS_FATSZ16, 0);
                    write32(m_block_buf, BS_TOTSEC32, total_sectors);
                    write32(m_block_buf, BS_FATSZ32, fatsz);
                    write32(m_block_buf, BS_ROOTCLUS, 2);
                    write16(m_block_buf, BS_FSINFO, 1);
                    write16(m_block_buf, BS_BKBOOTSEC, 6);
                } else {
                    write16(m_block_buf, BS_FATSZ16, fatsz);
                    write16(m_block_buf, BS_TOTSEC16, total_sectors);
                }

                // Volume label and FS type
                if (label) {
                    for (int i = 0; i < 11 && label[i]; ++i) {
                        m_block_buf[BS_VOLUME_LAB + i] = toupper(label[i]);
                    }
                }
                const char *fs_type = use_fat32 ? "FAT32   " : "FAT16   ";
                memcpy(m_block_buf + BS_FS_TYPE, fs_type, 8);

                m_block_buf[510] = 0x55;
                m_block_buf[511] = 0xAA;

                // Write boot sector
                if (!m_card.write_block(0, m_block_buf)) {
                    m_mounted = false;
                    return false;
                }

                // Build and write FSInfo sector (FAT32 only)
                if (use_fat32) {
                    // Write backup boot sector
                    if (!m_card.write_block(6, m_block_buf)) {
                        m_mounted = false;
                        return false;
                    }

                    // Build FSInfo sector
                    memset(m_block_buf, 0, 512);
                    m_block_buf[0] = 0x52;
                    m_block_buf[1] = 0x52;
                    m_block_buf[2] = 0x61;
                    m_block_buf[3] = 0x41;
                    write32(m_block_buf, 484, 0xFFFFFFFF);  // Free cluster count
                    write32(m_block_buf, 488, 2);           // Next free cluster
                    m_block_buf[510] = 0x55;
                    m_block_buf[511] = 0xAA;

                    if (!m_card.write_block(1, m_block_buf)) {
                        m_mounted = false;
                        return false;
                    }
                }

                // Initialize FATs
                memset(m_block_buf, 0, 512);
                if (use_fat32) {
                    write32(m_block_buf, 0, 0x0FFFFFF8);
                    write32(m_block_buf, 4, 0x0FFFFFFF);
                    write32(m_block_buf, 8, 0x0FFFFFFF);  // Root cluster
                } else {
                    write16(m_block_buf, 0, 0xFFF8);
                    write16(m_block_buf, 2, 0xFFFF);
                }

                uint32_t fat_sector = rsvd_sec;
                for (int fat = 0; fat < num_fats; ++fat) {
                    for (uint32_t sec = 0; sec < fatsz; ++sec) {
                        if (sec == 0) {
                            if (!m_card.write_block(fat_sector + sec, m_block_buf)) {
                                m_mounted = false;
                                return false;
                            }
                        } else {
                            memset(m_block_buf, 0, 512);
                            if (!m_card.write_block(fat_sector + sec, m_block_buf)) {
                                m_mounted = false;
                                return false;
                            }
                        }
                    }
                    fat_sector += fatsz;
                }

                // Initialize root directory
                uint32_t root_sector = rsvd_sec + num_fats * fatsz;
                memset(m_block_buf, 0, 512);

                if (use_fat32) {
                    // Root directory is cluster 2
                    root_sector += (2 - 2) * sec_per_clus;
                    for (uint32_t sec = 0; sec < sec_per_clus; ++sec) {
                        if (!m_card.write_block(root_sector + sec, m_block_buf)) {
                            m_mounted = false;
                            return false;
                        }
                    }
                } else {
                    uint32_t root_sectors = (root_entries * 32 + 511) / 512;
                    for (uint32_t sec = 0; sec < root_sectors; ++sec) {
                        if (!m_card.write_block(root_sector + sec, m_block_buf)) {
                            m_mounted = false;
                            return false;
                        }
                    }
                }

                // Update state after successful format
                m_is_fat32 = use_fat32;
                m_fat_start = rsvd_sec;
                m_fat_size = fatsz;
                m_data_start = rsvd_sec + num_fats * fatsz +
                               (use_fat32 ? 0 : (root_entries * 32 + 511) / 512);
                m_root_cluster = use_fat32 ? 2 : 0;
                m_total_sectors = total_sectors;
                m_mounted = true;

                return true;
            }

            void unmount() {
                m_mounted = false;
            }

        private:
            // ================================================================
            // Helper Functions
            // ================================================================

            inline uint32_t eoc_value() const {
                return m_is_fat32 ? 0x0FFFFFF8 : 0xFFFF;
            }

            inline bool is_eoc(uint32_t cluster) const {
                if (m_is_fat32) {
                    return cluster >= 0x0FFFFFF8;
                } else {
                    return cluster >= 0xFFF8;
                }
            }

            void release_cluster_chain(uint32_t cluster) {
                uint32_t traverse_count = 0;
                while (!is_eoc(cluster)) {
                    uint32_t next = next_cluster(cluster);
                    set_cluster(cluster, CLUSTER_FREE);
                    if (next == 0 || is_eoc(next) || ++traverse_count > MAX_CLUSTER_TRAVERSE) {
                        break;
                    }
                    cluster = next;
                }
            }

            bool find_dir_entry(const char *name, uint32_t &sector_out,
                                uint16_t &offset_out, bool create_if_not_found) {
                uint32_t cluster = m_is_fat32 ? m_root_cluster : 0;
                uint32_t traverse_count = 0;

                while (true) {
                    uint32_t base_sector = m_is_fat32 ? cluster_to_sector(cluster) : m_root_start;

                    for (uint32_t sec = 0; sec < m_sec_per_clus; ++sec) {
                        if (!m_card.read_block(base_sector + sec, m_block_buf)) {
                            return false;
                        }

                        for (uint16_t off = 0; off < m_bytes_per_sec; off += 32) {
                            uint8_t first = m_block_buf[off];

                            if (first == 0x00) {
                                if (create_if_not_found) {
                                    sector_out = base_sector + sec;
                                    offset_out = off;
                                    return true;
                                }
                                return false;
                            }

                            if (first == 0xE5) {
                                if (create_if_not_found) {
                                    sector_out = base_sector + sec;
                                    offset_out = off;
                                    return true;
                                }
                                continue;
                            }

                            if (m_block_buf[off + DIR_ATTR] == ATTR_LONG_NAME) {
                                continue;
                            }

                            if (name_match(m_block_buf + off, name)) {
                                sector_out = base_sector + sec;
                                offset_out = off;
                                return true;
                            }
                        }
                    }

                    if (!m_is_fat32) {
                        break;
                    }
                    cluster = next_cluster(cluster);
                    if (is_eoc(cluster) || ++traverse_count > MAX_CLUSTER_TRAVERSE) {
                        break;
                    }
                }

                return false;
            }

            static inline uint16_t read16(uint8_t *buf, uint16_t off) {
                return (uint16_t)buf[off] | ((uint16_t)buf[off + 1] << 8);
            }

            static inline uint32_t read32(uint8_t *buf, uint16_t off) {
                return (uint32_t)buf[off] | ((uint32_t)buf[off + 1] << 8) |
                       ((uint32_t)buf[off + 2] << 16) | ((uint32_t)buf[off + 3] << 24);
            }

            static inline void write16(uint8_t *buf, uint16_t off, uint16_t val) {
                buf[off]     = val & 0xFF;
                buf[off + 1] = (val >> 8) & 0xFF;
            }

            static inline void write32(uint8_t *buf, uint16_t off, uint32_t val) {
                buf[off]     = val & 0xFF;
                buf[off + 1] = (val >> 8) & 0xFF;
                buf[off + 2] = (val >> 16) & 0xFF;
                buf[off + 3] = (val >> 24) & 0xFF;
            }

            inline uint32_t cluster_to_sector(uint32_t cluster) {
                return m_data_start + (cluster - 2) * m_sec_per_clus;
            }

            inline uint32_t next_cluster(uint32_t cluster) {
                uint32_t fat_offset = m_is_fat32 ? (cluster * 4) : (cluster * 2);
                uint32_t fat_sector = m_fat_start + (fat_offset / m_bytes_per_sec);
                uint16_t fat_index  = fat_offset % m_bytes_per_sec;

                if (!m_card.read_block(fat_sector, m_block_buf)) {
                    return eoc_value();
                }

                if (m_is_fat32) {
                    uint32_t val = read32(m_block_buf, fat_index) & 0x0FFFFFFF;
                    if (is_eoc(val)) return eoc_value();
                    return val;
                } else {
                    uint16_t val = read16(m_block_buf, fat_index);
                    if (is_eoc(val)) return eoc_value();
                    return val;
                }
            }

            inline bool set_cluster(uint32_t cluster, uint32_t value) {
                uint32_t fat_offset = m_is_fat32 ? (cluster * 4) : (cluster * 2);
                uint32_t fat_sector = m_fat_start + (fat_offset / m_bytes_per_sec);
                uint16_t fat_index  = fat_offset % m_bytes_per_sec;

                if (!m_card.read_block(fat_sector, m_block_buf)) {
                    return false;
                }

                if (m_is_fat32) {
                    uint32_t old = read32(m_block_buf, fat_index);
                    value = (value & 0x0FFFFFFF) | (old & 0xF0000000);
                    write32(m_block_buf, fat_index, value);
                } else {
                    write16(m_block_buf, fat_index, (uint16_t)value);
                }

                if (!m_card.write_block(fat_sector, m_block_buf)) {
                    return false;
                }

                uint32_t fat2_sector = fat_sector + m_fat_size;
                if (!m_card.write_block(fat2_sector, m_block_buf)) {
                    return false;
                }

                return true;
            }

            inline uint32_t alloc_cluster() {
                uint32_t max_cluster = m_total_sectors / m_sec_per_clus;
                uint32_t start = m_card.get_last_alloc_cluster();
                if (start < 2) start = 2;

                for (uint32_t cluster = start; cluster < max_cluster; ++cluster) {
                    if (next_cluster(cluster) == CLUSTER_FREE) {
                        if (set_cluster(cluster, eoc_value())) {
                            if (cluster + 1 >= max_cluster) {
                                m_card.set_last_alloc_cluster(2);
                            } else {
                                m_card.set_last_alloc_cluster(cluster + 1);
                            }
                            return cluster;
                        }
                    }
                }

                for (uint32_t cluster = 2; cluster < start; ++cluster) {
                    if (next_cluster(cluster) == CLUSTER_FREE) {
                        if (set_cluster(cluster, eoc_value())) {
                            if (cluster + 1 >= max_cluster) {
                                m_card.set_last_alloc_cluster(2);
                            } else {
                                m_card.set_last_alloc_cluster(cluster + 1);
                            }
                            return cluster;
                        }
                    }
                }

                return 0;
            }

            static bool name_match(const uint8_t *entry, const char *name) {
                if (!entry || !name) return false;

                char fname[13] = {0};
                int pos = 0;

                for (int i = 0; i < 8 && entry[i] != ' '; ++i) {
                    fname[pos++] = entry[i];
                }
                if (entry[8] != ' ') {
                    fname[pos++] = '.';
                    for (int i = 8; i < 11 && entry[i] != ' '; ++i) {
                        fname[pos++] = entry[i];
                    }
                }

                for (int i = 0; i < 13; ++i) {
                    char a = fname[i];
                    char b = name[i];

                    if (a == 0 && b == 0) return true;
                    if (a == 0 || b == 0) return false;

                    if (a >= 'A' && a <= 'Z') a += 32;
                    if (b >= 'A' && b <= 'Z') b += 32;

                    if (a != b) return false;
                }

                return true;
            }

            static void make_83_name(const char *name, uint8_t *out) {
                memset(out, ' ', 11);

                const char *dot = strchr(name, '.');
                int name_len = dot ? (int)(dot - name) : (int)strlen(name);
                if (name_len > 8) name_len = 8;

                for (int i = 0; i < name_len; ++i) {
                    char c = name[i];
                    if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                        out[i] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
                    } else if ((c >= '0' && c <= '9') || c == '_' || c == '-') {
                        out[i] = c;
                    } else {
                        out[i] = '_';
                    }
                }

                if (dot && dot[1]) {
                    int ext_len = strlen(dot + 1);
                    if (ext_len > 3) ext_len = 3;
                    for (int i = 0; i < ext_len; ++i) {
                        char c = dot[1 + i];
                        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')) {
                            out[8 + i] = (c >= 'a' && c <= 'z') ? (c - 32) : c;
                        } else if ((c >= '0' && c <= '9') || c == '_' || c == '-') {
                            out[8 + i] = c;
                        } else {
                            out[8 + i] = '_';
                        }
                    }
                }
            }

            SDCardIO &m_card;

            uint8_t   m_block_buf[512];
            uint16_t  m_bytes_per_sec;
            uint8_t   m_sec_per_clus;
            uint32_t  m_fat_start;
            uint32_t  m_fat_size;
            uint32_t  m_root_start;
            uint32_t  m_data_start;
            uint32_t  m_root_cluster;
            uint32_t  m_total_sectors;
            bool      m_is_fat32;
            bool      m_mounted;
    };

} // namespace SDCard

#endif // SDCARD_HPP
