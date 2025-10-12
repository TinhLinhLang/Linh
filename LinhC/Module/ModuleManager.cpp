#include "ModuleManager.hpp"
#include <iostream>
#include <fstream>
#include <algorithm>
#include "../Parsing/Lexer/Lexer.hpp"
#include "../Parsing/Parser/Parser.hpp"

#ifdef _WIN32
    #include <windows.h>
    #define PATH_SEPARATOR '\\'
    #define ALT_PATH_SEPARATOR '/'
#else
    #define PATH_SEPARATOR '/'
    #define ALT_PATH_SEPARATOR '\\'
#endif

namespace Linh
{
    namespace Module
    {
        ModuleManager::ModuleManager()
        {
            // Add default search paths
            m_search_paths.push_back(".");
            m_search_paths.push_back("./Li");
            m_search_paths.push_back("./Li/pkg");
            m_search_paths.push_back("./std");
        }

        ModuleManager::~ModuleManager() = default;

        std::string ModuleManager::normalize_path(const std::string& path)
        {
            std::string normalized = path;
            
            // Convert forward slashes to platform-specific separators on Windows
            #ifdef _WIN32
            std::replace(normalized.begin(), normalized.end(), '/', '\\');
            #else
            std::replace(normalized.begin(), normalized.end(), '\\', '/');
            #endif
            
            return normalized;
        }

        std::string ModuleManager::convert_path_separators(const std::string& path)
        {
            std::string converted = path;
            // Always convert / to \ on Windows for internal processing
            #ifdef _WIN32
            std::replace(converted.begin(), converted.end(), '/', '\\');
            #endif
            return converted;
        }

        bool ModuleManager::is_relative_path(const std::string& path)
        {
            if (path.empty()) return false;
            
            #ifdef _WIN32
            // Check for absolute paths on Windows (C:\, D:\, etc. or \\server\share)
            if (path.length() >= 2 && path[1] == ':') return false;
            if (path.length() >= 2 && path[0] == '\\' && path[1] == '\\') return false;
            #else
            // Check for absolute paths on Unix-like systems
            if (path[0] == '/') return false;
            #endif
            
            return true;
        }

        std::string ModuleManager::get_file_extension(const std::string& path)
        {
            size_t dot_pos = path.find_last_of('.');
            if (dot_pos == std::string::npos) return "";
            return path.substr(dot_pos);
        }

        std::string ModuleManager::find_module_file(const std::string& module_name, const std::string& current_dir)
        {
            std::vector<std::string> search_dirs;
            
            // For string imports (with quotes), always use current directory as base
            if (!current_dir.empty())
            {
                search_dirs.push_back(current_dir);
            }
            
            // Add configured search paths as fallback
            for (const auto& search_path : m_search_paths)
            {
                if (!current_dir.empty())
                {
                    // Also try search paths relative to current directory
                    std::string relative_search = current_dir;
                    if (!relative_search.empty() && relative_search.back() != PATH_SEPARATOR)
                    {
                        relative_search += PATH_SEPARATOR;
                    }
                    relative_search += search_path;
                    search_dirs.push_back(relative_search);
                }
                search_dirs.push_back(search_path);
            }
            
            // Try different extensions
            std::vector<std::string> extensions = {".li", ""};
            
            for (const auto& dir : search_dirs)
            {
                for (const auto& ext : extensions)
                {
                    std::string candidate = dir;
                    if (!candidate.empty() && candidate.back() != PATH_SEPARATOR)
                    {
                        candidate += PATH_SEPARATOR;
                    }
                    candidate += module_name + ext;
                    
                    candidate = normalize_path(candidate);
                    
                    if (std::filesystem::exists(candidate))
                    {
                        return std::filesystem::absolute(candidate).string();
                    }
                }
            }
            
            return "";
        }

        std::string ModuleManager::resolve_module_path(const std::string& module_name, const std::string& current_file_path)
        {
            std::string converted_name = convert_path_separators(module_name);
            
            // Get current directory from current file path
            std::string current_dir;
            if (!current_file_path.empty())
            {
                std::filesystem::path current_path(current_file_path);
                current_dir = current_path.parent_path().string();
            }
            
            // If it's already an absolute path, just normalize it
            if (!is_relative_path(converted_name))
            {
                std::string normalized = normalize_path(converted_name);
                if (std::filesystem::exists(normalized))
                {
                    return std::filesystem::absolute(normalized).string();
                }
                return "";
            }
            
            // For relative paths, search in directories
            return find_module_file(converted_name, current_dir);
        }

