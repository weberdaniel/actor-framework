// This file is part of CAF, the C++ Actor Framework. See the file LICENSE in
// the main distribution directory for license terms and copyright or visit
// https://github.com/actor-framework/actor-framework/blob/master/LICENSE.

#pragma once

#include "caf/fwd.hpp"

namespace caf::async {

// -- classes ------------------------------------------------------------------

class batch;
class consumer;
class execution_context;
class producer;

// -- template classes ---------------------------------------------------------

template <class T>
class spsc_buffer;

template <class T>
class consumer_resource;

template <class T>
class producer_resource;

template <class T>
class promise;

template <class T>
class future;

// -- smart pointer aliases ----------------------------------------------------

using execution_context_ptr = intrusive_ptr<execution_context>;

// -- free function templates --------------------------------------------------

template <class T>
batch make_batch(span<const T> items);

} // namespace caf::async
