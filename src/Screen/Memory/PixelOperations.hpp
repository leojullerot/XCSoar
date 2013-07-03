/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#ifndef XCSOAR_SCREEN_PIXEL_OPERATIONS_HPP
#define XCSOAR_SCREEN_PIXEL_OPERATIONS_HPP

#include <functional>

/**
 * Build a PixelOperations class with a base class that implements
 * only WritePixelOperation().
 */
template<typename PixelTraits, class WritePixelOperation,
         typename SPT=PixelTraits>
class PerPixelOperations : private WritePixelOperation {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::rpointer_type rpointer_type;
  typedef typename PixelTraits::const_pointer_type const_pointer_type;
  typedef typename PixelTraits::color_type color_type;

public:
  PerPixelOperations() = default;

  template<typename... Args>
  explicit constexpr PerPixelOperations(Args&&... args)
    :WritePixelOperation(std::forward<Args>(args)...) {}

  inline void WritePixel(pointer_type p, typename SPT::color_type c) const {
    WritePixelOperation::WritePixel(p, c);
  }

  gcc_hot
  void FillPixels(pointer_type p, unsigned n,
                  typename SPT::color_type c) const {
    PixelTraits::ForHorizontal(p, n, [this, c](pointer_type p){
        /* requires "this->" due to gcc 4.7.2 crash bug */
        this->WritePixel(p, c);
      });
  }

  gcc_hot
  void CopyPixels(rpointer_type p,
                  typename SPT::const_rpointer_type src,
                  unsigned n) const {
    for (unsigned i = 0; i < n; ++i)
      WritePixel(PixelTraits::Next(p, i), SPT::ReadPixel(SPT::Next(src, i)));
  }
};

template<typename PixelTraits, class Operation, typename SPT=PixelTraits>
class UnaryWritePixel : private Operation {
  typedef typename PixelTraits::pointer_type pointer_type;

public:
  UnaryWritePixel() = default;

  template<typename... Args>
  explicit constexpr UnaryWritePixel(Args&&... args)
    :Operation(std::forward<Args>(args)...) {}

  inline void WritePixel(pointer_type p, typename SPT::color_type c) const {
    PixelTraits::WritePixel(p, (*this)(c));
  }
};

/**
 * Build a PixelOperations class with a function object that
 * manipulates the source color.  It is called "unary" because the
 * function object has one parameter.
 */
template<typename PixelTraits, class Operation, typename SPT=PixelTraits>
using UnaryPerPixelOperations =
  PerPixelOperations<PixelTraits,
                     UnaryWritePixel<PixelTraits, Operation, SPT>,
                     SPT>;

template<typename PixelTraits, class Operation, typename SPT=PixelTraits>
class BinaryWritePixel : private Operation {
  typedef typename PixelTraits::pointer_type pointer_type;

public:
  BinaryWritePixel() = default;

  template<typename... Args>
  explicit constexpr BinaryWritePixel(Args&&... args)
    :Operation(std::forward<Args>(args)...) {}

  inline void WritePixel(pointer_type p, typename SPT::color_type c) const {
    PixelTraits::WritePixel(p, (*this)(PixelTraits::ReadPixel(p), c));
  }
};

/**
 * Build a PixelOperations class with a function object that
 * manipulates the source color, blending with the (old) destination
 * color.  It is called "binary" because the function object has two
 * parameters.
 */
template<typename PixelTraits, class Operation, typename SPT=PixelTraits>
using BinaryPerPixelOperations =
  PerPixelOperations<PixelTraits,
                     BinaryWritePixel<PixelTraits, Operation, SPT>,
                     SPT>;

/**
 * Modify a destination pixel only if the check returns true.
 */
template<typename PixelTraits, typename Check>
struct ConditionalWritePixel : private Check {
  typedef typename PixelTraits::rpointer_type rpointer_type;
  typedef typename PixelTraits::const_rpointer_type const_rpointer_type;
  typedef typename PixelTraits::color_type color_type;

  ConditionalWritePixel() = default;

  template<typename... Args>
  explicit constexpr ConditionalWritePixel(Args&&... args)
    :Check(std::forward<Args>(args)...) {}

  void WritePixel(rpointer_type p, color_type c) const {
    if (Check::operator()(c))
      PixelTraits::WritePixel(p, c);
  }
};

/**
 * Modify a destination pixel only if the check returns true.
 */
