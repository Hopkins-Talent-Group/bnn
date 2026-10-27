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

namespace bnn {

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
    uint8_t  sha256[32];          // Cryptographic SHA-256 of source definition
    uint8_t  _pad[44];            // Alignment padding
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

class BinarySkillReader {
public:
    BinarySkillReader() = default;

    bool open(const std::string& filepath) {
        std::ifstream file(filepath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return false;
        }

        std::streamsize size = file.tellg();
        if (size < static_cast<std::streamsize>(sizeof(BinaryHeader))) {
            return false;
        }

        file.seekg(0, std::ios::beg);
        buffer_.resize(static_cast<size_t>(size));
        if (!file.read(reinterpret_cast<char*>(buffer_.data()), size)) {
            buffer_.clear();
            return false;
        }

        const auto* hdr = header();
        if (std::memcmp(hdr->magic, "VSKB", 4) != 0 || hdr->version != 1) {
            buffer_.clear();
            return false;
        }

        size_t expected_min = sizeof(BinaryHeader) + hdr->skill_count * sizeof(SkillRecord);
        if (buffer_.size() < expected_min || buffer_.size() < (hdr->string_table_offset + hdr->string_table_bytes)) {
            buffer_.clear();
            return false;
        }

        return true;
    }

    bool is_valid() const {
        return !buffer_.empty();
    }

    const BinaryHeader* header() const {
        return reinterpret_cast<const BinaryHeader*>(buffer_.data());
    }

    size_t count() const {
        return is_valid() ? header()->skill_count : 0;
    }

    const SkillRecord& record(size_t index) const {
        if (index >= count()) {
            throw std::out_of_range("Skill index out of range");
        }
        const uint8_t* ptr = buffer_.data() + sizeof(BinaryHeader) + index * sizeof(SkillRecord);
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
        if (rec.desc_offset + rec.desc_len > hdr->string_table_bytes) {
            return {};
        }
        const char* str_pool = reinterpret_cast<const char*>(buffer_.data() + hdr->string_table_offset);
        return std::string_view(str_pool + rec.desc_offset, rec.desc_len);
    }

    std::string_view body(size_t index) const {
        const auto& rec = record(index);
        const auto* hdr = header();
        if (rec.body_offset + rec.body_len > hdr->string_table_bytes) {
            return {};
        }
        const char* str_pool = reinterpret_cast<const char*>(buffer_.data() + hdr->string_table_offset);
        return std::string_view(str_pool + rec.body_offset, rec.body_len);
    }

    std::string sha256_hex(size_t index) const {
        const auto& rec = record(index);
        std::ostringstream ss;
        for (size_t i = 0; i < 32; ++i) {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(rec.sha256[i]);
        }
        return ss.str();
    }

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

        auto contains_case_insensitive = [&](std::string_view text) {
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
            if (contains_case_insensitive(name(i)) ||
                contains_case_insensitive(description(i)) ||
                contains_case_insensitive(body(i))) {
                matches.push_back(i);
            }
        }
        return matches;
    }

private:
    std::vector<uint8_t> buffer_;
};

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