        bool ModuleManager::load_module(const std::string& module_name, const std::string& current_file_path)
        {
            // Check if already loaded
            if (is_module_loaded(module_name))
            {
                return true;
            }
            
            // Check for circular dependency
            if (m_loading_stack.find(module_name) != m_loading_stack.end())
            {
                std::cerr << "Circular dependency detected for module: " << module_name << std::endl;
                return false;
            }
            
            // Resolve module path
            std::string resolved_path = resolve_module_path(module_name, current_file_path);
            if (resolved_path.empty())
            {
                std::cerr << "Module not found: " << module_name << std::endl;
                return false;
            }
            
            // Create module info
            auto module_info = std::make_unique<ModuleInfo>();
            module_info->module_path = module_name;
            module_info->resolved_path = resolved_path;
            module_info->is_loading = true;
            
            // Add to loading stack
            m_loading_stack.insert(module_name);
            
            try
            {
                // Read and parse the module file
                std::ifstream file(resolved_path);
                if (!file.is_open())
                {
                    std::cerr << "Failed to open module file: " << resolved_path << std::endl;
                    m_loading_stack.erase(module_name);
                    return false;
                }
                
                std::string source((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                file.close();
                
                // Tokenize
                Lexer lexer(source);
                std::vector<Token> tokens = lexer.scan_tokens();
                
                // Parse
                Parser parser(tokens);
                module_info->parsed_statements = parser.parse();
                
                if (parser.had_error())
                {
                    std::cerr << "Parse error in module: " << module_name << std::endl;
                    m_loading_stack.erase(module_name);
                    return false;
                }
                
                // Mark as loaded
                module_info->is_loading = false;
                module_info->is_loaded = true;
                
                // Store module info
                m_modules[module_name] = std::move(module_info);
                
                // Remove from loading stack
                m_loading_stack.erase(module_name);
                
                std::cout << "Module loaded successfully: " << module_name << " -> " << resolved_path << std::endl;
                return true;
            }
            catch (const std::exception& e)
            {
                std::cerr << "Error loading module " << module_name << ": " << e.what() << std::endl;
                m_loading_stack.erase(module_name);
                return false;
            }
        }

        bool ModuleManager::is_module_loaded(const std::string& module_name)
        {
            auto it = m_modules.find(module_name);
            return it != m_modules.end() && it->second->is_loaded;
        }

        ModuleInfo* ModuleManager::get_module_info(const std::string& module_name)
        {
            auto it = m_modules.find(module_name);
            if (it != m_modules.end())
            {
                return it->second.get();
            }
            return nullptr;
        }

        void ModuleManager::register_export(const std::string& module_name, const std::string& symbol_name, const std::string& symbol_type)
        {
            auto it = m_modules.find(module_name);
            if (it != m_modules.end())
            {
                it->second->exports[symbol_name] = symbol_type;
            }
        }

        bool ModuleManager::has_export(const std::string& module_name, const std::string& symbol_name)
        {
            auto it = m_modules.find(module_name);
            if (it != m_modules.end())
            {
                return it->second->exports.find(symbol_name) != it->second->exports.end();
            }
            return false;
        }

        std::string ModuleManager::get_export_type(const std::string& module_name, const std::string& symbol_name)
        {
            auto it = m_modules.find(module_name);
            if (it != m_modules.end())
            {
                auto export_it = it->second->exports.find(symbol_name);
                if (export_it != it->second->exports.end())
                {
                    return export_it->second;
                }
            }
            return "";
        }

        bool ModuleManager::has_circular_dependency(const std::string& module_name, const std::string& importing_module)
        {
            return m_loading_stack.find(importing_module) != m_loading_stack.end();
        }

        void ModuleManager::add_search_path(const std::string& path)
        {
            std::string normalized = normalize_path(path);
            if (std::find(m_search_paths.begin(), m_search_paths.end(), normalized) == m_search_paths.end())
            {
                m_search_paths.push_back(normalized);
            }
        }

        void ModuleManager::remove_search_path(const std::string& path)
        {
            std::string normalized = normalize_path(path);
            m_search_paths.erase(
                std::remove(m_search_paths.begin(), m_search_paths.end(), normalized),
                m_search_paths.end()
            );
        }

        std::vector<std::string> ModuleManager::get_search_paths() const
        {
            return m_search_paths;
        }

        // Global instance
        static ModuleManager g_module_manager;

        ModuleManager& get_module_manager()
        {
            return g_module_manager;
        }
    }
}
