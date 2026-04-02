#!/usr/bin/env python3
"""
LRU Cache Implementation
Author: Hinton (Silver Maine Coon)
Spec:
  - Data structure: Doubly linked list + Hash map
  - Capacity: configurable (default 1000)
  - Operations: get(key) / put(key, value)
  - Time complexity: O(1)
"""

from typing import Any, Optional


class Node:
    """Doubly linked list node for LRU cache."""
    __slots__ = ['key', 'value', 'prev', 'next']
    
    def __init__(self, key: Any = None, value: Any = None):
        self.key = key
        self.value = value
        self.prev: Optional['Node'] = None
        self.next: Optional['Node'] = None


class LRUCache:
    """
    LRU (Least Recently Used) Cache with O(1) get/put operations.
    
    Uses a doubly linked list for ordering and a hash map for O(1) lookups.
    Most recently used items are at the head, least recently used at the tail.
    """
    
    def __init__(self, capacity: int = 1000):
        if capacity <= 0:
            raise ValueError("Capacity must be positive")
        
        self.capacity = capacity
        self.cache: dict[Any, Node] = {}
        
        # Sentinel nodes for dummy head and tail
        self.head = Node()  # Most recently used
        self.tail = Node()  # Least recently used
        self.head.next = self.tail
        self.tail.prev = self.head
    
    def _remove_node(self, node: Node) -> None:
        """Remove a node from the linked list."""
        prev_node = node.prev
        next_node = node.next
        prev_node.next = next_node
        next_node.prev = prev_node
    
    def _add_to_head(self, node: Node) -> None:
        """Add a node right after the head (most recently used)."""
        node.prev = self.head
        node.next = self.head.next
        self.head.next.prev = node
        self.head.next = node
    
    def _move_to_head(self, node: Node) -> None:
        """Move an existing node to the head (mark as recently used)."""
        self._remove_node(node)
        self._add_to_head(node)
    
    def _remove_tail(self) -> Node:
        """Remove and return the least recently used node (before dummy tail)."""
        node = self.tail.prev
        self._remove_node(node)
        return node
    
    def get(self, key: Any) -> Optional[Any]:
        """
        Get value by key. Returns None if key doesn't exist.
        Time complexity: O(1)
        """
        if key not in self.cache:
            return None
        
        node = self.cache[key]
        self._move_to_head(node)  # Mark as recently used
        return node.value
    
    def put(self, key: Any, value: Any) -> None:
        """
        Put key-value pair into cache.
        If capacity is exceeded, evict the least recently used item.
        Time complexity: O(1)
        """
        if key in self.cache:
            # Update existing node
            node = self.cache[key]
            node.value = value
            self._move_to_head(node)
        else:
            # Create new node
            new_node = Node(key, value)
            self.cache[key] = new_node
            self._add_to_head(new_node)
            
            # Check capacity
            if len(self.cache) > self.capacity:
                # Evict LRU node
                lru_node = self._remove_tail()
                del self.cache[lru_node.key]
    
    def __len__(self) -> int:
        """Return current cache size."""
        return len(self.cache)
    
    def __contains__(self, key: Any) -> bool:
        """Check if key exists in cache."""
        return key in self.cache
    
    def clear(self) -> None:
        """Clear all items from cache."""
        self.cache.clear()
        self.head.next = self.tail
        self.tail.prev = self.head


# ============== Test Cases ==============

def test_basic_operations():
    """Test 1: Basic get/put operations."""
    cache = LRUCache(capacity=3)
    
    cache.put(1, "a")
    cache.put(2, "b")
    cache.put(3, "c")
    
    assert cache.get(1) == "a"
    assert cache.get(2) == "b"
    assert cache.get(3) == "c"
    assert len(cache) == 3
    
    print("✅ Test 1 passed: Basic operations work correctly")


def test_eviction():
    """Test 2: LRU eviction when capacity exceeded."""
    cache = LRUCache(capacity=2)
    
    cache.put(1, "first")
    cache.put(2, "second")
    
    # Access key 1, making key 2 the LRU
    assert cache.get(1) == "first"
    
    # Add new item, should evict key 2
    cache.put(3, "third")
    
    assert cache.get(1) == "first"  # Still exists
    assert cache.get(2) is None     # Evicted
    assert cache.get(3) == "third"  # New item
    
    print("✅ Test 2 passed: LRU eviction works correctly")


def test_update_existing():
    """Test 3: Update existing key and verify LRU order."""
    cache = LRUCache(capacity=2)
    
    cache.put("a", 1)
    cache.put("b", 2)
    
    # Update existing key
    cache.put("a", 100)
    
    assert cache.get("a") == 100
    assert len(cache) == 2
    
    # Add new item, should evict "b" (not "a" which was just updated)
    cache.put("c", 3)
    
    assert cache.get("a") == 100  # Still exists (was recently updated)
    assert cache.get("b") is None # Evicted
    assert cache.get("c") == 3
    
    print("✅ Test 3 passed: Update existing key works correctly")


if __name__ == "__main__":
    test_basic_operations()
    test_eviction()
    test_update_existing()
    print("\n🎉 All tests passed! Meow~")
