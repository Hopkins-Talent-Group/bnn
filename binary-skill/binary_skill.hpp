#pragma once

#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <stdexcept>
#include <memory>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace bnn {

// ─────────────────────────────────────────────────────────────────────────────
// 1. Binary Layout Structures (VSKB0001)
// ─────────────────────────────────────────────────────────────────────────────

// cppcheck-suppress-begin [preprocessorErrorDirective]
#pragma pack(push, 1)

struct BinaryHeader {
    char     magic[4];            // "VSKB"
    uint16_t version;             // 1
    uint16_t reserved;            // 0
    uint32_t skill_count;         // Total number of packed records
    uint32_t string_table_offset; // Byte offset where the string pool begins
    uint32_t string_table_bytes;  // Total byte size of the string pool
};

static_assert(sizeof(BinaryHeader) == 20, "BinaryHeader must be exactly 20 bytes");

struct SkillRecord {
    char     name[32];            // Null-terminated / padded name
    uint16_t category_id;         // Category enum
    uint16_t flags;               // Bit 0: has_scripts, Bit 1: nvidia_stack, Bit 2: has_license
    uint32_t desc_offset;         // Offset into string pool
    uint32_t desc_len;            // Byte length of description
    uint32_t body_offset;         // Offset into string pool
    uint32_t body_len;            // Byte length of markdown body
    uint8_t  sha256[32];          // Cryptographic SHA-256 of canonical source
    uint8_t  _pad[44];            // Cache-line alignment padding
};

static_assert(sizeof(SkillRecord) == 128, "SkillRecord must be exactly 128 bytes");

#pragma pack(pop)

enum Category : uint16_t {
    CAT_UNKNOWN             = 0,
    CAT_NVIDIA_STACK        = 1,
    CAT_CORE_ENGINEERING    = 2,
    CAT_ANTI_SLOP           = 3,
    CAT_DESIGN_FRONTEND     = 4,
    CAT_DATABASE_BACKEND    = 5,
    CAT_MULTIMODAL_CREATIVE = 6
};

