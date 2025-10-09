#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <cstring>
#include <cstdint>
#include <regex>

namespace fs = std::filesystem;

struct MD5Context {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
};

class MD5 {
public:
    static std::string hash(const std::string& input) {
        MD5Context ctx;
        init(ctx);
        update(ctx, reinterpret_cast<const uint8_t*>(input.c_str()), input.length());
        uint8_t digest[16];
        finalize(ctx, digest);
        
        std::ostringstream oss;
        for (int i = 0; i < 16; ++i) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
        }
        return oss.str();
    }

private:
    static void init(MD5Context& ctx) {
        ctx.count[0] = ctx.count[1] = 0;
        ctx.state[0] = 0x67452301;
        ctx.state[1] = 0xefcdab89;
        ctx.state[2] = 0x98badcfe;
        ctx.state[3] = 0x10325476;
    }

    static void transform(uint32_t state[4], const uint8_t block[64]) {
        uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
        uint32_t x[16];
        
        for (int i = 0, j = 0; i < 16; ++i, j += 4) {
            x[i] = static_cast<uint32_t>(block[j]) |
                   (static_cast<uint32_t>(block[j + 1]) << 8) |
                   (static_cast<uint32_t>(block[j + 2]) << 16) |
                   (static_cast<uint32_t>(block[j + 3]) << 24);
        }

        auto FF = [](uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
            a += ((b & c) | (~b & d)) + x + ac;
            a = ((a << s) | (a >> (32 - s))) + b;
            return a;
        };
        
        auto GG = [](uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
            a += ((b & d) | (c & ~d)) + x + ac;
            a = ((a << s) | (a >> (32 - s))) + b;
            return a;
        };
        
        auto HH = [](uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
            a += (b ^ c ^ d) + x + ac;
            a = ((a << s) | (a >> (32 - s))) + b;
            return a;
        };
        
        auto II = [](uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t x, uint32_t s, uint32_t ac) {
            a += (c ^ (b | ~d)) + x + ac;
            a = ((a << s) | (a >> (32 - s))) + b;
            return a;
        };

        a = FF(a, b, c, d, x[0], 7, 0xd76aa478);
        d = FF(d, a, b, c, x[1], 12, 0xe8c7b756);
        c = FF(c, d, a, b, x[2], 17, 0x242070db);
        b = FF(b, c, d, a, x[3], 22, 0xc1bdceee);
        a = FF(a, b, c, d, x[4], 7, 0xf57c0faf);
        d = FF(d, a, b, c, x[5], 12, 0x4787c62a);
        c = FF(c, d, a, b, x[6], 17, 0xa8304613);
        b = FF(b, c, d, a, x[7], 22, 0xfd469501);
        a = FF(a, b, c, d, x[8], 7, 0x698098d8);
        d = FF(d, a, b, c, x[9], 12, 0x8b44f7af);
        c = FF(c, d, a, b, x[10], 17, 0xffff5bb1);
        b = FF(b, c, d, a, x[11], 22, 0x895cd7be);
        a = FF(a, b, c, d, x[12], 7, 0x6b901122);
        d = FF(d, a, b, c, x[13], 12, 0xfd987193);
        c = FF(c, d, a, b, x[14], 17, 0xa679438e);
        b = FF(b, c, d, a, x[15], 22, 0x49b40821);

        a = GG(a, b, c, d, x[1], 5, 0xf61e2562);
        d = GG(d, a, b, c, x[6], 9, 0xc040b340);
        c = GG(c, d, a, b, x[11], 14, 0x265e5a51);
        b = GG(b, c, d, a, x[0], 20, 0xe9b6c7aa);
        a = GG(a, b, c, d, x[5], 5, 0xd62f105d);
        d = GG(d, a, b, c, x[10], 9, 0x02441453);
        c = GG(c, d, a, b, x[15], 14, 0xd8a1e681);
        b = GG(b, c, d, a, x[4], 20, 0xe7d3fbc8);
        a = GG(a, b, c, d, x[9], 5, 0x21e1cde6);
        d = GG(d, a, b, c, x[14], 9, 0xc33707d6);
        c = GG(c, d, a, b, x[3], 14, 0xf4d50d87);
        b = GG(b, c, d, a, x[8], 20, 0x455a14ed);
        a = GG(a, b, c, d, x[13], 5, 0xa9e3e905);
        d = GG(d, a, b, c, x[2], 9, 0xfcefa3f8);
        c = GG(c, d, a, b, x[7], 14, 0x676f02d9);
        b = GG(b, c, d, a, x[12], 20, 0x8d2a4c8a);

        a = HH(a, b, c, d, x[5], 4, 0xfffa3942);
        d = HH(d, a, b, c, x[8], 11, 0x8771f681);
        c = HH(c, d, a, b, x[11], 16, 0x6d9d6122);
        b = HH(b, c, d, a, x[14], 23, 0xfde5380c);
        a = HH(a, b, c, d, x[1], 4, 0xa4beea44);
        d = HH(d, a, b, c, x[4], 11, 0x4bdecfa9);
        c = HH(c, d, a, b, x[7], 16, 0xf6bb4b60);
        b = HH(b, c, d, a, x[10], 23, 0xbebfbc70);
        a = HH(a, b, c, d, x[13], 4, 0x289b7ec6);
        d = HH(d, a, b, c, x[0], 11, 0xeaa127fa);
        c = HH(c, d, a, b, x[3], 16, 0xd4ef3085);
        b = HH(b, c, d, a, x[6], 23, 0x04881d05);
        a = HH(a, b, c, d, x[9], 4, 0xd9d4d039);
        d = HH(d, a, b, c, x[12], 11, 0xe6db99e5);
        c = HH(c, d, a, b, x[15], 16, 0x1fa27cf8);
        b = HH(b, c, d, a, x[2], 23, 0xc4ac5665);

        a = II(a, b, c, d, x[0], 6, 0xf4292244);
        d = II(d, a, b, c, x[7], 10, 0x432aff97);
        c = II(c, d, a, b, x[14], 15, 0xab9423a7);
        b = II(b, c, d, a, x[5], 21, 0xfc93a039);
        a = II(a, b, c, d, x[12], 6, 0x655b59c3);
        d = II(d, a, b, c, x[3], 10, 0x8f0ccc92);
        c = II(c, d, a, b, x[10], 15, 0xffeff47d);
        b = II(b, c, d, a, x[1], 21, 0x85845dd1);
        a = II(a, b, c, d, x[8], 6, 0x6fa87e4f);
        d = II(d, a, b, c, x[15], 10, 0xfe2ce6e0);
        c = II(c, d, a, b, x[6], 15, 0xa3014314);
        b = II(b, c, d, a, x[13], 21, 0x4e0811a1);
        a = II(a, b, c, d, x[4], 6, 0xf7537e82);
        d = II(d, a, b, c, x[11], 10, 0xbd3af235);
        c = II(c, d, a, b, x[2], 15, 0x2ad7d2bb);
        b = II(b, c, d, a, x[9], 21, 0xeb86d391);

        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
    }

    static void update(MD5Context& ctx, const uint8_t* input, size_t length) {
        size_t index = (ctx.count[0] >> 3) & 0x3F;
        ctx.count[0] += static_cast<uint32_t>(length << 3);
        if (ctx.count[0] < (length << 3)) {
            ctx.count[1]++;
        }
        ctx.count[1] += static_cast<uint32_t>(length >> 29);
        
        size_t partLen = 64 - index;
        size_t i = 0;
        
        if (length >= partLen) {
            std::memcpy(&ctx.buffer[index], input, partLen);
            transform(ctx.state, ctx.buffer);
            
            for (i = partLen; i + 63 < length; i += 64) {
                transform(ctx.state, &input[i]);
            }
            index = 0;
        }
        
        std::memcpy(&ctx.buffer[index], &input[i], length - i);
    }

    static void finalize(MD5Context& ctx, uint8_t digest[16]) {
        uint8_t bits[8];
        for (int i = 0; i < 8; ++i) {
            bits[i] = static_cast<uint8_t>((ctx.count[i >> 2] >> ((i & 3) << 3)) & 0xFF);
        }
        
        size_t index = (ctx.count[0] >> 3) & 0x3F;
        size_t padLen = (index < 56) ? (56 - index) : (120 - index);
        uint8_t padding[64];
        padding[0] = 0x80;
        std::memset(padding + 1, 0, padLen - 1);
        
        update(ctx, padding, padLen);
        update(ctx, bits, 8);
        
        for (int i = 0; i < 16; ++i) {
            digest[i] = static_cast<uint8_t>((ctx.state[i >> 2] >> ((i & 3) << 3)) & 0xFF);
        }
    }
};

