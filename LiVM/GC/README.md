# Garbage Collector for Linh VM

## Overview

The Linh VM Garbage Collector is designed following the Go runtime model, using a **tri-color mark-and-sweep** algorithm with the following features:

### Key Features

1. **Tri-color Marking Algorithm**
   - WHITE: Not visited (candidate for collection)
   - GRAY: Visited but children not scanned
   - BLACK: Visited and all children scanned

2. **Generational Collection**
   - Young Generation: Recently allocated objects
   - Old Generation: Objects that survived multiple collections
   - Promotion: Automatically promotes objects from young to old after N collections

3. **Concurrent Marking**
   - Collects concurrently with the main program
   - Reduces pause time (Stop-The-World)
   - Write barriers to ensure correctness

4. **Incremental Marking**
   - Splits marking process into multiple steps
   - Reduces latency

5. **Memory Pooling Integration**
   - Integrates with existing ObjectPool
   - Optimizes memory allocation/deallocation

## Structure

```
GC/
├── GC.hpp          # Header file với định nghĩa class và API
├── GC.cpp          # Implementation của GC
└── README.md       # This documentation
```

## Usage

### 1. Initialize GC

```cpp
#include "LiVM/GC/GC.hpp"

int main() {
    // Initialize with default configuration
    Linh::gc_init();
    
    // Or with custom configuration
    Linh::GarbageCollector::Config config;
    config.young_gen_threshold = 16 * 1024 * 1024;  // 16MB
    config.heap_threshold = 128 * 1024 * 1024;      // 128MB
    config.max_heap_size = 1024 * 1024 * 1024;      // 1GB
    config.promotion_age = 5;                        // Promote after 5 GCs
    config.concurrent_marking = true;
    config.incremental_marking = true;
    
    Linh::GarbageCollector::instance().init(config);
    
    // ... run program ...
    
    // Shutdown when done
    Linh::gc_shutdown();
    return 0;
}
```

### 2. Integration with LiVM

In `LiVM.cpp`, add GC support:

```cpp
#include "LiVM/GC/GC.hpp"

LiVM::LiVM() {
    // Initialize GC if not already initialized
    static bool gc_initialized = false;
    if (!gc_initialized) {
        Linh::gc_init();
        gc_initialized = true;
    }
}

void LiVM::push(const Value& val) {
    stack.push_back(val);
    
    // Add to root set if it's a heap object
    if (std::holds_alternative<Array>(val) || 
        std::holds_alternative<Map>(val) ||
        std::holds_alternative<FunctionPtr>(val)) {
        Linh::GarbageCollector::instance().add_root(&stack.back());
    }
    
    // Trigger GC if needed
    Linh::GarbageCollector::instance().collect_if_needed();
}

Value LiVM::pop() {
    if (stack.empty()) {
        throw std::runtime_error("Stack underflow");
    }
    Value val = stack.back();
    
    // Remove from root set
    Linh::GarbageCollector::instance().remove_root(&stack.back());
    
    stack.pop_back();
    return val;
}
```

### 3. Using GC-managed allocations

```cpp
// Allocate array with GC tracking
auto gc_array = Linh::GarbageCollector::instance()
    .allocate<std::vector<Value>>(sizeof(std::vector<Value>));

// Use like a normal shared_ptr
gc_array->push_back(Value(42));

// GC will automatically collect when no more references
```

### 4. Write Barriers

When assigning new values to heap objects during concurrent marking:

```cpp
void update_map_value(Map& map, const std::string& key, const Value& new_value) {
    // Call write barrier before assignment
    Linh::GarbageCollector::instance().write_barrier(&(*map)[key], new_value);
    
    // Perform assignment
    (*map)[key] = new_value;
}
```

### 5. Manual GC Control

```cpp
// Force GC immediately
Linh::gc_collect();

// Enable/disable GC
Linh::gc_enable(false);  // Disable GC
// ... code that doesn't need GC ...
Linh::gc_enable(true);   // Re-enable GC

// Check status
auto& gc = Linh::GarbageCollector::instance();
std::cout << "Heap size: " << gc.get_heap_size() << " bytes\n";
std::cout << "Live objects: " << gc.stats().live_objects << "\n";
std::cout << "Heap usage: " << (gc.get_heap_usage() * 100) << "%\n";

// Print detailed statistics
gc.print_stats();
```

## Optimization Configurations

### For low-latency applications