template<typename PixelTraits, typename Check>
using ConditionalPixelOperations =
  PerPixelOperations<PixelTraits, ConditionalWritePixel<PixelTraits, Check>>;

/**
 * Wrap an existing function object that expects to operate on one
 * channel.  The resulting function object will operate on a
 * PixelTraits::color_type.
 */
template<typename PixelTraits, typename Operation>
class PixelPerChannelAdapter : private Operation {
  typedef typename PixelTraits::color_type color_type;
  typedef typename PixelTraits::channel_type channel_type;

public:
  PixelPerChannelAdapter() = default;

  template<typename... Args>
  explicit constexpr PixelPerChannelAdapter(Args&&... args)
    :Operation(std::forward<Args>(args)...) {}

  constexpr color_type operator()(color_type x) const {
    return PixelTraits::TransformChannels(x, [this](channel_type x) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(x);
      });
  }

  constexpr color_type operator()(color_type a, color_type b) const {
    return PixelTraits::TransformChannels(a, b,
                                          [this](channel_type a,
                                                 channel_type b) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(a, b);
      });
  }
};

/**
 * Wrapper that glues #UnaryPerPixelOperations,
 * #PixelPerChannelAdapter and a custom function class together.
 */
template<typename PixelTraits, typename Operation>
using UnaryPerChannelOperations =
  UnaryPerPixelOperations<PixelTraits,
                          PixelPerChannelAdapter<PixelTraits, Operation>>;

/**
 * Wrapper that glues #BinaryPerPixelOperations,
 * #PixelPerChannelAdapter and a custom function class together.
 */
template<typename PixelTraits, typename Operation>
using BinaryPerChannelOperations =
  BinaryPerPixelOperations<PixelTraits,
                           PixelPerChannelAdapter<PixelTraits, Operation>>;

/**
 * Wrap an existing function object that expects to operate on one
 * integer.  The resulting function object will operate on a
 * PixelTraits::color_type.
 */
template<typename PixelTraits, typename Operation>
class PixelIntegerAdapter : private Operation {
  typedef typename PixelTraits::color_type color_type;
  typedef typename PixelTraits::integer_type integer_type;

public:
  PixelIntegerAdapter() = default;

  template<typename... Args>
  explicit constexpr PixelIntegerAdapter(Args&&... args)
    :Operation(std::forward<Args>(args)...) {}

  constexpr color_type operator()(color_type x) const {
    return PixelTraits::TransformInteger(x, [this](integer_type x) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(x);
      });
  }

  constexpr color_type operator()(color_type a, color_type b) const {
    return PixelTraits::TransformInteger(a, b,
                                         [this](integer_type a,
                                                integer_type b) {
        /* requires "this->" due to gcc 4.7.2 crash bug */
        return this->Operation::operator()(a, b);
      });
  }
};

/**
 * Wrapper that glues #UnaryPerPixelOperations, #PixelIntegerAdapter
 * and a custom function class together.
 */
template<typename PixelTraits, typename Operation>
using UnaryIntegerOperations =
  UnaryPerPixelOperations<PixelTraits,
                          PixelIntegerAdapter<PixelTraits, Operation>>;

/**
 * Wrapper that glues #BinaryPerPixelOperations, #PixelIntegerAdapter
 * and a custom function class together.
 */
template<typename PixelTraits, typename Operation>
using BinaryIntegerOperations =
  BinaryPerPixelOperations<PixelTraits,
                           PixelIntegerAdapter<PixelTraits, Operation>>;

/**
 * Function that inverts all bits in the given integer.
 */
template<typename integer_type>
struct PixelBitNot {
  constexpr integer_type operator()(integer_type x) const {
    return ~x;
  }
};

/**
 * Invert all source colors.
 */
template<typename PixelTraits>
using BitNotPixelOperations =
  UnaryIntegerOperations<PixelTraits,
                         PixelBitNot<typename PixelTraits::integer_type>>;

/**
 * Combine source and destination color with bit-wise "or".
 */
template<typename PixelTraits>
using BitOrPixelOperations =
  BinaryIntegerOperations<PixelTraits,
                          std::bit_or<typename PixelTraits::integer_type>>;

template<typename integer_type>
struct PixelBitNotOr {
  constexpr integer_type operator()(integer_type a, integer_type b) const {
    return a | ~b;
  }
};

template<typename PixelTraits>
using BitNotOrPixelOperations =
  BinaryIntegerOperations<PixelTraits,
                          PixelBitNotOr<typename PixelTraits::integer_type>>;