struct ResourceInfo {
    fs::path filePath;
    std::string normalizedPath;
    std::string identifier;
    float designScale;
    size_t fileSize;
};

class ResourceGenerator {
public:
    ResourceGenerator(const std::string& inputDir, const std::string& outputDir,
                     const std::string& nameSpace, const std::string& headerFile,
                     const std::string& sourceFile)
        : inputDir_(inputDir), outputDir_(outputDir), nameSpace_(nameSpace),
          headerFile_(headerFile), sourceFile_(sourceFile) {}

    bool generate() {
        if (!fs::exists(inputDir_)) {
            std::cerr << "Error: Input directory does not exist: " << inputDir_ << std::endl;
            return false;
        }

        fs::create_directories(outputDir_);

        if (!collectResources()) {
            return false;
        }

        if (resources_.empty()) {
            std::cerr << "Warning: No resources found in " << inputDir_ << std::endl;
        }

        if (!generateHeader()) {
            return false;
        }

        if (!generateSource()) {
            return false;
        }

        std::cout << "Generated " << resources_.size() << " embedded resources" << std::endl;
        return true;
    }

private:
    std::string inputDir_;
    std::string outputDir_;
    std::string nameSpace_;
    std::string headerFile_;
    std::string sourceFile_;
    std::vector<ResourceInfo> resources_;

