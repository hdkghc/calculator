/** @file /inc/sdcard.hpp
 *  @brief Ultra-fast software SPI SD card interface with FAT16/FAT32 support
 *  @author hdkghc
 *  @version 0.1
 *
 *  @details This driver uses direct register manipulation for maximum performance.
 *           The SPI bit-banging loop is hand-optimized. All GPIO operations
 *           bypass HAL for speed.
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

#include "fsl_common.h"
#include "fsl_clock.h"
#include "fsl_device_registers.h"
#include "fsl_iomuxc.h"

namespace SDCard {

    /**
     * @brief SD card SPI command codes
     */
    enum SDCommand : uint8_t {
        CMD0    = 0x00,     /**< GO_IDLE_STATE - Reset card to idle */
        CMD8    = 0x08,     /**< SEND_IF_COND - Check voltage range */
        CMD17   = 0x11,     /**< READ_SINGLE_BLOCK - Read one block */
        CMD24   = 0x18,     /**< WRITE_BLOCK - Write one block */
        CMD55   = 0x37,     /**< APP_CMD - Prefix for ACMD */
        ACMD41  = 0x29,     /**< SD_SEND_OP_COND - Initialize card */
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

    // ================================================================
    // GPIO2 Register Definitions (cached for speed)
    // ================================================================
    // GPIO2 Base Address: 0x4200_0000 (per RM Table 12-1)
    // Register offsets:
    //   0x00: DR (Data Register) - Read/write current pin states
    //   0x04: GDIR (Direction Register) - Configure input/output
    //   0x08: PSR (Pad Status Register) - Read actual pin states
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
    // GPIO_SD_00 -> GPIO2_IO00 (MOSI)
    // GPIO_SD_05 -> GPIO2_IO05 (CS)
    // GPIO_SD_12 -> GPIO2_IO12 (MISO)
    // GPIO_SD_13 -> GPIO2_IO13 (SCK)
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
     *          IOMUXC is configured in init() for GPIO mode.
     *
     *          Pin map (GPIO2):
     *          - Bit 0:  MOSI (Master Out Slave In)
     *          - Bit 5:  CS   (Chip Select, active low)
     *          - Bit 12: MISO (Master In Slave Out)
     *          - Bit 13: SCK  (Serial Clock)
     */
    class SDCardIO {
        public:
            static constexpr int BLOCK_SIZE = 512;
            static constexpr int MAX_RETRIES = 3;

            SDCardIO(uint8_t miso = 12, uint8_t mosi = 0,
                     uint8_t sck = 13, uint8_t cs = 5)
                : m_miso(miso), m_mosi(mosi), m_sck(sck), m_cs(cs),
                  m_miso_mask(1UL << miso), m_mosi_mask(1UL << mosi),
                  m_sck_mask(1UL << sck), m_cs_mask(1UL << cs),
                  m_mounted(false), m_last_alloc_cluster(2) {
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
                // Per RM: "The I/O multiplexer must be configured to GPIO mode
                // for the GPIO_DR value to connect with the signal."
                // Also: "The IOMUXC must be configured to GPIO mode for
                // GPIO_PSR to reflect the state of the corresponding signal."
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
                // GDIR: bit = 1 -> Output, bit = 0 -> Input
                // MOSI, SCK, CS are outputs; MISO is input (default 0)
                // ============================================================
                *m_gdir |= (m_mosi_mask | m_sck_mask | m_cs_mask);

                // ============================================================
                // Step 3: Set initial pin states using DR_SET/CLEAR (atomic)
                // ============================================================
                // CS=HIGH (de-asserted), MOSI=HIGH (idle), SCK=LOW (idle)
                // ============================================================
                *m_set = (m_cs_mask | m_mosi_mask | m_sck_mask);
                *m_clr = m_sck_mask;

                // ============================================================
                // Step 4: Send 80 clock pulses to wake up SD card
                // ============================================================
                // SD card needs 74+ clocks after power-up to enter SPI mode
                // ============================================================
                for (int i = 0; i < 10; ++i) {
                    fast_spi_xfer(0xFF);
                }

                // ============================================================
                // Step 5: CMD0 - GO_IDLE_STATE
                // ============================================================
                // R1 response should be 0x01 (card in idle state)
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
                // Check voltage range (2.7-3.6V) and pattern (0x1AA)
                // ============================================================
                if (send_cmd(CMD8, 0x1AA) != R1_IDLE) {
                    return false;
                }

                // Read 4-byte R7 response and discard
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);
                fast_spi_xfer(0xFF);

                // ============================================================
                // Step 7: ACMD41 - SD_SEND_OP_COND
                // ============================================================
                // CMD55 (APP_CMD) then ACMD41 with HCS=1 (High Capacity support)
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
                // Step 8: CMD59 - Disable CRC
                // ============================================================
                // Saves 2 bytes per block transfer
                // ============================================================
                send_cmd(CMD59, 0);

                m_mounted = true;
                m_last_alloc_cluster = 2;
                return true;
            }

            bool is_mounted() const {
                return m_mounted;
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
                    // Small delay before retry (~2-4us)
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
                    // Small delay before retry (~2-4us)
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

            // Cached register pointers (for performance)
            volatile uint32_t *m_psr;
            volatile uint32_t *m_set;
            volatile uint32_t *m_clr;
            volatile uint32_t *m_tgl;
            volatile uint32_t *m_dr;
            volatile uint32_t *m_gdir;

            /** @brief Pull CS low using DR_CLEAR (atomic) */
            __attribute__((always_inline))
            inline void CS_LOW() {
                *m_clr = m_cs_mask;
            }

            /** @brief Pull CS high using DR_SET (atomic) */
            __attribute__((always_inline))
            inline void CS_HIGH() {
                *m_set = m_cs_mask;
            }

            /** @brief Set SCK high using DR_SET (atomic) */
            __attribute__((always_inline))
            inline void SCK_HIGH() {
                *m_set = m_sck_mask;
            }

            /** @brief Set SCK low using DR_CLEAR (atomic) */
            __attribute__((always_inline))
            inline void SCK_LOW() {
                *m_clr = m_sck_mask;
            }

            /** @brief Set MOSI high using DR_SET (atomic) */
            __attribute__((always_inline))
            inline void MOSI_HIGH() {
                *m_set = m_mosi_mask;
            }

            /** @brief Set MOSI low using DR_CLEAR (atomic) */
            __attribute__((always_inline))
            inline void MOSI_LOW() {
                *m_clr = m_mosi_mask;
            }

            /**
             * @brief Read MISO pin state from PSR
             * @return 1 if MISO is HIGH, 0 if LOW
             *
             * @note Per RM: Two wait states are required any time PSR is
             *       accessed for synchronization. The 4 NOPs below account
             *       for this and provide signal settling time.
             */
            __attribute__((always_inline))
            inline uint32_t MISO_READ() {
                return (*m_psr & m_miso_mask) ? 1 : 0;
            }

            /**
             * @brief Write a bit to MOSI
             * @param bit  0 or 1
             */
            __attribute__((always_inline))
            inline void MOSI_WRITE(uint32_t bit) {
                if (bit) {
                    *m_set = m_mosi_mask;
                } else {
                    *m_clr = m_mosi_mask;
                }
            }

            /**
             * @brief Ultra-fast software SPI transfer one byte
             * @param tx  Byte to transmit
             * @return    Byte received
             *
             * @details Fully unrolled for speed. Uses DR_SET/DR_CLEAR for
             *          atomic pin control. 4 NOPs account for PSR wait states
             *          (2 wait states per RM) plus signal settling time.
             *
             *          SPI timing (CPOL=0, CPHA=0):
             *          - Data captured on SCK rising edge
             *          - Data changed on SCK falling edge
             *          - SCK idles LOW
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

            /**
             * @brief Fast SPI write multiple bytes
             * @param data  Data buffer to write
             * @param len   Number of bytes to write
             */
            __attribute__((always_inline))
            inline void fast_spi_write(const uint8_t *data, size_t len) {
                for (size_t i = 0; i < len; ++i) {
                    fast_spi_xfer(data[i]);
                }
            }

            /**
             * @brief Fast SPI read multiple bytes
             * @param data  Buffer to store read data
             * @param len   Number of bytes to read
             */
            __attribute__((always_inline))
            inline void fast_spi_read(uint8_t *data, size_t len) {
                for (size_t i = 0; i < len; ++i) {
                    data[i] = fast_spi_xfer(0xFF);
                }
            }

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
                fast_spi_xfer(0xFF);        // Dummy clock
                fast_spi_write(buf, 6);

                // Wait for R1 response (MSB = 0)
                uint8_t resp;
                int timeout = 8;
                do {
                    resp = fast_spi_xfer(0xFF);
                    --timeout;
                } while ((resp & 0x80) && timeout > 0);

                // Raise CS unless command succeeded and caller wants it low
                bool success = (resp == 0x00) || (resp == R1_IDLE);
                if (!keep_cs_low || !success) {
                    CS_HIGH();
                    fast_spi_xfer(0xFF);
                }

                return resp;
            }

            /**
             * @brief Internal read block implementation (no retry)
             */
            bool read_block_internal(uint32_t block, uint8_t *buffer) {
                if (send_cmd(CMD17, block, true) != 0x00) {
                    return false;
                }

                // CS is already low, wait for data start token
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
                fast_spi_xfer(0xFF);        // CRC high byte
                fast_spi_xfer(0xFF);        // CRC low byte

                CS_HIGH();
                fast_spi_xfer(0xFF);
                return true;
            }

            /**
             * @brief Internal write block implementation (no retry)
             */
            bool write_block_internal(uint32_t block, const uint8_t *buffer) {
                if (send_cmd(CMD24, block, true) != 0x00) {
                    return false;
                }

                // CS is already low
                fast_spi_xfer(DATA_START_TOKEN);
                fast_spi_write(buffer, BLOCK_SIZE);
                fast_spi_xfer(0xFF);        // CRC high byte
                fast_spi_xfer(0xFF);        // CRC low byte

                uint8_t resp = fast_spi_xfer(0xFF);
                CS_HIGH();

                // Check if data was accepted (bits 0-4 = 00101)
                if ((resp & 0x1F) != DATA_RESPONSE) {
                    return false;
                }

                // Wait for busy
                int timeout = 50000;
                CS_LOW();
                do {
                    resp = fast_spi_xfer(0xFF);
                    --timeout;
                } while (resp != 0xFF && timeout > 0);
                CS_HIGH();

                return (resp == 0xFF);
            }

            /**
             * @brief Millisecond delay using CPU cycles
             * @param ms  Milliseconds to delay
             */
            static inline void delay_ms(uint32_t ms) {
                uint32_t cpu_freq = CLOCK_GetFreq(kCLOCK_CpuClk);
                // 1 ms = cpu_freq / 1000 cycles, each loop ~3 cycles
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
     * @brief FAT16/FAT32 filesystem driver with read/write/append/delete support
     *
     * @details Supports FAT16 and FAT32, 8.3 filenames only.
     *          Provides: mount, open, read, write, append, delete.
     */
    class FATFS {
        public:
            // FAT BPB (BIOS Parameter Block) offsets
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

            // Directory entry offsets
            static constexpr uint16_t DIR_NAME      = 0;
            static constexpr uint16_t DIR_ATTR      = 11;
            static constexpr uint16_t DIR_FSTCLUSHI = 20;
            static constexpr uint16_t DIR_FSTCLUSLO = 26;
            static constexpr uint16_t DIR_FILESIZE  = 28;

            // File attributes
            static constexpr uint8_t ATTR_ARCHIVE   = 0x20;
            static constexpr uint8_t ATTR_DIRECTORY = 0x10;
            static constexpr uint8_t ATTR_LONG_NAME = 0x0F;

            // Cluster constants
            static constexpr uint32_t CLUSTER_FREE = 0x00000000;

            /**
             * @brief Get the EOC (End Of Chain) value for current FAT type
             * @return EOC value (0x0FFFFFF8 for FAT32, 0xFFFF for FAT16)
             */
            uint32_t eoc_value() const {
                return m_is_fat32 ? 0x0FFFFFF8 : 0xFFFF;
            }

            /**
             * @brief Check if a cluster is end-of-chain
             * @param cluster  Cluster number to check
             * @return true if cluster is end-of-chain
             */
            bool is_eoc(uint32_t cluster) const {
                if (m_is_fat32) {
                    return cluster >= 0x0FFFFFF8;
                } else {
                    return cluster >= 0xFFF8;
                }
            }

            /**
             * @brief File handle structure for open files
             */
            struct File {
                uint32_t start_cluster;
                uint32_t current_cluster;
                uint32_t file_size;
                uint32_t position;
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

                // Parse BPB
                m_bytes_per_sec = read16(m_block_buf, BS_BYTSPERSEC);
                m_sec_per_clus  = m_block_buf[BS_SECPERCLUS];
                uint16_t rsvd_sec     = read16(m_block_buf, BS_RSVDSECCNT);
                uint8_t  num_fats     = m_block_buf[BS_NUMFATS];
                uint16_t root_entries = read16(m_block_buf, BS_ROOTENTCNT);

                uint32_t fatsz16 = read16(m_block_buf, BS_FATSZ16);
                if (fatsz16 != 0) {
                    // FAT16
                    m_fat_size  = fatsz16;
                    m_total_sectors = read16(m_block_buf, BS_TOTSEC16);
                    if (m_total_sectors == 0) {
                        m_total_sectors = read32(m_block_buf, BS_TOTSEC32);
                    }
                    m_is_fat32 = false;
                } else {
                    // FAT32
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

            /**
             * @brief Check if filesystem is mounted
             * @return true if mounted
             */
            bool is_mounted() const {
                return m_mounted;
            }

            /**
             * @brief Open a file by name
             * @param name  Filename (8.3 format, e.g., "README.TXT")
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
             * @param file  File handle (updated with current position)
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
             * @brief Read an entire file into a vector
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
             * @brief Read an entire file into a string
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
             * @brief Write data to a file (creates or overwrites)
             * @param name  Filename (8.3 format)
             * @param data  Data to write
             * @param size  Number of bytes
             * @return true on success
             *
             * @details This function first writes all data to new clusters,
             *          then releases the old cluster chain (if any).
             *          This provides better atomicity than releasing first.
             */
            bool write_file(const char *name, const uint8_t *data, uint32_t size) {
                if (!m_mounted || !name || !data || size == 0) {
                    return false;
                }

                uint32_t sector, offset;
                File existing;
                uint32_t old_cluster = 0;
                bool has_existing = false;

                // Check if file already exists
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
                            // Release allocated clusters on error
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

                // Data written successfully, now release old clusters
                if (has_existing && old_cluster != 0 && !is_eoc(old_cluster)) {
                    release_cluster_chain(old_cluster);
                    m_card.set_last_alloc_cluster(2);
                }

                // Find or create directory entry
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
             * @brief Write a string to a file (creates or overwrites)
             * @param name  Filename (8.3 format)
             * @param str   String to write
             * @return true on success
             */
            bool write_file(const char *name, const std::string &str) {
                return write_file(name, (const uint8_t*)str.c_str(), str.size());
            }

            /**
             * @brief Append data to an existing file
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

                // Use File struct to cache cluster chain traversal
                File file;
                file.start_cluster = start_cluster;
                file.current_cluster = start_cluster;
                file.file_size = file_size;
                file.position = 0;

                // Fast forward to end of file
                uint32_t bytes_per_cluster = (uint32_t)m_sec_per_clus * m_bytes_per_sec;

                while (file.position < file.file_size) {
                    uint32_t cluster_offset = file.position % bytes_per_cluster;
                    if (cluster_offset == 0 && file.position > 0) {
                        uint32_t next = next_cluster(file.current_cluster);
                        if (is_eoc(next)) {
                            break;
                        }
                        file.current_cluster = next;
                    }
                    uint32_t remaining = file.file_size - file.position;
                    uint32_t chunk = bytes_per_cluster - cluster_offset;
                    if (chunk > remaining) {
                        chunk = remaining;
                    }
                    file.position += chunk;
                }

                // file.current_cluster is now the last cluster
                uint32_t cur_cluster = file.current_cluster;

                // Handle empty file case
                if (file_size == 0) {
                    // File is empty, just use start cluster or allocate new one
                    if (cur_cluster == 0 || is_eoc(cur_cluster)) {
                        cur_cluster = alloc_cluster();
                        if (cur_cluster == 0) {
                            return false;
                        }
                        // Update directory entry with first cluster
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
                uint32_t position = file.file_size;
                uint8_t temp[512];

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

                // Update file size in directory
                if (!m_card.read_block(sector, m_block_buf)) {
                    return false;
                }
                write32(m_block_buf, offset + DIR_FILESIZE, position);
                return m_card.write_block(sector, m_block_buf);
            }

            /**
             * @brief Append a string to an existing file
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

                // Clean up LFN (Long File Name) entries if present
                // LFN entries appear before the short name entry
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

                // Mark short name entry as deleted
                m_block_buf[offset] = 0xE5;
                return m_card.write_block(sector, m_block_buf);
            }

            /**
             * @brief Unmount the filesystem
             */
            void unmount() {
                m_mounted = false;
            }

        private:
            /**
             * @brief Release a cluster chain back to free pool
             * @param cluster  Starting cluster of the chain
             */
            void release_cluster_chain(uint32_t cluster) {
                while (!is_eoc(cluster)) {
                    uint32_t next = next_cluster(cluster);
                    set_cluster(cluster, CLUSTER_FREE);
                    if (next == 0 || is_eoc(next)) {
                        break;
                    }
                    cluster = next;
                }
            }

            /**
             * @brief Find a directory entry by name
             * @param name                  Filename to search
             * @param sector_out           [out] Sector containing the entry
             * @param offset_out           [out] Offset within sector
             * @param create_if_not_found  If true, return first free slot
             * @return true on success
             */
            bool find_dir_entry(const char *name, uint32_t &sector_out,
                                uint16_t &offset_out, bool create_if_not_found) {
                uint32_t cluster = m_is_fat32 ? m_root_cluster : 0;

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
                    if (is_eoc(cluster)) {
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

                // Wrap around and search from beginning
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

            /**
             * @brief Compare 8.3 filename with directory entry
             * @param entry  Directory entry (11 bytes, space-padded)
             * @param name   Filename (e.g., "README.TXT")
             * @return true if match
             */
            static bool name_match(const uint8_t *entry, const char *name) {
                if (!entry || !name) return false;

                char fname[13] = {0};
                int pos = 0;

                // Build 8.3 name from directory entry
                for (int i = 0; i < 8 && entry[i] != ' '; ++i) {
                    fname[pos++] = entry[i];
                }
                if (entry[8] != ' ') {
                    fname[pos++] = '.';
                    for (int i = 8; i < 11 && entry[i] != ' '; ++i) {
                        fname[pos++] = entry[i];
                    }
                }

                // Case-insensitive compare with bounds check
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

            /**
             * @brief Convert filename to 8.3 format
             * @param name  Source filename
             * @param out   11-byte buffer (space-padded)
             */
            static void make_83_name(const char *name, uint8_t *out) {
                memset(out, ' ', 11);

                const char *dot = strchr(name, '.');
                int name_len = dot ? (int)(dot - name) : (int)strlen(name);
                if (name_len > 8) name_len = 8;

                // Convert name part to uppercase
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

                // Convert extension part to uppercase
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