```cpp
Linh::GarbageCollector::Config config;
config.concurrent_marking = true;      // Enable concurrent marking
config.incremental_marking = true;     // Enable incremental marking
config.incremental_step_size = 512;    // Smaller steps
config.young_gen_threshold = 4 * 1024 * 1024;  // Collect young gen earlier
```

### For throughput-oriented applications

```cpp
Linh::GarbageCollector::Config config;
config.concurrent_marking = false;     // Disable concurrent (less overhead)
config.young_gen_threshold = 32 * 1024 * 1024;  // Higher threshold
config.promotion_age = 10;             // Keep objects in young gen longer
```

### For embedded systems (limited memory)

```cpp
Linh::GarbageCollector::Config config;
config.max_heap_size = 64 * 1024 * 1024;       // 64MB max
config.heap_threshold = 32 * 1024 * 1024;      // Collect early
config.young_gen_threshold = 8 * 1024 * 1024;  // Small young gen
```

## Monitoring and Debugging

### GC Statistics

```cpp
auto& gc = Linh::GarbageCollector::instance();
const auto& stats = gc.stats();

std::cout << "Total allocations: " << stats.total_allocations << "\n";
std::cout << "Total collections: " << stats.total_collections << "\n";
std::cout << "Young collections: " << stats.young_collections << "\n";
std::cout << "Full collections: " << stats.full_collections << "\n";
std::cout << "Bytes allocated: " << stats.bytes_allocated << "\n";
std::cout << "Bytes freed: " << stats.bytes_freed << "\n";
std::cout << "Average pause time: " 
          << (stats.total_pause_time_us / stats.total_collections / 1000.0) 
          << " ms\n";
std::cout << "Max pause time: " << (stats.max_pause_time_us / 1000.0) << " ms\n";
```

### Environment Variables (similar to Go)

You can add support for environment variables:

```cpp
// Read GOGC-style configuration
const char* gogc = std::getenv("LINH_GC");
if (gogc) {
    int percent = std::atoi(gogc);
    config.gc_percent = percent;
}

// LINH_GC=100 -> target 100% overhead (default)
// LINH_GC=50  -> target 50% overhead (more aggressive)
// LINH_GC=200 -> target 200% overhead (less aggressive)
```

## Comparison with Go GC

| Feature | Go GC | Linh GC | Notes |
|---------|-------|---------|-------|
| Tri-color marking | ✓ | ✓ | Same |
| Concurrent marking | ✓ | ✓ | Same |
| Generational | ✗ | ✓ | Linh has this |
| Write barriers | ✓ | ✓ | Same |
| STW pauses | Minimal | Minimal | Both optimized |
| GOGC tuning | ✓ | ✓ | Similar |

## Performance Tips

1. **Reduce allocations**: Use object pooling for temporary objects
2. **Batch operations**: Group multiple operations to reduce GC overhead
3. **Pre-allocate**: Allocate before entering hot paths
4. **Tune thresholds**: Adjust thresholds based on workload
5. **Monitor stats**: Track GC stats for optimization

## Troubleshooting

### GC pauses too long

- Decrease `heap_threshold` to run GC earlier
- Enable `concurrent_marking` and `incremental_marking`
- Decrease `incremental_step_size`

### High memory usage

- Decrease `young_gen_threshold`
- Decrease `promotion_age`
- Check for memory leaks (unreleased objects)

### GC runs too frequently

- Increase `young_gen_threshold` and `heap_threshold`
- Increase `promotion_age`
- Optimize code to reduce allocations

## API Reference

### GarbageCollector Class

```cpp
class GarbageCollector {
public:
    static GarbageCollector& instance();
    
    void init(const Config& config = Config());
    void shutdown();
    
    template<typename T>
    GCPtr<T> allocate(size_t size);
    
    void add_root(Value* root);
    void remove_root(Value* root);
    
    void collect_young();
    void collect_full();
    void collect_if_needed();
    void force_gc();
    
    void enable_gc(bool enabled);
    bool is_gc_enabled() const;
    
    void write_barrier(Value* slot, const Value& new_value);
    
    const GCStats& stats() const;
    void print_stats() const;
    
    size_t get_heap_size() const;
    size_t get_live_size() const;
    double get_heap_usage() const;
};
```

### Helper Functions

```cpp
void gc_init();                    // Initialize GC with default config
void gc_shutdown();                // Shutdown GC
void gc_collect();                 // Force full GC
void gc_enable(bool enabled);      // Enable/disable GC
```

## License

Same license as the Linh project.