    float parseDesignScale(const std::string& filename) {
        std::regex pattern(R"(@(\d+)x\.(png|jpg|jpeg|bmp))", std::regex_constants::icase);
        std::smatch match;
        if (std::regex_search(filename, match, pattern)) {
            return std::stof(match[1].str());
        }
        return 1.0f;
    }

    std::string sanitizeIdentifier(const std::string& path) {
        fs::path p(path);
        std::string stem = p.stem().string();
        std::string hashSuffix = MD5::hash(path).substr(0, 8);
        
        std::string sanitized;
        for (char c : stem) {
            if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
                sanitized += c;
            } else {
                sanitized += '_';
            }
        }
        
        if (sanitized.empty() || !std::isalpha(static_cast<unsigned char>(sanitized[0]))) {
            sanitized = "res_" + sanitized;
        }
        
        return sanitized + "_" + hashSuffix;
    }

    std::string normalizePath(const fs::path& path) {
        std::string pathStr = path.generic_string();
        std::replace(pathStr.begin(), pathStr.end(), '\\', '/');
        return pathStr;
    }

    bool collectResources() {
        std::map<std::string, int> identifierCounts;
        
        for (const auto& entry : fs::recursive_directory_iterator(inputDir_)) {
            if (!entry.is_regular_file()) {
                continue;
            }
            
            const fs::path& filePath = entry.path();
            std::string filename = filePath.filename().string();
            
            if (filename[0] == '.' || filename == ".DS_Store" || filename == ".gitkeep") {
                continue;
            }
            
            fs::path relativePath = fs::relative(filePath, inputDir_);
            std::string normalizedPath = normalizePath(relativePath);
            
            std::string baseIdentifier = sanitizeIdentifier(normalizedPath);
            std::string identifier = baseIdentifier;
            
            int counter = 1;
            while (identifierCounts.find(identifier) != identifierCounts.end()) {
                identifier = baseIdentifier + "_" + std::to_string(counter);
                counter++;
            }
            identifierCounts[identifier] = 1;
            
            ResourceInfo info;
            info.filePath = filePath;
            info.normalizedPath = normalizedPath;
            info.identifier = identifier;
            info.designScale = parseDesignScale(filename);
            info.fileSize = fs::file_size(filePath);
            
            resources_.push_back(info);
        }
        
        return true;
    }

    bool generateHeader() {
        fs::path headerPath = fs::path(outputDir_) / headerFile_;
        std::ofstream out(headerPath);
        
        if (!out) {
            std::cerr << "Error: Cannot create header file: " << headerPath << std::endl;
            return false;
        }

        out << "#pragma once\n\n";
        out << "#include <cstddef>\n";
        out << "#include <string_view>\n\n";
        out << "namespace " << nameSpace_ << " {\n\n";
        out << "struct ResourceData {\n";
        out << "    const unsigned char* data;\n";
        out << "    size_t size;\n";
        out << "    std::string_view path;\n";
        out << "    float designScale;\n";
        out << "};\n\n";

        for (const auto& res : resources_) {
            out << "extern const ResourceData " << res.identifier << ";\n";
        }

        out << "\nconst ResourceData* findResource(std::string_view path);\n";
        out << "const ResourceData* getAllResources();\n";
        out << "size_t getResourceCount();\n\n";
        out << "} // namespace " << nameSpace_ << "\n";

        return true;
    }

    bool generateSource() {
        fs::path sourcePath = fs::path(outputDir_) / sourceFile_;
        std::ofstream out(sourcePath);
        
        if (!out) {
            std::cerr << "Error: Cannot create source file: " << sourcePath << std::endl;
            return false;
        }

        out << "#include \"" << headerFile_ << "\"\n";
        out << "#include <array>\n";
        out << "#include <string_view>\n";
        out << "#include <algorithm>\n\n";
        out << "namespace " << nameSpace_ << " {\n\n";

        for (const auto& res : resources_) {
            std::ifstream file(res.filePath, std::ios::binary);
            if (!file) {
                std::cerr << "Error: Cannot read file: " << res.filePath << std::endl;
                return false;
            }

            std::vector<uint8_t> data((std::istreambuf_iterator<char>(file)),
                                     std::istreambuf_iterator<char>());

            out << "static const unsigned char " << res.identifier << "_data[] = {\n    ";
            
            for (size_t i = 0; i < data.size(); ++i) {
                out << "0x" << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(data[i]);
                
                if (i < data.size() - 1) {
                    out << ", ";
                    if ((i + 1) % 16 == 0) {
                        out << "\n    ";
                    }
                }
            }
            
            out << std::dec << "\n};\n\n";

            out << "const ResourceData " << res.identifier << " = {\n";
            out << "    " << res.identifier << "_data,\n";
            out << "    " << data.size() << ",\n";
            out << "    \"" << res.normalizedPath << "\",\n";
            out << "    " << std::fixed << std::setprecision(1) << res.designScale << "f\n";
            out << "};\n\n";
        }

        out << "static const std::array<ResourceData, " << resources_.size() << "> all_resources = {{\n";
        for (const auto& res : resources_) {
            out << "    " << res.identifier << ",\n";
        }
        out << "}};\n\n";

        out << "const ResourceData* findResource(std::string_view path) {\n";
        out << "    auto it = std::find_if(all_resources.begin(), all_resources.end(),\n";
        out << "        [path](const ResourceData& res) { return res.path == path; });\n";
        out << "    return (it != all_resources.end()) ? &(*it) : nullptr;\n";
        out << "}\n\n";

        out << "const ResourceData* getAllResources() {\n";
        out << "    return all_resources.data();\n";
        out << "}\n\n";

        out << "size_t getResourceCount() {\n";
        out << "    return all_resources.size();\n";
        out << "}\n\n";

        out << "} // namespace " << nameSpace_ << "\n";

        return true;
    }
};

