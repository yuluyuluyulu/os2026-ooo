#include <buddy_pmm.h>
#include <list.h>
#include <stdio.h>

/*
 * Challenge: a binary buddy allocator for the physical-page layer.
 * Order k contains free blocks of 2^k pages.  Requests are rounded up to the
 * next power of two, then a larger block is repeatedly split when necessary.
 */
#define BUDDY_MAX_ORDER 15

static list_entry_t buddy_free_lists[BUDDY_MAX_ORDER + 1];
static size_t buddy_nr_free;
static struct Page *buddy_base;
static size_t buddy_page_count;

static size_t
order_size(unsigned int order) {
    return (size_t)1 << order;
}

static unsigned int
size_to_order(size_t n) {
    unsigned int order = 0;
    size_t size = 1;
    while (size < n) {
        size <<= 1;
        order ++;
    }
    return order;
}

static void
add_free_block(struct Page *base, unsigned int order) {
    base->property = order_size(order);
    SetPageProperty(base);
    list_add(&buddy_free_lists[order], &base->page_link);
}

static void
buddy_init(void) {
    for (unsigned int order = 0; order <= BUDDY_MAX_ORDER; order ++) {
        list_init(&buddy_free_lists[order]);
    }
    buddy_nr_free = 0;
    buddy_base = NULL;
    buddy_page_count = 0;
}

static void
buddy_init_memmap(struct Page *base, size_t n) {
    assert(n > 0 && n <= order_size(BUDDY_MAX_ORDER));
    assert(buddy_base == NULL);

    buddy_base = base;
    buddy_page_count = n;
    for (struct Page *p = base; p != base + n; p ++) {
        assert(PageReserved(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }

    /* Decompose a non-power-of-two memory range into aligned buddy blocks. */
    size_t offset = 0;
    while (offset < n) {
        unsigned int order = 0;
        while (order < BUDDY_MAX_ORDER) {
            size_t next_size = order_size(order + 1);
            if (next_size > n - offset || (offset & (next_size - 1)) != 0) {
                break;
            }
            order ++;
        }
        add_free_block(base + offset, order);
        offset += order_size(order);
    }
    buddy_nr_free = n;
}

static struct Page *
buddy_alloc_pages(size_t n) {
    assert(n > 0);
    unsigned int wanted = size_to_order(n);
    if (wanted > BUDDY_MAX_ORDER || order_size(wanted) > buddy_nr_free) {
        return NULL;
    }

    unsigned int found = wanted;
    while (found <= BUDDY_MAX_ORDER && list_empty(&buddy_free_lists[found])) {
        found ++;
    }
    if (found > BUDDY_MAX_ORDER) {
        return NULL;
    }

    list_entry_t *le = list_next(&buddy_free_lists[found]);
    struct Page *block = le2page(le, page_link);
    list_del(le);
    ClearPageProperty(block);
    block->property = 0;

    while (found > wanted) {
        found --;
        struct Page *right = block + order_size(found);
        add_free_block(right, found);
    }

    buddy_nr_free -= order_size(wanted);
    return block;
}

static void
buddy_free_pages(struct Page *base, size_t n) {
    assert(n > 0 && buddy_base != NULL);
    unsigned int order = size_to_order(n);
    size_t size = order_size(order);
    size_t offset = base - buddy_base;
    assert(order <= BUDDY_MAX_ORDER && offset + size <= buddy_page_count);
    assert((offset & (size - 1)) == 0);

    for (struct Page *p = base; p != base + size; p ++) {
        assert(!PageReserved(p) && !PageProperty(p));
        p->flags = 0;
        p->property = 0;
        set_page_ref(p, 0);
    }

    buddy_nr_free += size;
    while (order < BUDDY_MAX_ORDER) {
        size_t buddy_offset = offset ^ order_size(order);
        if (buddy_offset + order_size(order) > buddy_page_count) {
            break;
        }
        struct Page *other = buddy_base + buddy_offset;
        if (!PageProperty(other) || other->property != order_size(order)) {
            break;
        }
        list_del(&other->page_link);
        ClearPageProperty(other);
        other->property = 0;
        if (buddy_offset < offset) {
            offset = buddy_offset;
            base = other;
        }
        order ++;
    }
    add_free_block(base, order);
}

static size_t
buddy_nr_free_pages(void) {
    return buddy_nr_free;
}

static void
buddy_check(void) {
    size_t before = nr_free_pages();
    struct Page *a = alloc_pages(1);
    struct Page *b = alloc_pages(3);
    struct Page *c = alloc_pages(5);
    assert(a != NULL && b != NULL && c != NULL);
    assert(a != b && a != c && b != c);
    assert(nr_free_pages() == before - 1 - 4 - 8);
    free_pages(a, 1);
    free_pages(b, 3);
    free_pages(c, 5);
    assert(nr_free_pages() == before);

    /* Two order-0 buddies must coalesce and satisfy an order-1 request. */
    a = alloc_page();
    b = alloc_page();
    assert(a != NULL && b != NULL);
    free_page(a);
    free_page(b);
    c = alloc_pages(2);
    assert(c != NULL);
    free_pages(c, 2);
    assert(nr_free_pages() == before);
    cprintf("buddy_system check succeeded!\n");
}

const struct pmm_manager buddy_pmm_manager = {
    .name = "buddy_system_pmm_manager",
    .init = buddy_init,
    .init_memmap = buddy_init_memmap,
    .alloc_pages = buddy_alloc_pages,
    .free_pages = buddy_free_pages,
    .nr_free_pages = buddy_nr_free_pages,
    .check = buddy_check,
};
