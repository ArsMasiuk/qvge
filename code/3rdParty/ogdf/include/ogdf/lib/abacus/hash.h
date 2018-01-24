/*!\file
 * \author Matthias Elf
 * \brief hash table.
 *
 * \par License:
 * This file is part of ABACUS - A Branch And CUt System
 * Copyright (C) 1995 - 2003
 * University of Cologne, Germany
 *
 * \par
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * \par
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * \par
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * \see http://www.gnu.org/copyleft/gpl.html
 */

#pragma once

#include <ogdf/lib/abacus/abacusroot.h>

namespace abacus {

class AbacusGlobal;

template<class KeyType,class ItemType> class AbaHash;

template <class KeyType, class ItemType>
class  AbaHashItem;
template <class KeyType, class ItemType>
class  AbaHash;

template <class KeyType, class ItemType>
std::ostream &operator<< (std::ostream &out, const AbaHashItem<KeyType, ItemType> &rhs);

template <class KeyType, class ItemType>
std::ostream &operator<< (std::ostream &out, const AbaHash<KeyType, ItemType> &hash);


//! Items in hash tables.
/*
 * \sa AbaHash
 */
template <class KeyType, class ItemType>
class  AbaHashItem :  public AbacusRoot  {
	friend class AbaHash<KeyType, ItemType>;

public:

	//! \brief The constructor.
	/**
	 * \param key  The key of the item.
	 * \param item The value of the item.
	 */
	AbaHashItem(const KeyType &key, const ItemType &item);


	//! The output operator writes the key and the value of the item on the stream \a out.
	/**
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<< <> (std::ostream &,
		const AbaHashItem<KeyType, ItemType> &);

	//! Returns a pointer to the next hash-item stored in the linked list corresponding to the slot of this item.
	AbaHashItem<KeyType, ItemType> *next();

	//! Returns a const pointer to the next hash-item stored in the linked list corresponding to the slot of this item.
	const AbaHashItem<KeyType, ItemType> *next() const;

private:
	KeyType                      key_;
	ItemType                     item_;
	AbaHashItem<KeyType, ItemType> *next_;

	OGDF_NEW_DELETE
};


//! Hash tables.
/**
 * This data structure stores a set of items and provides as central
 * functions the insertion of a new item, the search for an item, and
 * the deletion of an item.
 *
 * Each item is associated with a key. The set of all possible keys is
 * called the universe. A hash table has a fixed size n. A hash function
 * assigns to each key of the universe a number in {0,..., n-1},
 * which we denote slot. If an item is inserted in the hash table,
 * then it is stored in the component of the array associated with its slot.
 * Usually, \a n is much smaller than the cardinality
 * of the universe. Hence, it can happen that two elements are mapped
 * to the same slot. This is called a collision. In order to resolve
 * collisions, each slot of the hash table does not store an item
 * explicitly, but is the start of a linear list storing all items
 * mapped to this slot.
 *
 * This template implements a hash table where collisions are resolved
 * by chaining.
 * Currently hash functions for keys of type \a int and string
 * are implemented. If you want to use this data structure for other
 * types (e.g., \a YOURTYPE), you should derive a class from the class AbaHash
 * and define a hash function \a {int hf(YOURTYPE key)}.
 *
 * The following sections implement two new classes. The class AbaHash contains
 * the hash table which consists of pointers to the class AbaHashItem.
 * The class AbaHashItem stores
 * an inserted element and its key and provides the a pointer to the next item
 * in the linked list.
 */
template <class KeyType, class ItemType>
class  AbaHash :  public AbacusRoot  {
public:

	//! Initializes each slot with a 0-pointer to indicate that the linked list of hash items of this slot is empty.
	/**
	 * \param size The size of the hash table.
	 */
	explicit AbaHash(int size);

	//! The destructor
	/**
	 * Deletes each hash item by going through all non-empty lists of hash items.
	 */
	~AbaHash();

	//! The output operator
	/**
	 * Writes row by row all elements stored
	 * in the list associated with a slot on an output stream.
	 *
	 * The output of an empty slot is suppressed.
	 *
	 * \param out  The output stream.
	 * \param hash The hash table being output.
	 *
	 * \return A reference to the output stream.
	 */
	friend std::ostream &operator<< <> (std::ostream &out,
		const AbaHash<KeyType, ItemType> &hash);

	//! Adds an item to the hash table.
	/**
	 * The new item is inserted at the head of the list in the corresponding
	 * slot. It is possible to insert several items with the same key into
	 * the hash table.
	 *
	 * \param newKey  The key of the new item.
	 * \param newItem The item being inserted.
	 */
	void insert(const KeyType &newKey, const ItemType &newItem);

	//! Adds a item to the has table (with overwrite).
	/**
	 * Performs a regular \a insert() if there is no item with the same key in the hash table,
	 * otherwise the item is replaced by the new item.
	 *
	 * \param newKey  The key of the new item.
	 * \param newItem The item being inserted.
	 */
	void overWrite(const KeyType &newKey, const ItemType &newItem);