void printUsage(const char* programName) {
    std::cerr << "Usage: " << programName << " [options]\n"
              << "Options:\n"
              << "  --input-dir <path>      Input resources directory (required)\n"
              << "  --output-dir <path>     Output directory for generated files (required)\n"
              << "  --namespace <name>      C++ namespace for resources (default: Resources)\n"
              << "  --header-file <name>    Header file name (default: embedded_resources.h)\n"
              << "  --source-file <name>    Source file name (default: embedded_resources.cpp)\n"
              << "  --help                  Show this help message\n";
}

int main(int argc, char* argv[]) {
    std::string inputDir;
    std::string outputDir;
    std::string nameSpace = "Resources";
    std::string headerFile = "embedded_resources.h";
    std::string sourceFile = "embedded_resources.cpp";

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "--help") {
            printUsage(argv[0]);
            return 0;
        } else if (arg == "--input-dir") {
            if (i + 1 < argc) {
                inputDir = argv[++i];
            } else {
                std::cerr << "Error: --input-dir requires a value\n";
                return 1;
            }
        } else if (arg == "--output-dir") {
            if (i + 1 < argc) {
                outputDir = argv[++i];
            } else {
                std::cerr << "Error: --output-dir requires a value\n";
                return 1;
            }
        } else if (arg == "--namespace") {
            if (i + 1 < argc) {
                nameSpace = argv[++i];
            } else {
                std::cerr << "Error: --namespace requires a value\n";
                return 1;
            }
        } else if (arg == "--header-file") {
            if (i + 1 < argc) {
                headerFile = argv[++i];
            } else {
                std::cerr << "Error: --header-file requires a value\n";
                return 1;
            }
        } else if (arg == "--source-file") {
            if (i + 1 < argc) {
                sourceFile = argv[++i];
            } else {
                std::cerr << "Error: --source-file requires a value\n";
                return 1;
            }
        } else {
            std::cerr << "Error: Unknown argument: " << arg << "\n";
            printUsage(argv[0]);
            return 1;
        }
    }

    if (inputDir.empty() || outputDir.empty()) {
        std::cerr << "Error: --input-dir and --output-dir are required\n";
        printUsage(argv[0]);
        return 1;
    }

    ResourceGenerator generator(inputDir, outputDir, nameSpace, headerFile, sourceFile);
    
    if (!generator.generate()) {
        return 1;
    }

    return 0;
}