/**
 * Combine source and destination color with bit-wise "and".
 */
template<typename PixelTraits>
using BitAndPixelOperations =
  BinaryIntegerOperations<PixelTraits,
                          std::bit_and<typename PixelTraits::integer_type>>;

/**
 * Blend source and destination color with a given alpha value.  This
 * is a per-channel function.
 */
template<typename T>
class PixelAlphaOperation {
  const int alpha;

public:
  constexpr explicit PixelAlphaOperation(uint8_t _alpha):alpha(_alpha) {}

  T operator()(T a, T b) const {
    return a + ((int(b - a) * alpha) >> 8);
  }
};

/**
 * Blend source and destination color with a given alpha value.
 */
template<typename PixelTraits>
using AlphaPixelOperations =
  BinaryPerChannelOperations<PixelTraits,
                             PixelAlphaOperation<typename PixelTraits::channel_type>>;

template<typename PixelTraits, typename SPT>
class PixelOpaqueText {
  typedef typename PixelTraits::color_type color_type;

  const color_type background_color, text_color;

public:
  constexpr PixelOpaqueText(color_type _b, color_type _t)
    :background_color(_b), text_color(_t) {}

  inline color_type operator()(typename SPT::color_type x) const {
    return SPT::IsBlack(x)
      ? background_color
      : text_color;
  }
};

template<typename PixelTraits, typename SPT>
using OpaqueTextPixelOperations =
  UnaryPerPixelOperations<PixelTraits, PixelOpaqueText<PixelTraits, SPT>, SPT>;

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value, the existing color and the given color.
 */
template<typename PixelTraits>
class PixelColoredAlpha {
  typedef typename PixelTraits::color_type color_type;
  typedef typename PixelTraits::channel_type channel_type;

  color_type color;

public:
  constexpr explicit PixelColoredAlpha(color_type _color):color(_color) {}

  constexpr color_type operator()(color_type a, Luminosity8 alpha) const {
    return PixelTraits::TransformChannels(a, color,
                                          [alpha](channel_type a,
                                                  channel_type color) {
        return a + ((int(color - a) * int(alpha.GetLuminosity())) >> 8);
      });
  }
};

template<typename PixelTraits, typename SPT>
using ColoredAlphaPixelOperations =
  BinaryPerPixelOperations<PixelTraits, PixelColoredAlpha<PixelTraits>, SPT>;

/**
 * The input buffer contains alpha values, and each pixel is blended
 * using the alpha value between the two given colors.
 */
template<typename PixelTraits>
class PixelOpaqueAlpha {
  typedef typename PixelTraits::color_type color_type;
  typedef typename PixelTraits::channel_type channel_type;

  const color_type a, b;

public:
  constexpr PixelOpaqueAlpha(color_type _a, color_type _b):a(_a), b(_b) {}

  constexpr color_type operator()(Luminosity8 alpha) const {
    return PixelTraits::TransformChannels(a, b,
                                          [alpha](channel_type a,
                                                  channel_type b) {
        return a + ((int(b - a) * int(alpha.GetLuminosity())) >> 8);
      });
  }
};

template<typename PixelTraits, typename SPT>
using OpaqueAlphaPixelOperations =
  UnaryPerPixelOperations<PixelTraits, PixelOpaqueAlpha<PixelTraits>, SPT>;

template<typename PixelTraits>
struct ColorKey {
  typedef typename PixelTraits::color_type color_type;
  typedef color_type argument_type;
  typedef bool result_type;

  argument_type key;

  explicit constexpr ColorKey(argument_type _key):key(_key) {}

  result_type operator()(argument_type c) const {
    return c != key;
  }
};

/**
 * Color keying: skip writing a pixel if the source color matches the
 * given color key.
 */
template<typename PixelTraits>
using TransparentPixelOperations =
  ConditionalPixelOperations<PixelTraits, ColorKey<PixelTraits>>;

template<typename PixelTraits>
class TransparentInvertPixelOperations
  : private PixelIntegerAdapter<PixelTraits,
                                PixelBitNot<typename PixelTraits::integer_type>> {
  typedef typename PixelTraits::pointer_type pointer_type;
  typedef typename PixelTraits::color_type color_type;

  color_type key;

public:
  constexpr TransparentInvertPixelOperations(color_type _key):key(_key) {}

  void WritePixel(pointer_type p, color_type c) const {
    if (c != key)
      PixelTraits::WritePixel(p, (*this)(c));
  }
};

#endif
