#pragma once
#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <atomic>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <chrono>
#include <functional>

namespace Linh {
    // Forward declarations
    struct Value;
    class LiVM;
    
    // GC Object metadata - attached to each heap-allocated object
    struct GCObjectHeader {
        enum class Color : uint8_t {
            WHITE = 0,  // Not visited (candidate for collection)
            GRAY = 1,   // Visited but children not scanned
            BLACK = 2   // Visited and children scanned
        };
        
        enum class Generation : uint8_t {
            YOUNG = 0,  // Recently allocated objects
            OLD = 1     // Survived multiple collections
        };
        
        std::atomic<Color> color{Color::WHITE};
        Generation generation{Generation::YOUNG};
        uint8_t age{0};  // Number of collections survived
        size_t size{0};  // Object size in bytes
        bool marked{false};
        
        GCObjectHeader() = default;
    };
    
    // GC Statistics
    struct GCStats {
        std::atomic<size_t> total_allocations{0};
        std::atomic<size_t> total_collections{0};
        std::atomic<size_t> young_collections{0};
        std::atomic<size_t> full_collections{0};
        std::atomic<size_t> bytes_allocated{0};
        std::atomic<size_t> bytes_freed{0};
        std::atomic<size_t> live_objects{0};
        std::atomic<size_t> total_pause_time_us{0};
        std::atomic<size_t> max_pause_time_us{0};
        
        void reset() {
            total_allocations = 0;
            total_collections = 0;
            young_collections = 0;
            full_collections = 0;
            bytes_allocated = 0;
            bytes_freed = 0;
            live_objects = 0;
            total_pause_time_us = 0;
            max_pause_time_us = 0;
        }
    };
    
    // Managed pointer wrapper for GC-tracked objects
    template<typename T>
    class GCPtr {
    public:
        GCPtr() : ptr_(nullptr), header_(nullptr) {}
        explicit GCPtr(T* ptr, GCObjectHeader* header = nullptr) 
            : ptr_(ptr), header_(header) {}
        
        T* get() const { return ptr_; }
        T& operator*() const { return *ptr_; }
        T* operator->() const { return ptr_; }
        explicit operator bool() const { return ptr_ != nullptr; }
        
        GCObjectHeader* header() const { return header_; }
        
    private:
        T* ptr_;
        GCObjectHeader* header_;
    };
    
    // Main Garbage Collector class
    class GarbageCollector {
    public:
        // Singleton instance
        static GarbageCollector& instance() {
            static GarbageCollector gc;
            return gc;
        }
        
        // Configuration
        struct Config {
            size_t young_gen_threshold = 8 * 1024 * 1024;    // 8MB - trigger young GC
            size_t heap_threshold = 64 * 1024 * 1024;        // 64MB - trigger full GC
            size_t max_heap_size = 512 * 1024 * 1024;        // 512MB - maximum heap
            uint8_t promotion_age = 3;                        // Promote to old gen after 3 collections
            bool concurrent_marking = true;                   // Enable concurrent marking
            bool incremental_marking = true;                  // Enable incremental marking
            size_t incremental_step_size = 1024;             // Objects to mark per step
            double gc_percent = 100.0;                       // Target 100% overhead (like Go's GOGC)
        };
        
        // Initialize GC with configuration
        void init(const Config& config = Config());
        
        // Shutdown GC and cleanup
        void shutdown();
        
        // Allocation tracking
        template<typename T>
        GCPtr<T> allocate(size_t size);
        
        void track_allocation(void* ptr, size_t size, GCObjectHeader* header);
        void untrack_allocation(void* ptr);
        
        // Root set management
        void add_root(Value* root);
        void remove_root(Value* root);
        void scan_roots(LiVM* vm);
        
        // Collection triggers
        void collect_young();  // Minor GC - young generation only
        void collect_full();   // Major GC - full heap
        void collect_if_needed();
        
        // Manual collection control
        void force_gc();
        void enable_gc(bool enabled);
        bool is_gc_enabled() const { return gc_enabled_.load(); }
        
        // Write barrier for concurrent marking
        void write_barrier(Value* slot, const Value& new_value);
        
        // Statistics
        const GCStats& stats() const { return stats_; }
        void print_stats() const;
        
        // Memory pressure
        size_t get_heap_size() const { return heap_size_.load(); }
        size_t get_live_size() const { return live_size_.load(); }
        double get_heap_usage() const {
            return static_cast<double>(heap_size_.load()) / config_.max_heap_size;
        }
        
    private:
        GarbageCollector();
        ~GarbageCollector();
        
        // Prevent copying
        GarbageCollector(const GarbageCollector&) = delete;
        GarbageCollector& operator=(const GarbageCollector&) = delete;
        
        // Tri-color marking algorithm
        void mark_phase();
        void mark_object(Value* obj);
        void mark_gray_objects();
        void scan_object(Value* obj);
        
        // Sweep phase
        void sweep_phase();
        void sweep_young_generation();
        void sweep_old_generation();
        
        // Concurrent marking support
        void concurrent_mark_worker();
        void start_concurrent_marking();
        void stop_concurrent_marking();
        
        // Incremental marking
        void incremental_mark_step();
        
        // Object promotion
        void promote_to_old_generation(GCObjectHeader* header);
        
        // STW (Stop-The-World) helpers
        void stw_begin();
        void stw_end();
        
        // Configuration
        Config config_;
        
        // State
        std::atomic<bool> gc_enabled_{true};
        std::atomic<bool> gc_running_{false};
        std::atomic<bool> concurrent_marking_active_{false};
        std::atomic<size_t> heap_size_{0};
        std::atomic<size_t> live_size_{0};
        std::atomic<size_t> young_gen_size_{0};
        
        // Object tracking
        std::unordered_map<void*, GCObjectHeader*> tracked_objects_;
        std::mutex objects_mutex_;
        
        // Root set
        std::unordered_set<Value*> roots_;
        std::mutex roots_mutex_;
        
        // Tri-color marking sets
        std::vector<Value*> gray_objects_;
        std::mutex gray_mutex_;
        
        // Concurrent marking
        std::unique_ptr<std::thread> marking_thread_;
        std::condition_variable marking_cv_;
        std::mutex marking_mutex_;
        
        // Statistics
        GCStats stats_;
        
        // Timing
        std::chrono::steady_clock::time_point last_gc_time_;
        
        // VM reference for stack scanning
        LiVM* vm_instance_{nullptr};
    };
    
    // RAII helper for GC pause measurement
    class GCPauseTimer {
    public:
        GCPauseTimer(GCStats& stats) 
            : stats_(stats), start_(std::chrono::steady_clock::now()) {}
        
        ~GCPauseTimer() {
            auto end = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_).count();
            stats_.total_pause_time_us += duration;
            
            size_t current_max = stats_.max_pause_time_us.load();
            while (duration > current_max && 
                   !stats_.max_pause_time_us.compare_exchange_weak(current_max, duration)) {
                // Retry if another thread updated max
            }
        }
        
    private:
        GCStats& stats_;
        std::chrono::steady_clock::time_point start_;
    };
    
    // Helper functions
    inline void gc_init() {
        GarbageCollector::instance().init();
    }
    
    inline void gc_shutdown() {
        GarbageCollector::instance().shutdown();
    }
    
    inline void gc_collect() {
        GarbageCollector::instance().force_gc();
    }
    
    inline void gc_enable(bool enabled) {
        GarbageCollector::instance().enable_gc(enabled);
    }
}
