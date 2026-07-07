/** @file /inc/sdcard.hpp
 *  @brief SPI SD card interface with FAT16/FAT32 support
 *  @author hdkghc
 *  @version 0.1
 *  @example
    @code
    SDCard::SDCardIO sdcard(spi1, 8, 11, 10, 9);
    SDCard::FATFS    fatfs(sdcard);

    if (sdcard.init() && fatfs.mount()) {
        // Read file to string
        std::string content;
        if (fatfs.read_file("README.TXT", content)) {
            printf("File: %s\n", content.c_str());
        }

        // Write string to file
        fatfs.write_file("OUTPUT.TXT", "Hello, World!");

        // Append to file
        fatfs.append_file("LOG.TXT", "Another line\n");

        // Format card (wipes all data!)
        // fatfs.format();

        // Eject
        fatfs.unmount();
        sdcard.unmount();
    }
    @endcode
 *  Copyright (C) 2026 hdkghc (peitongxin@outlook.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SDCARD_HPP
#define SDCARD_HPP

extern "C" {
    #include <pico/stdlib.h>
    #include <hardware/spi.h>
}

#include <cstdint>
#include <string>
#include <vector>
#include <cstring>

namespace SDCard {

    // ==================== Low-Level SD Card ====================

    /**
     * @brief Low-level SPI SD card block driver
     */
    class SDCardIO {
        static constexpr uint8_t  CMD0        = 0x00; ///< GO_IDLE_STATE
        static constexpr uint8_t  CMD8        = 0x08; ///< SEND_IF_COND
        static constexpr uint8_t  CMD17       = 0x11; ///< READ_SINGLE_BLOCK
        static constexpr uint8_t  CMD24       = 0x18; ///< WRITE_BLOCK
        static constexpr uint8_t  CMD55       = 0x37; ///< APP_CMD
        static constexpr uint8_t  ACMD41      = 0x29; ///< SD_SEND_OP_COND
        static constexpr int      BLOCK_SIZE  = 512;

        spi_inst_t *spi;
        uint8_t     miso, mosi, sck, cs;
        bool        mounted;

        void cs_low() {
            asm volatile("nop \n nop \n nop");
            gpio_put(cs, 0);
            asm volatile("nop \n nop \n nop");
        }

        void cs_high() {
            asm volatile("nop \n nop \n nop");
            gpio_put(cs, 1);
            asm volatile("nop \n nop \n nop");
        }

        uint8_t spi_xfer(uint8_t data) {
            uint8_t rx;
            spi_write_read_blocking(spi, &data, &rx, 1);
            return rx;
        }

        uint8_t send_cmd(uint8_t cmd, uint32_t arg) {
            uint8_t buf[6];
            buf[0] = cmd | 0x40;
            buf[1] = (arg >> 24) & 0xFF;
            buf[2] = (arg >> 16) & 0xFF;
            buf[3] = (arg >> 8)  & 0xFF;
            buf[4] = arg & 0xFF;
            buf[5] = 0x95;

            cs_low();
            spi_write_blocking(spi, buf, 6);

            uint8_t resp;
            for (int i = 0; i < 8; i++) {
                resp = spi_xfer(0xFF);
                if (!(resp & 0x80)) break;
            }
            cs_high();
            return resp;
        }

    public:
        /**
         * @brief Constructor
         * @param spi   SPI instance (default spi1)
         * @param miso  MISO pin (default 8)
         * @param mosi  MOSI pin (default 11)
         * @param sck   Clock pin (default 10)
         * @param cs    Chip select pin (default 9)
         */
        SDCardIO(spi_inst_t *spi = spi1, uint miso = 8, uint mosi = 11,
                 uint sck = 10, uint cs = 9)
            : spi(spi), miso(miso), mosi(mosi), sck(sck), cs(cs), mounted(false) {}

        /**
         * @brief Initialize SPI and mount SD card
         * @param freq  SPI frequency in Hz (default 400kHz)
         * @return      true on success
         */
        bool init(uint freq = 400 * 1000) {
            spi_init(spi, freq);
            gpio_set_function(miso, GPIO_FUNC_SPI);
            gpio_set_function(mosi, GPIO_FUNC_SPI);
            gpio_set_function(sck,  GPIO_FUNC_SPI);
            gpio_init(cs);
            gpio_set_dir(cs, GPIO_OUT);
            cs_high();

            for (int i = 0; i < 10; i++) spi_xfer(0xFF);

            int retry;
            for (retry = 0; retry < 10; retry++) {
                if (send_cmd(CMD0, 0) == 0x01) break;
                sleep_ms(10);
            }
            if (retry == 10) return false;

            if (send_cmd(CMD8, 0x1AA) != 0x01) return false;

            for (retry = 0; retry < 100; retry++) {
                send_cmd(CMD55, 0);
                if (send_cmd(ACMD41, 0x40000000) == 0x00) break;
                sleep_ms(10);
            }
            if (retry == 100) return false;

            spi_set_baudrate(spi, 10 * 1000 * 1000);
            mounted = true;
            return true;
        }

        bool is_mounted() const { return mounted; }

        bool read_block(uint32_t block, uint8_t *buffer) {
            if (!mounted) return false;
            if (send_cmd(CMD17, block) != 0x00) return false;

            cs_low();
            for (int i = 0; i < 10000; i++) {
                if (spi_xfer(0xFF) == 0xFE) break;
            }
            spi_read_blocking(spi, 0xFF, buffer, BLOCK_SIZE);
            spi_xfer(0xFF); spi_xfer(0xFF);
            cs_high();
            return true;
        }

        bool write_block(uint32_t block, const uint8_t *buffer) {
            if (!mounted) return false;
            if (send_cmd(CMD24, block) != 0x00) return false;

            cs_low();
            spi_xfer(0xFE);
            spi_write_blocking(spi, buffer, BLOCK_SIZE);
            spi_xfer(0xFF); spi_xfer(0xFF);

            uint8_t resp = spi_xfer(0xFF);
            cs_high();
            return (resp & 0x1F) == 0x05;
        }

        void unmount() {
            cs_high();
            mounted = false;
        }
    };

    // ==================== FAT16/FAT32 Filesystem ====================

    /**
     * @brief FAT16/FAT32 filesystem driver with read/write/format support
     */
    class FATFS {
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

        static constexpr uint16_t DIR_NAME      = 0;
        static constexpr uint16_t DIR_ATTR      = 11;
        static constexpr uint16_t DIR_FSTCLUSLO = 26;
        static constexpr uint16_t DIR_FSTCLUSHI = 20;
        static constexpr uint16_t DIR_FILESIZE  = 28;

        static constexpr uint8_t  ATTR_ARCHIVE   = 0x20;
        static constexpr uint8_t  ATTR_DIRECTORY = 0x10;
        static constexpr uint8_t  ATTR_LONG_NAME = 0x0F;
        static constexpr uint32_t CLUSTER_END    = 0x0FFFFFF8;
        static constexpr uint32_t CLUSTER_FREE   = 0x00000000;

        SDCardIO  &card;
        uint8_t   block_buf[512];
        uint16_t  bytes_per_sec;
        uint8_t   sec_per_clus;
        uint32_t  fat_start;
        uint32_t  fat_size;
        uint32_t  root_start;
        uint32_t  data_start;
        uint32_t  root_cluster;
        uint32_t  total_sectors;
        bool      is_fat32;
        bool      mounted;

        static uint32_t read32(uint8_t *buf, uint16_t off) {
            return (uint32_t)buf[off] | ((uint32_t)buf[off+1] << 8) |
                   ((uint32_t)buf[off+2] << 16) | ((uint32_t)buf[off+3] << 24);
        }

        static uint16_t read16(uint8_t *buf, uint16_t off) {
            return (uint16_t)buf[off] | ((uint16_t)buf[off+1] << 8);
        }

        static void write32(uint8_t *buf, uint16_t off, uint32_t val) {
            buf[off]   = val & 0xFF;
            buf[off+1] = (val >> 8)  & 0xFF;
            buf[off+2] = (val >> 16) & 0xFF;
            buf[off+3] = (val >> 24) & 0xFF;
        }

        static void write16(uint8_t *buf, uint16_t off, uint16_t val) {
            buf[off]   = val & 0xFF;
            buf[off+1] = (val >> 8) & 0xFF;
        }

        uint32_t cluster_to_sector(uint32_t cluster) {
            return data_start + (cluster - 2) * sec_per_clus;
        }

        uint32_t next_cluster(uint32_t cluster) {
            uint32_t fat_offset = is_fat32 ? (cluster * 4) : (cluster * 2);
            uint32_t fat_sector = fat_start + (fat_offset / bytes_per_sec);
            uint16_t fat_index  = fat_offset % bytes_per_sec;

            card.read_block(fat_sector, block_buf);
            if (is_fat32) {
                return read32(block_buf, fat_index) & 0x0FFFFFFF;
            } else {
                return read16(block_buf, fat_index);
            }
        }

        bool set_cluster(uint32_t cluster, uint32_t value) {
            uint32_t fat_offset = is_fat32 ? (cluster * 4) : (cluster * 2);
            uint32_t fat_sector = fat_start + (fat_offset / bytes_per_sec);
            uint16_t fat_index  = fat_offset % bytes_per_sec;

            if (!card.read_block(fat_sector, block_buf)) return false;

            if (is_fat32) {
                uint32_t old = read32(block_buf, fat_index);
                value = (value & 0x0FFFFFFF) | (old & 0xF0000000);
                write32(block_buf, fat_index, value);
            } else {
                write16(block_buf, fat_index, (uint16_t)value);
            }

            // Write both FAT copies
            if (!card.write_block(fat_sector, block_buf)) return false;
            uint32_t fat2_sector = fat_sector + fat_size;
            if (!card.write_block(fat2_sector, block_buf)) return false;
            return true;
        }

        uint32_t alloc_cluster() {
            // Simple linear search for free cluster
            for (uint32_t cluster = 2; cluster < total_sectors / sec_per_clus; cluster++) {
                if (next_cluster(cluster) == CLUSTER_FREE) {
                    set_cluster(cluster, CLUSTER_END);
                    return cluster;
                }
            }
            return 0;
        }

        static bool name_match(const uint8_t *entry, const char *name) {
            char fname[13] = {0};
            int pos = 0;
            for (int i = 0; i < 8 && entry[i] != ' '; i++) fname[pos++] = entry[i];
            if (entry[8] != ' ') {
                fname[pos++] = '.';
                for (int i = 8; i < 11 && entry[i] != ' '; i++) fname[pos++] = entry[i];
            }
            for (int i = 0; i < 13; i++) {
                char a = fname[i], b = name[i];
                if (a >= 'A' && a <= 'Z') a += 32;
                if (b >= 'A' && b <= 'Z') b += 32;
                if (a != b) return false;
                if (a == 0 && b == 0) return true;
            }
            return true;
        }

        void make_83_name(const char *name, uint8_t *out) {
            memset(out, ' ', 11);
            const char *dot = strchr(name, '.');
            int name_len = dot ? (int)(dot - name) : (int)strlen(name);
            if (name_len > 8) name_len = 8;
            for (int i = 0; i < name_len; i++) {
                out[i] = (name[i] >= 'a' && name[i] <= 'z') ? (name[i] - 32) : name[i];
            }
            if (dot && dot[1]) {
                int ext_len = strlen(dot + 1);
                if (ext_len > 3) ext_len = 3;
                for (int i = 0; i < ext_len; i++) {
                    out[8 + i] = (dot[1+i] >= 'a' && dot[1+i] <= 'z') ? (dot[1+i] - 32) : dot[1+i];
                }
            }
        }

        bool find_free_dir_entry(uint32_t dir_cluster, uint32_t &sector_out, uint16_t &offset_out) {
            uint32_t cluster = is_fat32 ? dir_cluster : 0;

            while (true) {
                uint32_t base_sector = is_fat32 ? cluster_to_sector(cluster) : root_start;

                for (uint32_t sec = 0; sec < sec_per_clus; sec++) {
                    if (!card.read_block(base_sector + sec, block_buf)) return false;

                    for (uint16_t off = 0; off < bytes_per_sec; off += 32) {
                        uint8_t first = block_buf[off];
                        if (first == 0x00 || first == 0xE5) {
                            sector_out = base_sector + sec;
                            offset_out = off;
                            return true;
                        }
                    }
                }

                if (!is_fat32) break;
                cluster = next_cluster(cluster);
                if (cluster >= CLUSTER_END) break;
            }
            return false;
        }

        void build_fat16_bpb(uint8_t *buf, uint32_t total_sec) {
            memset(buf, 0, 512);
            buf[0] = 0xEB; buf[1] = 0x3C; buf[2] = 0x90;
            memcpy(buf + 3, "MSDOS5.0", 8);
            write16(buf, BS_BYTSPERSEC, 512);
            buf[BS_SECPERCLUS] = (total_sec > 65536) ? 8 : 4;
            write16(buf, BS_RSVDSECCNT, 1);
            buf[BS_NUMFATS] = 2;
            write16(buf, BS_ROOTENTCNT, 512);
            write16(buf, BS_TOTSEC16, (total_sec < 65536) ? total_sec : 0);
            buf[21] = 0xF8;
            uint16_t fatsz = (total_sec > 65536) ? 128 : 32;
            write16(buf, BS_FATSZ16, fatsz);
            if (total_sec >= 65536) write32(buf, BS_TOTSEC32, total_sec);
            buf[510] = 0x55; buf[511] = 0xAA;
        }

    public:
        struct File {
            uint32_t start_cluster;
            uint32_t current_cluster;
            uint32_t file_size;
            uint32_t position;
        };

        FATFS(SDCardIO &card_ref) : card(card_ref), mounted(false) {}

        bool mount() {
            if (!card.is_mounted()) return false;
            if (!card.read_block(0, block_buf)) return false;

            bytes_per_sec = read16(block_buf, BS_BYTSPERSEC);
            sec_per_clus  = block_buf[BS_SECPERCLUS];
            uint16_t rsvd_sec     = read16(block_buf, BS_RSVDSECCNT);
            uint8_t  num_fats     = block_buf[BS_NUMFATS];
            uint16_t root_entries = read16(block_buf, BS_ROOTENTCNT);

            uint32_t fatsz16 = read16(block_buf, BS_FATSZ16);
            if (fatsz16 != 0) {
                fat_size  = fatsz16;
                total_sectors = read16(block_buf, BS_TOTSEC16);
                if (total_sectors == 0) total_sectors = read32(block_buf, BS_TOTSEC32);
                is_fat32 = false;
            } else {
                fat_size  = read32(block_buf, BS_FATSZ32);
                total_sectors = read32(block_buf, BS_TOTSEC32);
                is_fat32 = true;
            }

            fat_start  = rsvd_sec;
            uint32_t root_dir_sectors = ((root_entries * 32) + (bytes_per_sec - 1)) / bytes_per_sec;
            root_start = fat_start + num_fats * fat_size;
            data_start = root_start + root_dir_sectors;

            if (is_fat32) {
                root_cluster = read32(block_buf, BS_ROOTCLUS);
            }

            mounted = true;
            return true;
        }

        bool format() {
            if (!card.is_mounted()) return false;

            uint32_t total_sec = 2000000;
            uint8_t  spc       = (total_sec > 65536) ? 8 : 4;
            uint16_t fatsz     = (total_sec > 65536) ? 128 : 32;

            memset(block_buf, 0, 512);
            build_fat16_bpb(block_buf, total_sec);
            block_buf[0x1BE] = 0x80;
            block_buf[0x1C2] = 0x06;
            write32(block_buf, 0x1C6, 1);
            write32(block_buf, 0x1CA, total_sec - 1);
            block_buf[0x1FE] = 0x55; block_buf[0x1FF] = 0xAA;
            card.write_block(0, block_buf);

            memset(block_buf, 0, 512);
            build_fat16_bpb(block_buf, total_sec - 1);
            card.write_block(1, block_buf);

            memset(block_buf, 0, 512);
            block_buf[0] = 0xF8; block_buf[1] = 0xFF; block_buf[2] = 0xFF; block_buf[3] = 0xFF;
            uint32_t fat_sector = 2;
            for (int fat = 0; fat < 2; fat++) {
                card.write_block(fat_sector, block_buf);
                memset(block_buf, 0, 512);
                for (uint16_t i = 1; i < fatsz; i++) {
                    card.write_block(fat_sector + i, block_buf);
                }
                fat_sector += fatsz;
            }

            memset(block_buf, 0, 512);
            uint16_t root_sectors = (512 * 32) / 512;
            for (uint16_t i = 0; i < root_sectors; i++) {
                card.write_block(fat_sector + i, block_buf);
            }

            return mount();
        }

        bool open(const char *name, File &file) {
            if (!mounted) return false;

            uint32_t cluster = is_fat32 ? root_cluster : 0;
            uint32_t sector;
            uint8_t entry_buf[32];

            while (true) {
                sector = is_fat32 ? cluster_to_sector(cluster) : root_start;

                for (uint32_t sec = 0; sec < sec_per_clus; sec++) {
                    if (!card.read_block(sector + sec, block_buf)) return false;

                    for (uint16_t off = 0; off < bytes_per_sec; off += 32) {
                        memcpy(entry_buf, block_buf + off, 32);
                        uint8_t first = entry_buf[0];
                        if (first == 0x00) return false;
                        if (first == 0xE5) continue;
                        if (entry_buf[DIR_ATTR] == ATTR_LONG_NAME) continue;

                        if (name_match(entry_buf, name)) {
                            file.start_cluster = read16(entry_buf, DIR_FSTCLUSLO);
                            if (is_fat32) {
                                file.start_cluster |= (read16(entry_buf, DIR_FSTCLUSHI) << 16);
                            }
                            file.current_cluster = file.start_cluster;
                            file.file_size = read32(entry_buf, DIR_FILESIZE);
                            file.position  = 0;
                            return true;
                        }
                    }
                }

                if (!is_fat32) break;
                cluster = next_cluster(cluster);
                if (cluster >= CLUSTER_END) break;
            }
            return false;
        }

        uint32_t read(File &file, uint8_t *buf, uint32_t size) {
            if (!mounted || file.position >= file.file_size) return 0;

            uint32_t remaining = file.file_size - file.position;
            if (size > remaining) size = remaining;

            uint32_t read_total = 0;
            uint8_t  temp[512];

            while (read_total < size) {
                uint32_t cluster_offset = file.position % (sec_per_clus * bytes_per_sec);
                uint32_t sector_offset  = cluster_offset / bytes_per_sec;
                uint16_t byte_offset    = cluster_offset % bytes_per_sec;
                uint32_t sector = cluster_to_sector(file.current_cluster) + sector_offset;

                card.read_block(sector, temp);

                uint16_t chunk = bytes_per_sec - byte_offset;
                if (chunk > (size - read_total)) chunk = size - read_total;

                memcpy(buf + read_total, temp + byte_offset, chunk);
                read_total += chunk;
                file.position += chunk;

                if (file.position >= file.file_size) break;

                uint32_t next_boundary = file.current_cluster * sec_per_clus * bytes_per_sec
                                       + sec_per_clus * bytes_per_sec;
                if (file.position >= next_boundary) {
                    file.current_cluster = next_cluster(file.current_cluster);
                    if (file.current_cluster >= CLUSTER_END) break;
                }
            }
            return read_total;
        }

        bool read_file(const char *name, std::vector<uint8_t> &data) {
            File file;
            if (!open(name, file)) return false;
            data.resize(file.file_size);
            uint32_t got = read(file, data.data(), file.file_size);
            data.resize(got);
            return got > 0;
        }

        bool read_file(const char *name, std::string &str) {
            std::vector<uint8_t> data;
            if (!read_file(name, data)) return false;
            str.assign((const char*)data.data(), data.size());
            return true;
        }

        /**
         * @brief Write data to a file (creates or overwrites)
         * @param name  Filename (8.3 format, e.g. "DATA.TXT")
         * @param data  Data to write
         * @param size  Number of bytes
         * @return      true on success
         */
        bool write_file(const char *name, const uint8_t *data, uint32_t size) {
            if (!mounted) return false;

            uint32_t dir_cluster = is_fat32 ? root_cluster : 0;

            // Allocate first cluster
            uint32_t cluster = alloc_cluster();
            if (cluster == 0) return false;

            // Write data
            uint32_t written = 0;
            uint32_t cur_cluster = cluster;
            uint8_t temp[512];

            while (written < size) {
                uint32_t base_sector = cluster_to_sector(cur_cluster);

                for (uint32_t sec = 0; sec < sec_per_clus && written < size; sec++) {
                    memset(temp, 0, 512);
                    uint16_t chunk = bytes_per_sec;
                    if (chunk > (size - written)) chunk = size - written;
                    memcpy(temp, data + written, chunk);
                    card.write_block(base_sector + sec, temp);
                    written += chunk;
                }

                if (written < size) {
                    uint32_t next = alloc_cluster();
                    if (next == 0) return false;
                    set_cluster(cur_cluster, next);
                    cur_cluster = next;
                }
            }

            // Write directory entry
            uint32_t dir_sec;
            uint16_t dir_off;
            if (!find_free_dir_entry(dir_cluster, dir_sec, dir_off)) return false;
            if (!card.read_block(dir_sec, block_buf)) return false;

            memset(block_buf + dir_off, 0, 32);
            make_83_name(name, block_buf + dir_off + DIR_NAME);
            block_buf[dir_off + DIR_ATTR] = ATTR_ARCHIVE;
            write16(block_buf, dir_off + DIR_FSTCLUSLO, cluster & 0xFFFF);
            if (is_fat32) write16(block_buf, dir_off + DIR_FSTCLUSHI, (cluster >> 16) & 0xFFFF);
            write32(block_buf, dir_off + DIR_FILESIZE, size);

            return card.write_block(dir_sec, block_buf);
        }

        /**
         * @brief Write a string to a file (creates or overwrites)
         * @param name   Filename (8.3 format)
         * @param str    String to write
         * @return       true on success
         */
        bool write_file(const char *name, const std::string &str) {
            return write_file(name, (const uint8_t*)str.c_str(), str.size());
        }

        /**
         * @brief Append data to an existing file
         * @param name  Filename (8.3 format)
         * @param data  Data to append
         * @param size  Number of bytes
         * @return      true on success
         */
        bool append_file(const char *name, const uint8_t *data, uint32_t size) {
            File file;
            if (!open(name, file)) {
                // File doesn't exist, create it
                return write_file(name, data, size);
            }

            // Find last cluster of existing file
            uint32_t cur_cluster = file.start_cluster;
            uint32_t next;
            while ((next = next_cluster(cur_cluster)) < CLUSTER_END) {
                cur_cluster = next;
            }

            // Write from end position
            uint32_t written = 0;
            uint32_t position = file.file_size;
            uint8_t temp[512];

            while (written < size) {
                uint32_t cluster_offset = position % (sec_per_clus * bytes_per_sec);
                uint32_t sector_offset  = cluster_offset / bytes_per_sec;
                uint16_t byte_offset    = cluster_offset % bytes_per_sec;

                if (byte_offset == 0 && sector_offset == 0 && position > 0) {
                    // Need new cluster
                    uint32_t new_cluster = alloc_cluster();
                    if (new_cluster == 0) return false;
                    set_cluster(cur_cluster, new_cluster);
                    cur_cluster = new_cluster;
                }

                uint32_t sector = cluster_to_sector(cur_cluster) + sector_offset;

                if (byte_offset != 0) {
                    // Partial sector: read-modify-write
                    card.read_block(sector, temp);
                } else {
                    memset(temp, 0, 512);
                }

                uint16_t chunk = bytes_per_sec - byte_offset;
                if (chunk > (size - written)) chunk = size - written;
                memcpy(temp + byte_offset, data + written, chunk);
                card.write_block(sector, temp);

                written  += chunk;
                position += chunk;
            }

            // Update file size in directory
            uint32_t dir_cluster = is_fat32 ? root_cluster : 0;
            uint32_t dir_sector = is_fat32 ? cluster_to_sector(dir_cluster) : root_start;
            uint8_t entry_buf[32];

            for (uint32_t sec = 0; sec < sec_per_clus; sec++) {
                if (!card.read_block(dir_sector + sec, block_buf)) return false;
                for (uint16_t off = 0; off < bytes_per_sec; off += 32) {
                    if (name_match(block_buf + off, name)) {
                        write32(block_buf, off + DIR_FILESIZE, position);
                        return card.write_block(dir_sector + sec, block_buf);
                    }
                }
            }
            return false;
        }

        /**
         * @brief Append a string to an existing file
         * @param name  Filename (8.3 format)
         * @param str   String to append
         * @return      true on success
         */
        bool append_file(const char *name, const std::string &str) {
            return append_file(name, (const uint8_t*)str.c_str(), str.size());
        }

        void unmount() { mounted = false; }
        bool is_mounted() const { return mounted; }
    };

} // namespace SDCard

#endif