inline const char* category_to_string(uint16_t cat) {
    switch (cat) {
        case CAT_NVIDIA_STACK:        return "NVIDIA_STACK";
        case CAT_CORE_ENGINEERING:    return "CORE_ENGINEERING";
        case CAT_ANTI_SLOP:           return "ANTI_SLOP";
        case CAT_DESIGN_FRONTEND:     return "DESIGN_FRONTEND";
        case CAT_DATABASE_BACKEND:    return "DATABASE_BACKEND";
        case CAT_MULTIMODAL_CREATIVE: return "MULTIMODAL_CREATIVE";
        default:                      return "UNKNOWN";
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// 2. Self-Contained SHA-256 Implementation (NIST FIPS 180-4)
// ─────────────────────────────────────────────────────────────────────────────

class Sha256 {
public:
    Sha256() : bitlen_(0), buflen_(0) {
        state_[0] = 0x6a09e667;
        state_[1] = 0xbb67ae85;
        state_[2] = 0x3c6ef372;
        state_[3] = 0xa54ff53a;
        state_[4] = 0x510e527f;
        state_[5] = 0x9b05688c;
        state_[6] = 0x1f83d9ab;
        state_[7] = 0x5be0cd19;
    }

    void update(const void* data, size_t len) {
        const auto* p = static_cast<const uint8_t*>(data);
        bitlen_ += static_cast<uint64_t>(len) * 8;
        while (len > 0) {
            const size_t take = std::min(len, static_cast<size_t>(64) - buflen_);
            std::memcpy(buffer_ + buflen_, p, take);
            buflen_ += take;
            p += take;
            len -= take;
            if (buflen_ == 64) {
                transform(buffer_);
                buflen_ = 0;
            }
        }
    }

    void finalize(uint8_t digest[32]) {
        const uint64_t bits = bitlen_;
        const uint8_t pad = 0x80U;
        update(&pad, 1);
        bitlen_ = bits;
        const uint8_t zero = 0x00U;
        while (buflen_ != 56U) {
            update(&zero, 1);
            bitlen_ = bits;
        }
        uint8_t lenbytes[8];
        for (unsigned i = 0U; i < 8U; ++i) {
            lenbytes[7U - i] = static_cast<uint8_t>(bits >> (i * 8U));
        }
        update(lenbytes, 8);

        for (unsigned i = 0U; i < 8U; ++i) {
            digest[i * 4U + 0U] = static_cast<uint8_t>((state_[i] >> 24U) & 0xFFU);
            digest[i * 4U + 1U] = static_cast<uint8_t>((state_[i] >> 16U) & 0xFFU);
            digest[i * 4U + 2U] = static_cast<uint8_t>((state_[i] >>  8U) & 0xFFU);
            digest[i * 4U + 3U] = static_cast<uint8_t>( state_[i]         & 0xFFU);
        }
    }

    std::string finalize_hex() {
        uint8_t d[32];
        finalize(d);
        static const char* const hexd = "0123456789abcdef";
        std::string s;
        s.reserve(64);
        for (unsigned i = 0U; i < 32U; ++i) {
            s.push_back(hexd[(d[i] >> 4U) & 0x0FU]);
            s.push_back(hexd[ d[i]        & 0x0FU]);
        }
        return s;
    }

    static std::string hash_hex(const void* data, size_t len) {
        Sha256 h;
        h.update(data, len);
        return h.finalize_hex();
    }

    static std::string hash_hex(std::string_view sv) {
        return hash_hex(sv.data(), sv.size());
    }

private:
    // cppcheck-suppress unusedPrivateFunction
    static constexpr uint32_t rotr(uint32_t x, unsigned n) noexcept {
        // Shift amounts are always in [1,31] per FIPS 180-4 — never 0 or 32.
        // Expressed as unsigned to suppress signed-shift portability warnings.
        return (x >> n) | (x << (32U - n));
    }

    void transform(const uint8_t block[64]) {
        // cppcheck-suppress constVariableReference
        static constexpr uint32_t kK[64] = {
            0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U,
            0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
            0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U,
            0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
            0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU,
            0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
            0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U,
            0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
            0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U,
            0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
            0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U,
            0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
            0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U,
            0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
            0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U,
            0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U,
        };

        uint32_t w[64];
        for (unsigned i = 0U; i < 16U; ++i) {
            w[i] = (static_cast<uint32_t>(block[i * 4U])       << 24U) |
                   (static_cast<uint32_t>(block[i * 4U + 1U])  << 16U) |
                   (static_cast<uint32_t>(block[i * 4U + 2U])  <<  8U) |
                    static_cast<uint32_t>(block[i * 4U + 3U]);
        }
        for (unsigned i = 16U; i < 64U; ++i) {
            const uint32_t s0 = rotr(w[i - 15U], 7U) ^ rotr(w[i - 15U], 18U) ^ (w[i - 15U] >> 3U);
            const uint32_t s1 = rotr(w[i -  2U], 17U) ^ rotr(w[i -  2U], 19U) ^ (w[i -  2U] >> 10U);
            w[i] = w[i - 16U] + s0 + w[i - 7U] + s1;
        }

        uint32_t a = state_[0], b = state_[1], c = state_[2], d = state_[3];
        uint32_t e = state_[4], f = state_[5], g = state_[6], h = state_[7];

        for (unsigned i = 0U; i < 64U; ++i) {
            const uint32_t s1  = rotr(e, 6U) ^ rotr(e, 11U) ^ rotr(e, 25U);
            const uint32_t ch  = (e & f) ^ (~e & g);
            const uint32_t t1  = h + s1 + ch + kK[i] + w[i];
            const uint32_t s0  = rotr(a, 2U) ^ rotr(a, 13U) ^ rotr(a, 22U);
            const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            const uint32_t t2  = s0 + maj;

            h = g; g = f; f = e; e = d + t1;
            d = c; c = b; b = a; a = t1 + t2;
        }

        state_[0] += a; state_[1] += b; state_[2] += c; state_[3] += d;
        state_[4] += e; state_[5] += f; state_[6] += g; state_[7] += h;
    }

    uint32_t state_[8];
    uint64_t bitlen_;
    uint8_t  buffer_[64];
    size_t   buflen_;
};

// ─────────────────────────────────────────────────────────────────────────────
// 3. Checked Bounds & Arithmetic Helpers
// ─────────────────────────────────────────────────────────────────────────────

inline bool check_bounds(uint32_t offset, uint32_t length, uint32_t max_size) noexcept {
    if (offset > max_size) return false;
    if (length > max_size - offset) return false;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// 4. Native OS Memory-Mapped / Buffered Skill Reader
// ─────────────────────────────────────────────────────────────────────────────

class BinarySkillReader {
public:
    BinarySkillReader() = default;
    ~BinarySkillReader() { close(); }

    BinarySkillReader(const BinarySkillReader&) = delete;
    BinarySkillReader& operator=(const BinarySkillReader&) = delete;

    BinarySkillReader(BinarySkillReader&& other) noexcept {
        move_from(std::move(other));
    }

    BinarySkillReader& operator=(BinarySkillReader&& other) noexcept {
        if (this != &other) {
            close();
            move_from(std::move(other));
        }
        return *this;
    }

    void close() {
#ifdef _WIN32
        if (mapped_data_ && is_mmap_) {
            UnmapViewOfFile(mapped_data_);
        }
        if (map_handle_) {
            CloseHandle(map_handle_);
            map_handle_ = NULL;
        }
        if (file_handle_ != INVALID_HANDLE_VALUE) {
            CloseHandle(file_handle_);
            file_handle_ = INVALID_HANDLE_VALUE;
        }
#else
        if (mapped_data_ && is_mmap_) {
            ::munmap(const_cast<uint8_t*>(mapped_data_), mapped_size_);
        }
        if (file_fd_ >= 0) {
            ::close(file_fd_);
            file_fd_ = -1;
        }
#endif
        mapped_data_ = nullptr;
        mapped_size_ = 0;
        is_mmap_ = false;
        fallback_buffer_.clear();
    }

    bool open(const std::string& filepath, bool prefer_mmap = true) {
        close();

        if (prefer_mmap) {
            if (open_mmap(filepath)) {
                if (validate_layout()) return true;
                close();
            }
        }

        // Fallback to buffered read
        if (open_buffered(filepath)) {
            return validate_layout();
        }

        close();
        return false;
    }

    bool is_valid() const noexcept { return mapped_data_ != nullptr && mapped_size_ >= sizeof(BinaryHeader); }
    bool is_memory_mapped() const noexcept { return is_mmap_; }
    size_t file_size() const noexcept { return mapped_size_; }

    const BinaryHeader* header() const noexcept {
        return reinterpret_cast<const BinaryHeader*>(mapped_data_);
    }

    // Direct O(1) indexed record access
    size_t count() const noexcept {
        return is_valid() ? header()->skill_count : 0;
    }

    const SkillRecord& record(size_t index) const {
        if (index >= count()) {
            throw std::out_of_range("Skill record index out of range");
        }
        const uint8_t* ptr = mapped_data_ + sizeof(BinaryHeader) + index * sizeof(SkillRecord);
        return *reinterpret_cast<const SkillRecord*>(ptr);
    }

    std::string_view name(size_t index) const {
        const auto& rec = record(index);
        size_t len = 0;
        while (len < sizeof(rec.name) && rec.name[len] != '\0') {
            len++;
        }
        return std::string_view(rec.name, len);
    }

    uint16_t category(size_t index) const {
        return record(index).category_id;
    }

    const char* category_name(size_t index) const {
        return category_to_string(category(index));
    }

    uint16_t flags(size_t index) const {
        return record(index).flags;
    }

    std::string_view description(size_t index) const {
        const auto& rec = record(index);
        const auto* hdr = header();
        if (!check_bounds(rec.desc_offset, rec.desc_len, hdr->string_table_bytes)) {
            return {};
        }
        const char* pool = reinterpret_cast<const char*>(mapped_data_ + hdr->string_table_offset);
        return std::string_view(pool + rec.desc_offset, rec.desc_len);
    }

    std::string_view body(size_t index) const {
        const auto& rec = record(index);
        const auto* hdr = header();
        if (!check_bounds(rec.body_offset, rec.body_len, hdr->string_table_bytes)) {
            return {};
        }
        const char* pool = reinterpret_cast<const char*>(mapped_data_ + hdr->string_table_offset);
        return std::string_view(pool + rec.body_offset, rec.body_len);
    }

    // Stored canonical SHA-256
    std::string stored_sha256_hex(size_t index) const {
        const auto& rec = record(index);
        static const char* const hexd = "0123456789abcdef";
        std::string s;
        s.reserve(64);
        for (unsigned i = 0U; i < 32U; ++i) {
            s.push_back(hexd[(rec.sha256[i] >> 4U) & 0x0FU]);
            s.push_back(hexd[ rec.sha256[i]        & 0x0FU]);
        }
        return s;
    }

    // Computed SHA-256 over the stored description and body bytes
    std::string compute_payload_sha256_hex(size_t index) const {
        Sha256 h;
        std::string_view d = description(index);
        std::string_view b = body(index);
        h.update(d.data(), d.size());
        h.update(b.data(), b.size());
        return h.finalize_hex();
    }

    // Whole-file SHA-256 verification
    std::string compute_file_sha256_hex() const {
        if (!is_valid()) return {};
        return Sha256::hash_hex(mapped_data_, mapped_size_);
    }

    // Linear O(N) query methods
    int find_by_name(std::string_view query) const {
        for (size_t i = 0; i < count(); ++i) {
            if (name(i) == query) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    std::vector<size_t> find_by_category(uint16_t cat_id) const {
        std::vector<size_t> indices;
        for (size_t i = 0; i < count(); ++i) {
            if (category(i) == cat_id) {
                indices.push_back(i);
            }
        }
        return indices;
    }

    std::vector<size_t> search_keyword(std::string_view query) const {
        std::vector<size_t> matches;
        if (query.empty()) return matches;

        std::string q_lower;
        q_lower.reserve(query.size());
        for (char c : query) q_lower += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));

        auto contains_ci = [&](std::string_view text) {
            if (text.size() < q_lower.size()) return false;
            for (size_t i = 0; i + q_lower.size() <= text.size(); ++i) {
                bool match = true;
                for (size_t j = 0; j < q_lower.size(); ++j) {
                    if (std::tolower(static_cast<unsigned char>(text[i + j])) != q_lower[j]) {
                        match = false;
                        break;
                    }
                }
                if (match) return true;
            }
            return false;
        };

        for (size_t i = 0; i < count(); ++i) {
            if (contains_ci(name(i)) || contains_ci(description(i)) || contains_ci(body(i))) {
                matches.push_back(i);
            }
        }
        return matches;
    }

    // Thorough structural and arithmetic integrity verification
    bool verify_all(std::string* error_msg = nullptr) const {
        if (!is_valid()) {
            if (error_msg) *error_msg = "File is not open or below minimum header size";
            return false;
        }

        const auto* hdr = header();
        if (std::memcmp(hdr->magic, "VSKB", 4) != 0) {
            if (error_msg) *error_msg = "Invalid magic bytes (expected 'VSKB')";
            return false;
        }
        if (hdr->version != 1) {
            if (error_msg) *error_msg = "Unsupported version: " + std::to_string(hdr->version);
            return false;
        }

        // Use uint64_t throughout to prevent 32-bit overflow on large files.
        // Explicit casts of mapped_size_ (size_t) to uint64_t for portability.
        const uint64_t mapped_size_u64 = static_cast<uint64_t>(mapped_size_);

        const uint64_t records_end = static_cast<uint64_t>(sizeof(BinaryHeader)) +
                               static_cast<uint64_t>(hdr->skill_count) * sizeof(SkillRecord);
        if (records_end > mapped_size_u64) {
            if (error_msg) *error_msg = "Record table exceeds mapped file size";
            return false;
        }

        if (static_cast<uint64_t>(hdr->string_table_offset) < records_end) {
            if (error_msg) *error_msg = "String table overlaps with record table";
            return false;
        }

        const uint64_t string_table_end = static_cast<uint64_t>(hdr->string_table_offset) +
                                    static_cast<uint64_t>(hdr->string_table_bytes);
        if (string_table_end > mapped_size_u64) {
            if (error_msg) *error_msg = "String table exceeds mapped file size";
            return false;
        }

        for (size_t i = 0; i < hdr->skill_count; ++i) {
            const auto& rec = record(i);
            if (!check_bounds(rec.desc_offset, rec.desc_len, hdr->string_table_bytes)) {
                if (error_msg) *error_msg = "Record #" + std::to_string(i) + " description bounds overflow";
                return false;
            }
            if (!check_bounds(rec.body_offset, rec.body_len, hdr->string_table_bytes)) {
                if (error_msg) *error_msg = "Record #" + std::to_string(i) + " body bounds overflow";
                return false;
            }
            if (name(i).empty()) {
                if (error_msg) *error_msg = "Record #" + std::to_string(i) + " has empty name";
                return false;
            }
        }

        return true;
    }

private:
    bool open_mmap(const std::string& filepath) {
#ifdef _WIN32
        HANDLE fh = CreateFileA(filepath.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL,
                                OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (fh == INVALID_HANDLE_VALUE) return false;

        LARGE_INTEGER sz;
        if (!GetFileSizeEx(fh, &sz) || sz.QuadPart < static_cast<LONGLONG>(sizeof(BinaryHeader))) {
            CloseHandle(fh);
            return false;
        }

        HANDLE mh = CreateFileMappingA(fh, NULL, PAGE_READONLY, 0, 0, NULL);
        if (mh == NULL) {
            CloseHandle(fh);
            return false;
        }

        void* view = MapViewOfFile(mh, FILE_MAP_READ, 0, 0, 0);
        if (view == NULL) {
            CloseHandle(mh);
            CloseHandle(fh);
            return false;
        }

        file_handle_ = fh;
        map_handle_ = mh;
        mapped_data_ = static_cast<const uint8_t*>(view);
        mapped_size_ = static_cast<size_t>(sz.QuadPart);
        is_mmap_ = true;
        return true;
#else
        int fd = ::open(filepath.c_str(), O_RDONLY);
        if (fd < 0) return false;

        struct stat st;
        if (::fstat(fd, &st) != 0 || st.st_size < static_cast<off_t>(sizeof(BinaryHeader))) {
            ::close(fd);
            return false;
        }

        void* view = ::mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
        if (view == MAP_FAILED) {
            ::close(fd);
            return false;
        }

        file_fd_ = fd;
        mapped_data_ = static_cast<const uint8_t*>(view);
        mapped_size_ = static_cast<size_t>(st.st_size);
        is_mmap_ = true;
        return true;
#endif
    }

    bool open_buffered(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) return false;

        std::streamsize sz = file.tellg();
        if (sz < static_cast<std::streamsize>(sizeof(BinaryHeader))) return false;

        file.seekg(0, std::ios::beg);
        fallback_buffer_.resize(static_cast<size_t>(sz));
        if (!file.read(reinterpret_cast<char*>(fallback_buffer_.data()), sz)) {
            fallback_buffer_.clear();
            return false;
        }

        mapped_data_ = fallback_buffer_.data();
        mapped_size_ = fallback_buffer_.size();
        is_mmap_ = false;
        return true;
    }

    bool validate_layout() {
        return verify_all(nullptr);
    }

    void move_from(BinarySkillReader&& other) noexcept {
#ifdef _WIN32
        file_handle_ = other.file_handle_;
        map_handle_ = other.map_handle_;
        other.file_handle_ = INVALID_HANDLE_VALUE;
        other.map_handle_ = NULL;
#else
        file_fd_ = other.file_fd_;
        other.file_fd_ = -1;
#endif
        mapped_data_ = other.mapped_data_;
        mapped_size_ = other.mapped_size_;
        is_mmap_ = other.is_mmap_;
        fallback_buffer_ = std::move(other.fallback_buffer_);

        other.mapped_data_ = nullptr;
        other.mapped_size_ = 0;
        other.is_mmap_ = false;
    }

#ifdef _WIN32
    HANDLE file_handle_ = INVALID_HANDLE_VALUE;
    HANDLE map_handle_ = NULL;
#else
    int file_fd_ = -1;
#endif
    const uint8_t* mapped_data_ = nullptr;
    size_t mapped_size_ = 0;
    bool is_mmap_ = false;
    std::vector<uint8_t> fallback_buffer_;
};

// ─────────────────────────────────────────────────────────────────────────────
// 5. C++ Binary Compiler / Serializer
// ─────────────────────────────────────────────────────────────────────────────

class BinarySkillCompiler {
public:
    struct Entry {
        std::string name;
        uint16_t    category_id;
        uint16_t    flags;
        std::string description;
        std::string body;
        uint8_t     sha256[32];
    };

    void add_skill(std::string_view name, uint16_t cat, uint16_t flags,
                   std::string_view desc, std::string_view body, const uint8_t sha[32]) {
        Entry e;
        e.name = std::string(name);
        e.category_id = cat;
        e.flags = flags;
        e.description = std::string(desc);
        e.body = std::string(body);
        std::memcpy(e.sha256, sha, 32);
        entries_.push_back(std::move(e));
    }

    bool compile_to_file(const std::string& out_path) const {
        BinaryHeader hdr;
        std::memcpy(hdr.magic, "VSKB", 4);
        hdr.version = 1;
        hdr.reserved = 0;
        hdr.skill_count = static_cast<uint32_t>(entries_.size());
        hdr.string_table_offset = sizeof(BinaryHeader) + hdr.skill_count * sizeof(SkillRecord);

        std::vector<uint8_t> string_pool;
        std::vector<SkillRecord> records;
        records.reserve(entries_.size());

        for (const auto& e : entries_) {
            SkillRecord r;
            std::memset(&r, 0, sizeof(SkillRecord));
            size_t n_len = std::min(e.name.size(), sizeof(r.name) - 1);
            std::memcpy(r.name, e.name.data(), n_len);
            r.name[n_len] = '\0';

            r.category_id = e.category_id;
            r.flags = e.flags;

            r.desc_offset = static_cast<uint32_t>(string_pool.size());
            r.desc_len = static_cast<uint32_t>(e.description.size());
            string_pool.insert(string_pool.end(), e.description.begin(), e.description.end());

            r.body_offset = static_cast<uint32_t>(string_pool.size());
            r.body_len = static_cast<uint32_t>(e.body.size());
            string_pool.insert(string_pool.end(), e.body.begin(), e.body.end());

            std::memcpy(r.sha256, e.sha256, 32);
            records.push_back(r);
        }

        hdr.string_table_bytes = static_cast<uint32_t>(string_pool.size());

        std::ofstream out(out_path, std::ios::binary);
        if (!out.is_open()) return false;

        out.write(reinterpret_cast<const char*>(&hdr), sizeof(BinaryHeader));
        out.write(reinterpret_cast<const char*>(records.data()), records.size() * sizeof(SkillRecord));
        out.write(reinterpret_cast<const char*>(string_pool.data()), string_pool.size());

        return out.good();
    }

private:
    std::vector<Entry> entries_;
};

} // namespace bnn
