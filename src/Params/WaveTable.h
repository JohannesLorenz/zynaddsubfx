/*
  ZynAddSubFX - a software synthesizer

  WaveTable.h - WaveTable definition
  Copyright (C) 2020-2020 Johannes Lorenz
  Author: Johannes Lorenz

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#ifndef WAVETABLE_H
#define WAVETABLE_H

#include <cassert>
#include "../Misc/Allocator.h"

namespace zyn {

struct Shape1 {
    std::size_t dim[1];
    bool operator==(const Shape1& other) const { return dim[0] == other.dim[0]; }
};
struct Shape2  { std::size_t dim[2];
    bool operator==(const Shape2& other) const {
            return dim[0] == other.dim[0] && dim[1] == other.dim[1]; }
};
struct Shape3  { std::size_t dim[3];
    bool operator==(const Shape3& other) const {
             return dim[0] == other.dim[0] && dim[1] == other.dim[1] && dim[2] == other.dim[2]; }
};



template <class T>
class Tensor3
{
    T    ***m_data;
    Shape3  m_shape;
public:
    Tensor3(Shape3 shape) :
        m_data(reinterpret_cast<T***>(new T[shape.dim[0] * shape.dim[1] * shape.dim[2]])),
        m_shape(shape)
    {}
    ~Tensor3() { delete[](reinterpret_cast<T*>(m_data)); }
    Tensor3(Tensor3&& other) = default;
    Tensor3& operator=(Tensor3&& other) = default;

    Shape3 shape() const { return m_shape; }
    // those operator[] are a bit cheated, but well...
    T**& operator[](std::size_t i) { return m_data[i]; }
    const T**& operator[](std::size_t i) const { return m_data[i]; }
    template<class X>
    friend void pointer_swap(Tensor3<X>&, Tensor3<X>&);
};

template <class T>
class Tensor2
{
    T     **m_data;
    Shape2  m_shape;
public:
    Tensor2(Shape2 shape) :
        m_data(reinterpret_cast<T**>(new T[shape.dim[0] * shape.dim[1]])),
        m_shape(shape)
    {}
    ~Tensor2() { delete[](reinterpret_cast<T*>(m_data)); }
    Tensor2(Tensor2&& other) = default;
    Tensor2& operator=(Tensor2&& other) = default;

    Shape2 shape() const { return m_shape; }
    // those operator[] are a bit cheated, but well...
    T*& operator[](std::size_t i) { return m_data[i]; }
    const T*& operator[](std::size_t i) const { return m_data[i]; }
    template<class X>
    friend void pointer_swap(Tensor2<X>&, Tensor2<X>&);
};
template <class T>
class Tensor1
{
    T      *m_data;
    Shape1  m_shape;
public:
    Tensor1(Shape1 shape) :
        m_data(new T[shape.dim[0]]),
        m_shape(shape)
    {}
    ~Tensor1() { delete[] m_data; }
    Tensor1(Tensor1&& other) = default;
    Tensor1& operator=(Tensor1&& other) = default;

    Shape1 shape() const { return m_shape; }
    T& operator[](std::size_t i) { return m_data[i]; }
    const T& operator[](std::size_t i) const { return m_data[i]; }
    template<class X>
    friend void pointer_swap(Tensor1<X>&, Tensor1<X>&);
};

template<class T>
void pointer_swap(Tensor1<T>& t1, Tensor1<T>& t2) {
    assert(t1.m_shape == t2.m_shape);
    std::swap(t1.m_data, t2.m_data);
}

template<class T>
void pointer_swap(Tensor2<T>& t1, Tensor2<T>& t2) {
    assert(t1.m_shape == t2.m_shape);
    std::swap(t1.m_data, t2.m_data);
}

template<class T>
void pointer_swap(Tensor3<T>& t1, Tensor3<T>& t2) {
    assert(t1.m_shape == t2.m_shape);
    std::swap(t1.m_data, t2.m_data);
}

class WaveTable
{
public:
    using float32 = float;
    // pure guesses for what sounds good:
    constexpr const static std::size_t num_freqs = 10;
    constexpr const static std::size_t num_semantics = 128;

    enum class WtMode
    {
        freq_smps, // (freq)->samples
        freqseed_smps, // (freq, seed)->samples
        freqwave_smps // (freq, wave param)->samples
    };

private:
    Tensor1<float32> semantics; //!< E.g. oscil params or random seed (e.g. 0...127)
    Tensor1<float32> freqs; //!< The frequency of each 'row'
    Tensor3<float32> data;  //!< time=col,freq=row,semantics(oscil param or random seed)=depth
    WtMode m_mode;

public:

    void setMode(WtMode mode) { m_mode = mode; }

    //! Return sample slice for given frequency
    const Tensor1<float32>& get(float32 freq) const; // works for both seed and seedless setups
    // future extensions
    // Tensor2<float32> get_antialiased(void); // works for seed and seedless setups
    // Tensor2<float32> get_wavetablemod(float32 freq);

    //! Insert generated data into this object
    //! If this is only adding new random seeds, then the rest of the data does
    //! not need to be purged
    //! @param semantics seed or param
    void insert(Tensor3<float>& data, Tensor1<float32>& freqs, Tensor1<float32>& semantics, bool invalidate=true);

    // future extension
    // Used to determine if new random seeds are needed
    // std::size_t number_of_remaining_seeds(void);

    WaveTable(std::size_t buffersize);
    WaveTable(WaveTable&& other) = default;
    WaveTable& operator=(WaveTable&& other) = default;
};

}

#endif // WAVETABLE_H