	//! Looks for an item in the hash table with a given key.
	/**
	 * \param key The key of the searched item.
	 *
	 * \return A pointer to an item with the given key, or a 0-pointer if there
	 *         is no item with this key in the hash table. If there is more than
	 *         one item in the hash table with this key, a pointer to the first item found
	 *         is returned.
	 */
	const ItemType *find(const KeyType &key) const;

	//! Looks for an item in the hash table with a given key.
	/**
	 * \param key The key of the searched item.
	 *
	 * \return A pointer to an item with the given key, or a 0-pointer if there
	 *         is no item with this key in the hash table. If there is more than
	 *         one item in the hash table with this key, a pointer to the first item found
	 *         is returned.
	 */
	ItemType *find(const KeyType &key);

	//! Checks if a prespecified item with a prespecified key is contained in the hash table.
	/**
	 * \param key The key of the item.
	 * \param item The searched item.
	 *
	 * \return true If there is an element \a (key, item) in the hash table, false otherwise.
	 */
	bool find(const KeyType &key, const ItemType &item) const;

	/*! @name
	 * The functions \a initializeIteration() and \a next()
	 * can be used to iterate through all items stored in the hash table having the same key.
	 */
	//@{

	//! This function retrieves the first item.
	/**
	 * \param key The key of the items through which we want to iterate.
	 *
	 * \return A pointer to the first item found in the hash table having key \a key,
	 *         or 0 if there is no such item.
	 */
	ItemType *initializeIteration(const KeyType &key);

	//! This function retrieves the first item.
	/**
	 * \param key The key of the items through which we want to iterate.
	 *
	 * \return A const pointer to the first item found in the hash table having key \a key,
	 *         or 0 if there is no such item.
	 */
	const ItemType *initializeIteration(const KeyType &key) const;

	//! This function can be used to go to the next item in the hash table with key \a key.
	/**
	 * Before the first call of \a next() for a certain
	 * can the iteration has to be initialized by calling \a initializeItaration().
	 *
	 * \note The function \a next() gives you the next item having \a key key but
	 * not the next item in the linked list starting in a slot of the hash table.
	 *
	 * \param key The key of the items through which we want to iterate.
	 *
	 * \return A pointer to the next item having key \a key, or 0 if there is no more
	 *         item with this key in the hash table.
	 */
	ItemType *next(const KeyType &key);
	//@}

	//! This function can be used to go to the next item in the hash table with key \a key.
	/**
	 * Before the first call of \a next() for a certain
	 * can the iteration has to be initialized by calling \a initializeItaration().
	 *
	 * \note The function \a next() gives you the next item having \a key key but
	 * not the next item in the linked list starting in a slot of the hash table.
	 *
	 * \param key The key of the items through which we want to iterate.
	 *
	 * \return A const pointer to the next item having key \a key, or 0 if there is no more
	 *         item with this key in the hash table.
	 */
	const ItemType *next(const KeyType &key) const;
	//@}

	//! Removes the first item with a given key from the hash table.
	/**
	 * \param key The key of the item that should be removed.
	 *
	 * \return 0 If an item with the key is found.
	 * \return 1 If there is no item with this key.
	 */
	int remove(const KeyType &key);

	//! Removes the first item with a given key and a prespecified element from the hash table.
	/**
	 * \param key  The key of the item that should be removed.
	 * \param item The item which is searched.
	 *
	 * \return 0 If an item with the key is found.
	 * \return 1 If there is no item with this key.
	 */
	int remove(const KeyType &key, const ItemType &item);

	//! Returns the length of the hash table.
	int size() const;

	//! Returns the number of collisions which occurred during all previous calls of the functions \a insert() and \a overWrite().
	int nCollisions() const;

	//! Can be used to change the size of the hash table.
	/**
	 * \param newSize The new size of the hash table (must be positive).
	 */
	void resize(int newSize);

	private:

	//! Computes the hash value of \a key.
	/**
	 * It must be overloaded for all key types, which are used together with this template.
	 *
	 * This following version of \a hf() implements a Fibonacci hash function
	 * for keys of type \a int.
	 */
	int hf(int key) const;

	//! This version of \a hf() implements a Fibonacci hash function for keys of type \a unsigned.
	int hf(unsigned key) const;

	//! This is a hash function for character strings.
	/**
	 * It is taken from Knuth, 1993, page 300.
	 */
	int hf(const string &str) const;


	//! \brief The hash table storing a linked list of hash items in each slot.
	/**
	 *  \a table_[i] is initialized with a 0-pointer in order to indicate that
	 *  it is empty. The linked lists of each slot are terminated with
	 *  a 0-pointer, too.
	 */
	AbaHashItem<KeyType, ItemType> **table_;

	//! The length of the hash table.
	int size_;

	//! The number of collisions on calls of \a insert() and \a overWrite().
	int nCollisions_;

	//! \brief An iterator for all items stored in a slot.
	/**
	 * This variable is initialized by calling \a initializeIteration() and incremented
	 * by the function \a next().
	 */
	mutable AbaHashItem<KeyType, ItemType> *iter_;

	AbaHash(const AbaHash &rhs);
	AbaHash &operator=(const AbaHash &rhs);
};

}

#include <ogdf/lib/abacus/hash.inc>
