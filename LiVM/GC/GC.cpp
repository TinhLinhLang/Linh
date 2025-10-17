#include "GC.hpp"
#include "LiVM/Variable/Value.hpp"
#include "LiVM/LiVM.hpp"
#include <iostream>
#include <algorithm>
#include <iomanip>

namespace Linh {

// ============================================================================
// GarbageCollector Implementation
// ============================================================================

GarbageCollector::GarbageCollector() 
    : last_gc_time_(std::chrono::steady_clock::now()) {
}

GarbageCollector::~GarbageCollector() {
    shutdown();
}

void GarbageCollector::init(const Config& config) {
    config_ = config;
    gc_enabled_ = true;
    
    if (config_.concurrent_marking) {
        start_concurrent_marking();
    }
    
    std::cout << "[GC] Initialized with:" << std::endl;
    std::cout << "  Young gen threshold: " << (config_.young_gen_threshold / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Heap threshold: " << (config_.heap_threshold / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Max heap: " << (config_.max_heap_size / 1024 / 1024) << " MB" << std::endl;
    std::cout << "  Concurrent marking: " << (config_.concurrent_marking ? "enabled" : "disabled") << std::endl;
}

void GarbageCollector::shutdown() {
    if (config_.concurrent_marking) {
        stop_concurrent_marking();
    }
    
    // Final cleanup
    std::lock_guard<std::mutex> lock(objects_mutex_);
    for (auto& [ptr, header] : tracked_objects_) {
        delete header;
    }
    tracked_objects_.clear();
}

// ============================================================================
// Allocation Tracking
// ============================================================================

template<typename T>
GCPtr<T> GarbageCollector::allocate(size_t size) {
    if (!gc_enabled_.load()) {
        return GCPtr<T>(new T(), nullptr);
    }
    
    // Check if we need to trigger GC before allocation
    collect_if_needed();
    
    // Allocate object and header
    T* obj = new T();
    GCObjectHeader* header = new GCObjectHeader();
    header->size = size;
    header->generation = GCObjectHeader::Generation::YOUNG;
    header->color = GCObjectHeader::Color::WHITE;
    
    // Track allocation
    track_allocation(obj, size, header);
    
    return GCPtr<T>(obj, header);
}

void GarbageCollector::track_allocation(void* ptr, size_t size, GCObjectHeader* header) {
    std::lock_guard<std::mutex> lock(objects_mutex_);
    
    tracked_objects_[ptr] = header;
    heap_size_ += size;
    
    if (header->generation == GCObjectHeader::Generation::YOUNG) {
        young_gen_size_ += size;
    }
    
    stats_.total_allocations++;
    stats_.bytes_allocated += size;
    stats_.live_objects++;
}

void GarbageCollector::untrack_allocation(void* ptr) {
    std::lock_guard<std::mutex> lock(objects_mutex_);
    
    auto it = tracked_objects_.find(ptr);
    if (it != tracked_objects_.end()) {
        GCObjectHeader* header = it->second;
        heap_size_ -= header->size;
        
        if (header->generation == GCObjectHeader::Generation::YOUNG) {
            young_gen_size_ -= header->size;
        }
        
        stats_.bytes_freed += header->size;
        stats_.live_objects--;
        
        delete header;
        tracked_objects_.erase(it);
    }
}

// ============================================================================
// Root Set Management
// ============================================================================

void GarbageCollector::add_root(Value* root) {
    std::lock_guard<std::mutex> lock(roots_mutex_);
    roots_.insert(root);
}

void GarbageCollector::remove_root(Value* root) {
    std::lock_guard<std::mutex> lock(roots_mutex_);
    roots_.erase(root);
}

void GarbageCollector::scan_roots(LiVM* vm) {
    vm_instance_ = vm;
    
    // Scan VM stack
    if (vm) {
        // Stack is private, so we'll need to add friend access or public getter
        // For now, we'll rely on explicitly added roots
    }
    
    // Mark all roots as gray
    std::lock_guard<std::mutex> lock(roots_mutex_);
    for (Value* root : roots_) {
        mark_object(root);
    }
}

// ============================================================================
// Collection Triggers
// ============================================================================

void GarbageCollector::collect_if_needed() {
    if (!gc_enabled_.load() || gc_running_.load()) {
        return;
    }
    
    size_t young_size = young_gen_size_.load();
    size_t total_size = heap_size_.load();
    
    // Trigger young generation collection
    if (young_size > config_.young_gen_threshold) {
        collect_young();
    }
    // Trigger full collection
    else if (total_size > config_.heap_threshold) {
        collect_full();
    }
}

void GarbageCollector::collect_young() {
    if (!gc_enabled_.load() || gc_running_.load()) {
        return;
    }
    
    GCPauseTimer timer(stats_);
    gc_running_ = true;
    
    std::cout << "[GC] Starting young generation collection..." << std::endl;
    
    stw_begin();
    
    // Mark phase - only scan young generation
    mark_phase();
    
    // Sweep young generation
    sweep_young_generation();
    
    stw_end();
    
    stats_.young_collections++;
    stats_.total_collections++;
    gc_running_ = false;
    
    std::cout << "[GC] Young collection completed. Live objects: " 
              << stats_.live_objects.load() << std::endl;
}

void GarbageCollector::collect_full() {
    if (!gc_enabled_.load() || gc_running_.load()) {
        return;
    }
    
    GCPauseTimer timer(stats_);
    gc_running_ = true;
    
    std::cout << "[GC] Starting full heap collection..." << std::endl;
    
    stw_begin();
    
    // Mark phase - scan entire heap
    mark_phase();
    
    // Sweep both generations
    sweep_phase();
    
    stw_end();
    
    stats_.full_collections++;
    stats_.total_collections++;
    gc_running_ = false;
    
    std::cout << "[GC] Full collection completed. Heap: " 
              << (heap_size_.load() / 1024 / 1024) << " MB, Live objects: "
              << stats_.live_objects.load() << std::endl;
}

void GarbageCollector::force_gc() {
    collect_full();
}

void GarbageCollector::enable_gc(bool enabled) {
    gc_enabled_ = enabled;
    std::cout << "[GC] " << (enabled ? "Enabled" : "Disabled") << std::endl;
}

// ============================================================================
// Tri-Color Marking Algorithm
// ============================================================================

void GarbageCollector::mark_phase() {
    // Reset all objects to white
    {
        std::lock_guard<std::mutex> lock(objects_mutex_);
        for (auto& [ptr, header] : tracked_objects_) {
            header->color = GCObjectHeader::Color::WHITE;
            header->marked = false;
        }
    }
    
    // Clear gray set
    {
        std::lock_guard<std::mutex> lock(gray_mutex_);
        gray_objects_.clear();
    }
    
    // Scan roots and mark them gray
    scan_roots(vm_instance_);
    
    // Process gray objects until none remain
    mark_gray_objects();
}

void GarbageCollector::mark_object(Value* obj) {
    if (!obj) return;
    
    // Check if object is heap-allocated and tracked
    void* ptr = nullptr;
    
    // Extract pointer from Value based on type
    if (obj->is_array()) {
        ptr = obj->data.as_array;
    } else if (obj->is_map()) {
        ptr = obj->data.as_map;
    } else if (obj->is_function()) {
        ptr = obj->data.as_function;
    }
    
    if (!ptr) return;
    
    // Find header
    GCObjectHeader* header = nullptr;
    {
        std::lock_guard<std::mutex> lock(objects_mutex_);
        auto it = tracked_objects_.find(ptr);
        if (it != tracked_objects_.end()) {
            header = it->second;
        }
    }
    
    if (!header) return;
    
    // If already marked, skip
    auto expected = GCObjectHeader::Color::WHITE;
    if (!header->color.compare_exchange_strong(expected, GCObjectHeader::Color::GRAY)) {
        return; // Already marked
    }
    
    // Add to gray set
    std::lock_guard<std::mutex> lock(gray_mutex_);
    gray_objects_.push_back(obj);
}

void GarbageCollector::mark_gray_objects() {
    while (true) {
        Value* obj = nullptr;
        
        // Get next gray object
        {
            std::lock_guard<std::mutex> lock(gray_mutex_);
            if (gray_objects_.empty()) {
                break;
            }
            obj = gray_objects_.back();
            gray_objects_.pop_back();
        }
        
        // Scan object and mark children
        scan_object(obj);
    }
}

void GarbageCollector::scan_object(Value* obj) {
    if (!obj) return;
    
    // Get header and mark as black
    void* ptr = nullptr;
    if (obj->is_array()) {
        ptr = obj->data.as_array;
        
        // Scan array elements
        auto& arr = obj->as_array_ref();
        for (auto& elem : arr) {
            mark_object(&elem);
        }
    } else if (obj->is_map()) {
        ptr = obj->data.as_map;
        
        // Scan map values
        auto& map = obj->as_map_ref();
        for (auto& [key, value] : map) {
            mark_object(&value);
        }
    } else if (obj->is_function()) {
        ptr = obj->data.as_function;
        
        // Scan closure environment
        auto& func = obj->as_function_ref();
        if (func.is_closure) {
            for (auto& [name, value] : func.environment) {
                mark_object(&value);
            }
        }
    }
    
    if (!ptr) return;
    
    // Mark as black
    std::lock_guard<std::mutex> lock(objects_mutex_);
    auto it = tracked_objects_.find(ptr);
    if (it != tracked_objects_.end()) {
        it->second->color = GCObjectHeader::Color::BLACK;
        it->second->marked = true;
    }
}

// ============================================================================
// Sweep Phase
// ============================================================================

void GarbageCollector::sweep_phase() {
    sweep_young_generation();
    sweep_old_generation();
}

void GarbageCollector::sweep_young_generation() {
    std::lock_guard<std::mutex> lock(objects_mutex_);
    
    std::vector<void*> to_delete;
    
    for (auto& [ptr, header] : tracked_objects_) {
        if (header->generation != GCObjectHeader::Generation::YOUNG) {
            continue;
        }
        
        if (!header->marked) {
            // Object is white - collect it
            to_delete.push_back(ptr);
        } else {
            // Object survived - age it
            header->age++;
            
            // Promote to old generation if old enough
            if (header->age >= config_.promotion_age) {
                promote_to_old_generation(header);
            }
        }
    }
    
    // Delete unmarked objects
    for (void* ptr : to_delete) {
        auto it = tracked_objects_.find(ptr);
        if (it != tracked_objects_.end()) {
            GCObjectHeader* header = it->second;
            
            heap_size_ -= header->size;
            young_gen_size_ -= header->size;
            stats_.bytes_freed += header->size;
            stats_.live_objects--;
            
            delete header;
            tracked_objects_.erase(it);
            
            // Note: We don't delete the actual object here because it's managed by shared_ptr
            // The shared_ptr will handle deletion when ref count reaches 0
        }
    }
}

void GarbageCollector::sweep_old_generation() {
    std::lock_guard<std::mutex> lock(objects_mutex_);
    
    std::vector<void*> to_delete;
    
    for (auto& [ptr, header] : tracked_objects_) {
        if (header->generation != GCObjectHeader::Generation::OLD) {
            continue;
        }
        
        if (!header->marked) {
            to_delete.push_back(ptr);
        }
    }
    
    // Delete unmarked objects
    for (void* ptr : to_delete) {
        auto it = tracked_objects_.find(ptr);
        if (it != tracked_objects_.end()) {
            GCObjectHeader* header = it->second;
            
            heap_size_ -= header->size;
            stats_.bytes_freed += header->size;
            stats_.live_objects--;
            
            delete header;
            tracked_objects_.erase(it);
        }
    }
}

void GarbageCollector::promote_to_old_generation(GCObjectHeader* header) {
    if (header->generation == GCObjectHeader::Generation::YOUNG) {
        young_gen_size_ -= header->size;
        header->generation = GCObjectHeader::Generation::OLD;
    }
}

// ============================================================================
// Concurrent Marking
// ============================================================================

void GarbageCollector::start_concurrent_marking() {
    concurrent_marking_active_ = true;
    marking_thread_ = std::make_unique<std::thread>(&GarbageCollector::concurrent_mark_worker, this);
}

void GarbageCollector::stop_concurrent_marking() {
    concurrent_marking_active_ = false;
    marking_cv_.notify_all();
    
    if (marking_thread_ && marking_thread_->joinable()) {
        marking_thread_->join();
    }
}

void GarbageCollector::concurrent_mark_worker() {
    while (concurrent_marking_active_.load()) {
        std::unique_lock<std::mutex> lock(marking_mutex_);
        marking_cv_.wait_for(lock, std::chrono::milliseconds(100), [this] {
            return !concurrent_marking_active_.load() || !gray_objects_.empty();
        });
        
        if (!concurrent_marking_active_.load()) {
            break;
        }
        
        // Perform incremental marking
        if (config_.incremental_marking) {
            incremental_mark_step();
        }
    }
}

void GarbageCollector::incremental_mark_step() {
    size_t marked = 0;
    
    while (marked < config_.incremental_step_size) {
        Value* obj = nullptr;
        
        {
            std::lock_guard<std::mutex> lock(gray_mutex_);
            if (gray_objects_.empty()) {
                break;
            }
            obj = gray_objects_.back();
            gray_objects_.pop_back();
        }
        
        scan_object(obj);
        marked++;
    }
}

// ============================================================================
// Write Barrier
// ============================================================================

void GarbageCollector::write_barrier(Value* slot, const Value& new_value) {
    if (!config_.concurrent_marking || !concurrent_marking_active_.load()) {
        return;
    }
    
    // If we're writing a pointer during concurrent marking,
    // we need to ensure the new object is marked
    Value* new_val = const_cast<Value*>(&new_value);
    mark_object(new_val);
}

// ============================================================================
// STW Helpers
// ============================================================================

void GarbageCollector::stw_begin() {
    // In a real implementation, this would pause all mutator threads
    // For now, we just set a flag
}

void GarbageCollector::stw_end() {
    // Resume mutator threads
}

// ============================================================================
// Statistics
// ============================================================================

void GarbageCollector::print_stats() const {
    std::cout << "\n=== Garbage Collector Statistics ===" << std::endl;
    std::cout << "Total allocations:    " << stats_.total_allocations.load() << std::endl;
    std::cout << "Total collections:    " << stats_.total_collections.load() << std::endl;
    std::cout << "  Young collections:  " << stats_.young_collections.load() << std::endl;
    std::cout << "  Full collections:   " << stats_.full_collections.load() << std::endl;
    std::cout << "Bytes allocated:      " << (stats_.bytes_allocated.load() / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Bytes freed:          " << (stats_.bytes_freed.load() / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Current heap size:    " << (heap_size_.load() / 1024 / 1024) << " MB" << std::endl;
    std::cout << "Live objects:         " << stats_.live_objects.load() << std::endl;
    std::cout << "Total pause time:     " << (stats_.total_pause_time_us.load() / 1000.0) << " ms" << std::endl;
    std::cout << "Max pause time:       " << (stats_.max_pause_time_us.load() / 1000.0) << " ms" << std::endl;
    
    if (stats_.total_collections.load() > 0) {
        double avg_pause = static_cast<double>(stats_.total_pause_time_us.load()) / 
                          stats_.total_collections.load() / 1000.0;
        std::cout << "Avg pause time:       " << std::fixed << std::setprecision(2) 
                  << avg_pause << " ms" << std::endl;
    }
    
    std::cout << "Heap usage:           " << std::fixed << std::setprecision(1)
              << (get_heap_usage() * 100.0) << "%" << std::endl;
    std::cout << "====================================\n" << std::endl;
}

// Explicit template instantiations for common types
template GCPtr<std::vector<Value>> GarbageCollector::allocate<std::vector<Value>>(size_t);
template GCPtr<std::unordered_map<std::string, Value>> GarbageCollector::allocate<std::unordered_map<std::string, Value>>(size_t);
template GCPtr<FunctionObject> GarbageCollector::allocate<FunctionObject>(size_t);

} // namespace Linh
