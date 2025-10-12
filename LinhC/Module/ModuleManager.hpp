#ifndef LINH_MODULE_MANAGER_HPP
#define LINH_MODULE_MANAGER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <memory>
#include <filesystem>
#include "../Parsing/AST/ASTNode.hpp"

namespace Linh
{
    namespace Module
    {
        struct ModuleInfo
        {
            std::string module_path;
            std::string resolved_path;
            AST::StmtList parsed_statements;
            std::unordered_map<std::string, std::string> exports; // name -> type
            bool is_loaded = false;
            bool is_loading = false; // For circular dependency detection
        };

        class ModuleManager
        {
        public:
            ModuleManager();
            ~ModuleManager();

            // Path resolution
            std::string resolve_module_path(const std::string& module_name, const std::string& current_file_path = "");
            std::string normalize_path(const std::string& path);
            
            // Module loading
            bool load_module(const std::string& module_name, const std::string& current_file_path = "");
            bool is_module_loaded(const std::string& module_name);
            ModuleInfo* get_module_info(const std::string& module_name);
            
            // Export/Import management
            void register_export(const std::string& module_name, const std::string& symbol_name, const std::string& symbol_type);
            bool has_export(const std::string& module_name, const std::string& symbol_name);
            std::string get_export_type(const std::string& module_name, const std::string& symbol_name);
            
            // Circular dependency detection
            bool has_circular_dependency(const std::string& module_name, const std::string& importing_module);
            
            // Search paths
            void add_search_path(const std::string& path);
            void remove_search_path(const std::string& path);
            std::vector<std::string> get_search_paths() const;

        private:
            std::unordered_map<std::string, std::unique_ptr<ModuleInfo>> m_modules;
            std::vector<std::string> m_search_paths;
            std::unordered_set<std::string> m_loading_stack; // For circular dependency detection
            
            // Helper methods
            std::string find_module_file(const std::string& module_name, const std::string& current_dir);
            bool is_relative_path(const std::string& path);
            std::string get_file_extension(const std::string& path);
            std::string convert_path_separators(const std::string& path);
        };

        // Global module manager instance
        ModuleManager& get_module_manager();
    }
}

#endif // LINH_MODULE_MANAGER_HPP
