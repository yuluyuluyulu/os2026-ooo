#include <list.h>
#include <mmu.h>
#include <pmm.h>
#include <slub.h>
#include <stdio.h>
#include <string.h>

/* Simplified two-layer SLUB: page allocator below, size-class objects above. */
#define SLUB_CACHE_COUNT 8
#define SLUB_MAGIC 0x534c5542534d414cULL
#define SLUB_LARGE_MAGIC 0x534c55424c415247ULL

static const size_t slub_sizes[SLUB_CACHE_COUNT] = {
    16, 32, 64, 128, 256, 512, 1024, 2048,
};

struct slub_cache {
    size_t object_size;
    list_entry_t slabs;
};

struct slub_page {
    uint64_t magic;
    size_t cache_index;
    size_t total;
    size_t free;
    void *free_list;
    struct Page *page;
    list_entry_t link;
};

struct slub_large {
    uint64_t magic;
    size_t pages;
    struct Page *page;
};

static struct slub_cache caches[SLUB_CACHE_COUNT];
static bool slub_ready;

static void *
page_kva(struct Page *page) {
    return (void *)(page2pa(page) + va_pa_offset);
}

void
slub_init(void) {
    for (size_t i = 0; i < SLUB_CACHE_COUNT; i ++) {
        caches[i].object_size = slub_sizes[i];
        list_init(&caches[i].slabs);
    }
    slub_ready = 1;
}

static struct slub_page *
new_slab(size_t cache_index) {
    struct Page *page = alloc_page();
    if (page == NULL) {
        return NULL;
    }

    uintptr_t start = (uintptr_t)page_kva(page);
    memset((void *)start, 0, PGSIZE);
    struct slub_page *slab = (struct slub_page *)start;
    uintptr_t first = ROUNDUP(start + sizeof(struct slub_page), sizeof(uintptr_t));
    size_t object_size = caches[cache_index].object_size;

    slab->magic = SLUB_MAGIC;
    slab->cache_index = cache_index;
    slab->total = (PGSIZE - (first - start)) / object_size;
    slab->free = slab->total;
    slab->free_list = NULL;
    slab->page = page;

    for (size_t i = slab->total; i > 0; i --) {
        void *object = (void *)(first + (i - 1) * object_size);
        *(void **)object = slab->free_list;
        slab->free_list = object;
    }
    list_add(&caches[cache_index].slabs, &slab->link);
    return slab;
}

void *
slub_alloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    if (!slub_ready) {
        slub_init();
    }

    size_t cache_index = 0;
    while (cache_index < SLUB_CACHE_COUNT && slub_sizes[cache_index] < size) {
        cache_index ++;
    }
    if (cache_index == SLUB_CACHE_COUNT) {
        size_t pages = ROUNDUP(sizeof(struct slub_large) + size, PGSIZE) / PGSIZE;
        struct Page *page = alloc_pages(pages);
        if (page == NULL) {
            return NULL;
        }
        struct slub_large *large = (struct slub_large *)page_kva(page);
        large->magic = SLUB_LARGE_MAGIC;
        large->pages = pages;
        large->page = page;
        return (void *)ROUNDUP((uintptr_t)(large + 1), sizeof(uintptr_t));
    }

    struct slub_cache *cache = &caches[cache_index];
    struct slub_page *slab = NULL;
    list_entry_t *le = &cache->slabs;
    while ((le = list_next(le)) != &cache->slabs) {
        struct slub_page *candidate = to_struct(le, struct slub_page, link);
        if (candidate->free > 0) {
            slab = candidate;
            break;
        }
    }
    if (slab == NULL && (slab = new_slab(cache_index)) == NULL) {
        return NULL;
    }

    void *object = slab->free_list;
    slab->free_list = *(void **)object;
    slab->free --;
    return object;
}

void
slub_free(void *object) {
    assert(object != NULL);
    uintptr_t page_start = ROUNDDOWN((uintptr_t)object, PGSIZE);
    uint64_t magic = *(uint64_t *)page_start;

    if (magic == SLUB_LARGE_MAGIC) {
        struct slub_large *large = (struct slub_large *)page_start;
        free_pages(large->page, large->pages);
        return;
    }

    assert(magic == SLUB_MAGIC);
    struct slub_page *slab = (struct slub_page *)page_start;
    assert(slab->cache_index < SLUB_CACHE_COUNT && slab->free < slab->total);
    size_t object_size = caches[slab->cache_index].object_size;
    uintptr_t first = ROUNDUP(page_start + sizeof(struct slub_page), sizeof(uintptr_t));
    uintptr_t address = (uintptr_t)object;
    assert(address >= first && address < first + slab->total * object_size);
    assert((address - first) % object_size == 0);

    *(void **)object = slab->free_list;
    slab->free_list = object;
    slab->free ++;
    if (slab->free == slab->total) {
        struct Page *page = slab->page;
        list_del(&slab->link);
        free_page(page);
    }
}

void
slub_check(void) {
    size_t before = nr_free_pages();
    const size_t sizes[] = {1, 17, 100, 2000, 6000};
    void *objects[5];
    for (size_t i = 0; i < 5; i ++) {
        objects[i] = slub_alloc(sizes[i]);
        assert(objects[i] != NULL);
        memset(objects[i], (char)(0x20 + i), sizes[i]);
    }
    assert(nr_free_pages() < before);
    for (size_t i = 0; i < 5; i ++) {
        slub_free(objects[i]);
    }
    assert(nr_free_pages() == before);

    /* Force the 64-byte class to span multiple slabs, then release them all. */
    void *many[70];
    for (size_t i = 0; i < 70; i ++) {
        many[i] = slub_alloc(64);
        assert(many[i] != NULL);
    }
    for (size_t i = 0; i < 70; i ++) {
        slub_free(many[i]);
    }
    assert(nr_free_pages() == before);
    cprintf("slub allocator check succeeded!\n");
}
