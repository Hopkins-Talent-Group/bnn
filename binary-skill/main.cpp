#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include "binary_skill.hpp"

void print_usage(const char* prog) {
    std::cout << "BNN Binary-Skill CLI & Runtime (C++20 Zero-FPU Knowledge Engine)\n"
              << "Usage: " << prog << " [binary_path] [command] [args...]\n\n"
              << "Commands:\n"
              << "  info             Display binary header and allocation metadata\n"
              << "  list             List all distilled skills in tabular layout\n"
              << "  category <name>  List skills by category (e.g. NVIDIA_STACK)\n"
              << "  search <query>   Perform sub-millisecond keyword lookup\n"
              << "  show <name>      Inspect skill definition, SHA-256, and body\n"
              << "  verify           Verify magic, record bounds, and memory layout\n"
              << "  test-compile     Run pure C++ compiler self-test roundtrip\n";
}

int main(int argc, char* argv[]) {
    std::string bin_path = "secondbrain.bin";
    std::string command = "info";
    std::string arg = "";

    if (argc >= 2) {
        std::string first = argv[1];
        if (first == "-h" || first == "--help") {
            print_usage(argv[0]);
            return 0;
        }
        // Check if first arg is a command or file path
        if (first == "info" || first == "list" || first == "verify" || first == "test-compile" ||
            first == "search" || first == "show" || first == "category") {
            command = first;
            if (argc >= 3) arg = argv[2];
        } else {
            bin_path = first;
            if (argc >= 3) command = argv[2];
            if (argc >= 4) arg = argv[3];
        }
    }

    if (command == "test-compile") {
        std::cout << "Running pure C++ binary skill compiler self-test...\n";
        bnn::BinarySkillCompiler compiler;
        uint8_t dummy_sha[32] = {0xAB, 0xCD, 0xEF};
        compiler.add_skill("bnn-quant", bnn::CAT_NVIDIA_STACK, 0x02,
                           "Quantized INT4/BIN1 inference acceleration.",
                           "# BNN Quantization\nRuns XNOR/popcount kernels without FPU floating-point conversion.",
                           dummy_sha);
        compiler.add_skill("bnn-memory", bnn::CAT_CORE_ENGINEERING, 0x04,
                           "Zero-copy memory mapped state cache.",
                           "# BNN Memory Cache\nDirect buffer reading bypassing tokenization layers.",
                           dummy_sha);

        std::string test_bin = "test_roundtrip.bin";
        if (!compiler.compile_to_file(test_bin)) {
            std::cerr << "FAIL: compilation failed\n";
            return 1;
        }

        bnn::BinarySkillReader test_reader;
        if (!test_reader.open(test_bin)) {
            std::cerr << "FAIL: reader could not open compiled test binary\n";
            return 1;
        }

        if (test_reader.count() != 2 || test_reader.name(0) != "bnn-quant") {
            std::cerr << "FAIL: verification mismatch\n";
            return 1;
        }

        std::cout << "SUCCESS: compiled " << test_reader.count() << " skills in pure C++!\n";
        std::cout << "Record 0: " << test_reader.name(0) << " (" << test_reader.category_name(0) << ")\n";
        std::cout << "Record 1: " << test_reader.name(1) << " (" << test_reader.category_name(1) << ")\n";
        return 0;
    }

    bnn::BinarySkillReader reader;
    if (!reader.open(bin_path)) {
        // Try fallback in binary-skill/ relative directory
        bin_path = "binary-skill/" + bin_path;
        if (!reader.open(bin_path)) {
            std::cerr << "Error: Could not open valid binary skill file at '" << bin_path << "'\n";
            return 1;
        }
    }

    if (command == "info") {
        const auto* hdr = reader.header();
        std::cout << "========================================================\n"
                  << "  BNN Zero-Copy Binary Skill Knowledge Base\n"
                  << "========================================================\n"
                  << "Magic Header       : " << std::string(hdr->magic, 4) << "\n"
                  << "Format Version     : " << hdr->version << "\n"
                  << "Skills Count       : " << hdr->skill_count << "\n"
                  << "String Table Offset: " << hdr->string_table_offset << " bytes\n"
                  << "String Table Size  : " << hdr->string_table_bytes << " bytes ("
                  << (hdr->string_table_bytes / 1024) << " KB)\n"
                  << "Record Size (Fixed): " << sizeof(bnn::SkillRecord) << " bytes\n"
                  << "Status             : ZERO-FPU MEMORY-MAPPED READY\n"
                  << "========================================================\n";
        return 0;
    }

    if (command == "list") {
        std::cout << "-------------------------------------------------------------------------------------------------\n";
        std::cout << std::left << std::setw(6)  << "ID"
                               << std::setw(34) << "SKILL NAME"
                               << std::setw(24) << "CATEGORY"
                               << std::setw(12) << "FLAGS"
                               << "DESCRIPTION PREVIEW\n";
        std::cout << "-------------------------------------------------------------------------------------------------\n";
        for (size_t i = 0; i < reader.count(); ++i) {
            std::string desc = std::string(reader.description(i));
            if (desc.size() > 40) desc = desc.substr(0, 37) + "...";
            std::cout << std::left << std::setw(6)  << (i + 1)
                                   << std::setw(34) << reader.name(i)
                                   << std::setw(24) << reader.category_name(i)
                                   << "0x" << std::hex << std::setw(10) << reader.flags(i) << std::dec
                                   << desc << "\n";
        }
        std::cout << "-------------------------------------------------------------------------------------------------\n";
        std::cout << "Total: " << reader.count() << " skills distilled.\n";
        return 0;
    }

    if (command == "category") {
        if (arg.empty()) {
            std::cout << "Available categories:\n"
                      << "  1: NVIDIA_STACK\n"
                      << "  2: CORE_ENGINEERING\n"
                      << "  3: ANTI_SLOP\n"
                      << "  4: DESIGN_FRONTEND\n"
                      << "  5: DATABASE_BACKEND\n"
                      << "  6: MULTIMODAL_CREATIVE\n";
            return 0;
        }

        uint16_t cat_id = 0;
        if (arg == "NVIDIA_STACK" || arg == "1") cat_id = bnn::CAT_NVIDIA_STACK;
        else if (arg == "CORE_ENGINEERING" || arg == "2") cat_id = bnn::CAT_CORE_ENGINEERING;
        else if (arg == "ANTI_SLOP" || arg == "3") cat_id = bnn::CAT_ANTI_SLOP;
        else if (arg == "DESIGN_FRONTEND" || arg == "4") cat_id = bnn::CAT_DESIGN_FRONTEND;
        else if (arg == "DATABASE_BACKEND" || arg == "5") cat_id = bnn::CAT_DATABASE_BACKEND;
        else if (arg == "MULTIMODAL_CREATIVE" || arg == "6") cat_id = bnn::CAT_MULTIMODAL_CREATIVE;

        auto indices = reader.find_by_category(cat_id);
        std::cout << "Category " << bnn::category_to_string(cat_id) << " (" << indices.size() << " skills):\n";
        for (size_t idx : indices) {
            std::cout << "  - " << std::left << std::setw(32) << reader.name(idx)
                      << " : " << reader.description(idx) << "\n";
        }
        return 0;
    }

    if (command == "search") {
        if (arg.empty()) {
            std::cerr << "Usage: search <query>\n";
            return 1;
        }

        auto start = std::chrono::high_resolution_clock::now();
        auto matches = reader.search_keyword(arg);
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now() - start
        ).count();

        std::cout << "Search query: '" << arg << "' found " << matches.size() << " match(es) in "
                  << elapsed << " us (Zero-FPU scan):\n";
        for (size_t idx : matches) {
            std::cout << "  [" << (idx + 1) << "] " << reader.name(idx)
                      << " (" << reader.category_name(idx) << ")\n"
                      << "      " << reader.description(idx) << "\n";
        }
        return 0;
    }

    if (command == "show") {
        if (arg.empty()) {
            std::cerr << "Usage: show <skill_name>\n";
            return 1;
        }

        int idx = reader.find_by_name(arg);
        if (idx < 0) {
            std::cerr << "Skill '" << arg << "' not found.\n";
            return 1;
        }

        std::cout << "Skill Name : " << reader.name(idx) << "\n"
                  << "Category   : " << reader.category_name(idx) << " (ID: " << reader.category(idx) << ")\n"
                  << "Flags      : 0x" << std::hex << reader.flags(idx) << std::dec << "\n"
                  << "SHA-256    : " << reader.sha256_hex(idx) << "\n"
                  << "Description: " << reader.description(idx) << "\n\n"
                  << "--- Body Extract ---\n"
                  << reader.body(idx).substr(0, 1000) << "\n...\n";
        return 0;
    }

    if (command == "verify") {
        std::cout << "Verifying binary integrity...\n";
        bool ok = true;
        for (size_t i = 0; i < reader.count(); ++i) {
            std::string_view n = reader.name(i);
            std::string_view d = reader.description(i);
            std::string_view b = reader.body(i);
            if (n.empty()) {
                std::cerr << "ERROR: Skill #" << i << " has empty name\n";
                ok = false;
            }
            if (d.empty()) {
                std::cerr << "WARNING: Skill '" << n << "' has empty description\n";
            }
            if (b.empty()) {
                std::cerr << "WARNING: Skill '" << n << "' has empty body\n";
            }
        }
        if (ok) {
            std::cout << "PASS: All " << reader.count() << " records structurally valid and memory-mappable.\n";
            return 0;
        } else {
            std::cout << "FAIL: Integrity check failed.\n";
            return 1;
        }
    }

    std::cerr << "Unknown command: " << command << "\n";
    print_usage(argv[0]);
    return 1;
}
