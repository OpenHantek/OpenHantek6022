// SPDX-License-Identifier: GPL-2.0+

#pragma once

/// \brief A class template for a simple array with a fixed size.
template <class T> class DataArray {
  public:
    DataArray(unsigned int size);
    ~DataArray();

    T *data() const;
    T operator[](unsigned int index);

    unsigned int getSize() const;

  protected:
    T *array;          ///< Pointer to the array holding the data
    unsigned int size; ///< Size of the array (Number of variables of type T)
};

/// \brief Initializes the data array.
/// \param size Size of the data array.
template <class T> DataArray<T>::DataArray(unsigned int size) {
    this->array = new T[size];
    for (unsigned int index = 0; index < size; ++index) this->array[index] = 0;
    this->size = size;
}

/// \brief Deletes the allocated data array.
template <class T> DataArray<T>::~DataArray() { delete[] this->array; }

/// \brief Returns a pointer to the array data.
/// \return The internal data array.
template <class T> T *DataArray<T>::data() const { return this->array; }

/// \brief Returns array element when using square brackets.
/// \return The array element.
template <class T> T DataArray<T>::operator[](unsigned int index) { return this->array[index]; }

/// \brief Gets the size of the array.
/// \return The size of the command in bytes.
template <class T> unsigned int DataArray<T>::getSize() const { return this->size; }
