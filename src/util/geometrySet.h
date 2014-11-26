#ifndef GEOMETRY_SET_H
#define GEOMETRY_SET_H

#include <vector>
#include <algorithm>

// This entire class is in the .h file because Visual Studio throws Intellisense errors
// when geometrySet.cpp is included at the end (as is commonplace for templated classes).

template <class T>
class GeometrySet {

public:

    /**
     * Default constructor; does nothing
     */
    GeometrySet() { }

    /**
     * Copy constructor; copies other's vector of items
     */
    GeometrySet(const GeometrySet<T> & other) {
        items = other.items;
    }

    /**
     * Inserts the given item into the set if it does not already exist in the set
     */
    void insert(T item) {
        if (!contains(item)) {
            items.push_back(item);
        }
    }

    /**
     * Returns a set that contains all of the elements of the current and given sets
     */
    GeometrySet<T> unionWith(GeometrySet<T> setToUnion) const {
        GeometrySet<T> newSet(*this);
        for (size_t i = 0; i < setToUnion.items.size(); i++) {
            newSet.insert(setToUnion.items[i]);
        }
        return newSet;
    }

    /**
     * Returns a set that contains all intersecting elements of the current and given sets
     */
    GeometrySet<T> intersect(GeometrySet<T> setToIntersect) const {
        GeometrySet<T> newSet;
        for (size_t i = 0; i < items.size(); i++) {
            if (setToIntersect.contains(items[i])) {
                newSet.insert(items[i]);
            }
        }
        return newSet;
    }
    
    /**
     * Removes the given item from the set if it exists in the set
     */
    void remove(T item) {
        std::vector<T>::iterator it = std::find(items.begin(), items.end(), item);
        if (it != items.end()) {
            items.erase(it);
        }
    }

    /**
     * Removes all items in the given set from the current set if they exist
     */
    void subtract(GeometrySet<T> itemsToRemove) {
        for (size_t i = 0; i < itemsToRemove.items.size(); i++) {
            remove(itemsToRemove.items[i]);
        }
    }

    /**
     * Returns the number of items in the set
     */
    int size() {
        return items.size();
    }

    /**
     * Returns true if the given item is in the set, false otherwise
     */
    bool contains(T item) {
        return (std::find(items.begin(), items.end(), item) != items.end());
    }

    /**
     * Returns a const reference to the items vector for iterating over the elements in the set
     */
    const std::vector<T> & getItems() const {
        return items;
    }

private:
    std::vector<T> items;

};

#endif
