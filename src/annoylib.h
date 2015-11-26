// Copyright (c) 2013 Spotify AB
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#ifndef ANNOYLIB_H
#define ANNOYLIB_H

#include <stdio.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __MINGW32__
#include "mman.h"
#include <windows.h>
#else
#include <sys/mman.h>
#endif

#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <queue>
#include <limits>
#include "protobuf/annoy.pb.h"
#include "lmdbforest.h"

// This allows others to supply their own logger / error printer without
// requiring Annoy to import their headers. See RcppAnnoy for a use case.
#ifndef __ERROR_PRINTER_OVERRIDE__
  #define showUpdate(...) { fprintf(stderr, __VA_ARGS__ ); }
#else
  #define showUpdate(...) { __ERROR_PRINTER_OVERRIDE__( __VA_ARGS__ ); }
#endif

#ifndef ANNOY_NODE_ATTRIBUTE
  #define ANNOY_NODE_ATTRIBUTE __attribute__((__packed__))
  // TODO: this is turned on by default, but may not work for all architectures! Need to investigate.
#endif


using std::vector;
using std::string;
using std::pair;
using std::numeric_limits;
using std::make_pair;

struct RandRandom {
  // Default implementation of annoy-specific random number generator that uses rand() from standard library.
  // Owned by the AnnoyIndex, passed around to the distance metrics
  inline int flip() {
    // Draw random 0 or 1
    return rand() & 1;
  }
  inline size_t index(size_t n) {
    // Draw random integer between 0 and n-1 where n is at most the number of data points you have
    return rand() % n;
  }
};

template<typename T>
inline T get_norm(T* v, int f) {
  T sq_norm = 0;
  for (int z = 0; z < f; z++)
    sq_norm += v[z] * v[z];
  return sqrt(sq_norm);
}


inline float get_norm1(data_info& d, int f) {
  float sq_norm = 0;
  for (int z = 0; z < f; z++)
    sq_norm += d.data(z) * d.data(z);
  return sqrt(sq_norm);
}


template<typename T>
inline void normalize(T* v, int f) {
  T norm = get_norm(v, f);
  for (int z = 0; z < f; z++)
    v[z] /= norm;
}

inline void normalize(tree_node& node, int f, float norm) {
  for (int z = 0; z < f; z++)
    node.set_v(z, node.v(z)/ norm);
}

template<typename S, typename T, class Random>
struct Angular {
  struct ANNOY_NODE_ATTRIBUTE Node {
    /*
     * We store a binary tree where each node has two things
     * - A vector associated with it
     * - Two children
     * All nodes occupy the same amount of memory
     * All nodes with n_descendants == 1 are leaf nodes.
     * A memory optimization is that for nodes with 2 <= n_descendants <= K,
     * we skip the vector. Instead we store a list of all descendants. K is
     * determined by the number of items that fits in the space of the vector.
     * For nodes with n_descendants == 1 the vector is a data point.
     * For nodes with n_descendants > K the vector is the normal of the split plane.
     * Note that we can't really do sizeof(node<T>) because we cheat and allocate
     * more memory to be able to fit the vector outside
     */
    S n_descendants;
    S children[2]; // Will possibly store more than 2
    T v[1]; // We let this one overflow intentionally. Need to allocate at least 1 to make GCC happy
  };
  
  static inline T distance(const T* x , data_info& y, int f) {
    // want to calculate (a/|a| - b/|b|)^2
    // = a^2 / a^2 + b^2 / b^2 - 2ab/|a||b|
    // = 2 - 2cos
    T pp = 0, qq = 0, pq = 0;
    for (int z = 0; z < f; z++, x++) {
      pp += (*x) * (*x);
      qq += (y.data(z)) * (y.data(z));
      pq += (y.data(z)) * (*x);
    }
    //printf("%f, %f, %f\n", pp, qq, pq);
    T ppqq = pp * qq;
    if (ppqq > 0) return 2.0 - 2.0 * pq / sqrt(ppqq);
    else return 2.0; // cos is 0
  }

  static inline T distance(data_info& x , data_info& y, int f) {
    // want to calculate (a/|a| - b/|b|)^2
    // = a^2 / a^2 + b^2 / b^2 - 2ab/|a||b|
    // = 2 - 2cos
    T pp = 0, qq = 0, pq = 0;
    for (int z = 0; z < f; z++) {
      pp += (x.data(z)) * (x.data(z));
      qq += (y.data(z)) * (y.data(z));
      pq += (y.data(z)) * (x.data(z));
    }
   // printf("%f, %f, %f\n", pp, qq, pq);
    T ppqq = pp * qq;
    if (ppqq > 0) return 2.0 - 2.0 * pq / sqrt(ppqq);
    else return 2.0; // cos is 0
  }

  static inline T distance(const T* x, const T* y, int f) {
    // want to calculate (a/|a| - b/|b|)^2
    // = a^2 / a^2 + b^2 / b^2 - 2ab/|a||b|
    // = 2 - 2cos
    T pp = 0, qq = 0, pq = 0;
    for (int z = 0; z < f; z++, x++, y++) {
      pp += (*x) * (*x);
      qq += (*y) * (*y);
      pq += (*x) * (*y);
    }
    T ppqq = pp * qq;
    if (ppqq > 0) return 2.0 - 2.0 * pq / sqrt(ppqq);
    else return 2.0; // cos is 0
  }

  static inline T margin(const tree_node& tn, const T* y, int f) {
    T dot = 0;
    for (int z = 0; z < f; z++) {
      dot += tn.v(z) * y[z];
    }
   // dot += tn.t();
    return dot;
  }

  static inline T margin(const tree_node& tn, data_info& y, int f) {
    T dot = 0;
    for (int z = 0; z < f; z++) {
      dot += tn.v(z) * y.data(z);
    }
   // dot += tn.t();
    return dot;
  }

  static inline T margin(const Node* n, const T* y, int f) {
    T dot = 0;
    for (int z = 0; z < f; z++)
      dot += n->v[z] * y[z];
    return dot;
  }
  static inline bool side(const Node* n, const T* y, int f, Random& random) {
    T dot = margin(n, y, f);
    if (dot != 0)
      return (dot > 0);
    else
      return random.flip();
  }

  static inline bool side(const tree_node& tree, data_info& y, int f, Random& random) {
    T dot = margin(tree, y, f);
    if (dot != 0)
      return (dot > 0);
    else
      return random.flip();
  }


  static inline bool side(tree_node& tn, const T* y, int f, Random& random) {
    T dot = margin(tn, y, f);
    if (dot != 0)
      return (dot > 0);
    else
      return random.flip();
  }

  static inline void create_split(const vector<Node*>& nodes, int f, Random& random, Node* n) {
    // Sample two random points from the set of nodes
    // Calculate the hyperplane equidistant from them
    size_t count = nodes.size();
    size_t i = random.index(count);
    size_t j = random.index(count-1);
    j += (j >= i); // ensure that i != j
    T* iv = nodes[i]->v;
    T* jv = nodes[j]->v;
    T i_norm = get_norm(iv, f);
    T j_norm = get_norm(jv, f);
    for (int z = 0; z < f; z++)
      n->v[z] = iv[z] / i_norm - jv[z] / j_norm;
    normalize(n->v, f);
  }
  
  
  static inline void split(const tree_node& tn, const vector<data_info>& nodes,
                           tree_node& new_node,  tree_node& left_node, tree_node& right_node,
                           Random& random, int f) {
    
    // Sample two random points from the set of nodes
    // Calculate the hyperplane equidistant from them
    size_t count = nodes.size();
    size_t i = random.index(count);
    size_t j = random.index(count-1);
    j += (j >= i); // ensure that i != j
    
    //printf("use data %d and %d as pivot for splitting ... ", i, j);

    data_info iv = nodes[i];
    data_info jv = nodes[j];

    //iv.PrintDebugString();
    //jv.PrintDebugString();
    
    T i_norm = get_norm1(iv, f);
    T j_norm = get_norm1(jv, f);
    
    float squared_sum = 0;
    for (int z = 0; z < f; z++) {
      T d = iv.data(z) / i_norm - jv.data(z) / j_norm;
      new_node.add_v(d);
      squared_sum += d*d;
    }

    normalize(new_node, f, sqrt(squared_sum));
    new_node.set_t(0.0);
    new_node.set_leaf(false);
    left_node.set_leaf(true);
    right_node.set_leaf(true);


    T* w_data = new T[f];
    for (int w = 0; w < nodes.size(); w ++ ){
      data_info d = nodes[w];
      for (int s = 0; s < f; s ++) {
        w_data[s] = d.data(s);
      }


      bool sd = side(new_node, w_data, f, random);
      if (sd) {
        left_node.add_items(nodes[w].id());
      } else {
        right_node.add_items(nodes[w].id());
      } 
    }    
  }
  
  
  static inline T normalized_distance(T distance) {
    // Used when requesting distances from Python layer
    return sqrt(distance);
  }
};

template<typename S, typename T, class Random>
struct Euclidean {
  struct ANNOY_NODE_ATTRIBUTE Node {
    S n_descendants;
    T a; // need an extra constant term to determine the offset of the plane
    S children[2];
    T v[1];
  };
  
  static inline T distance(data_info& x, data_info&  y, int f) {
    T d = 0.0;
    for (int i = 0; i < f; i++)
      d += ((x.data(i)) - (y.data(i))) * ((x.data(i)) - (y.data(i)));
    return d;
  }
  static inline T distance(const T* x, data_info& y, int f) {
    T d = 0.0;
    for (int i = 0; i < f; i++, x++)
      d += ((*x) - (y.data(i))) * ((*x) - (y.data(i)));
    return d;
  }
 
  static inline T distance(data_info& x, const T*  y, int f) {
    T d = 0.0;
    for (int i = 0; i < f; i++, y++)
      d += ((x.data(i)) - (*y)) * ((x.data(i)) - (*y));
    return d;
  }

  static inline T distance(const T* x, const T* y, int f) {
    T d = 0.0;
    for (int i = 0; i < f; i++, x++, y++)
      d += ((*x) - (*y)) * ((*x) - (*y));
    return d;
  }

  static inline T margin(tree_node& tn, const T* y, int f) {
    T dot = tn.t();
    for (int z = 0; z < f; z++)
      dot += tn.v(z) * y[z];
    return dot;
  }
  static inline T margin(tree_node& tn, data_info& y, int f) {
    T dot = tn.t();
    for (int z = 0; z < f; z++)
      dot += tn.v(z) * y.data(z);
    return dot;
  }

  static inline T margin(const Node* n, const T* y, int f) {
    T dot = n->a;
    for (int z = 0; z < f; z++)
      dot += n->v[z] * y[z];
    return dot;
  }
  static inline bool side(const Node* n, const T* y, int f, Random& random) {
    T dot = margin(n, y, f);
    if (dot != 0)
      return (dot > 0);
    else
      return random.flip();
  }
  
  static inline bool side(tree_node& tn, data_info& y, int f, Random& random) {
    T dot = margin(tn, y, f);
    if (dot != 0)
      return (dot > 0);
    else
      return random.flip();
  }

  static inline void split(const tree_node& tn, const vector<data_info>& nodes,
                           tree_node& new_node,  tree_node& left_node, tree_node& right_node,
                           Random& random, int f) {
    // Same as Angular version, but no normalization and has to compute the offset too
    size_t count = tn.items_size();
    size_t i = random.index(count);
    size_t j = random.index(count-1);
    j += (j >= i); // ensure that i != j
  
    data_info iv = nodes[i];
    data_info jv = nodes[j];
  
    
    T t = 0.0;
    
    for (int z = 0; z < f; z++) {
      T d = iv.data(z) - jv.data(z);
      new_node.add_v(d);
      t += - d * (iv.data(z) + jv.data(z)) / 2;
    }
    new_node.set_t(t);
    new_node.set_leaf(false);
    new_node.set_left(left_node.index());
    new_node.set_right(right_node.index());
    left_node.set_leaf(true);
    right_node.set_leaf(true);
    
    //distribute the items into left and right children
    for (int w = 0; w < count; w ++ ) {
      data_info di = nodes[w];
      T dot = t;
      for (int z = 0; z < f; z ++ ) {
        dot += new_node.v(z) * di.data(z);
      }
      if (dot == 0) {
        dot = random.flip();
      }
      if (dot < 0) {
        left_node.add_items(tn.items(w));
      } else {
        right_node.add_items(tn.items(w));
      }
    }
    
    return; 
  }


  static inline void create_split(const vector<Node*>& nodes, int f, Random& random, Node* n) {
    // Same as Angular version, but no normalization and has to compute the offset too
    size_t count = nodes.size();
    size_t i = random.index(count);
    size_t j = random.index(count-1);
    j += (j >= i); // ensure that i != j
    T* iv = nodes[i]->v;
    T* jv = nodes[j]->v;
    n->a = 0.0;
    for (int z = 0; z < f; z++) {
      n->v[z] = iv[z] - jv[z];
      n->a += -n->v[z] * (iv[z] + jv[z]) / 2;
    }
  }
  static inline T normalized_distance(T distance) {
    return sqrt(distance);
  }
};

#endif
// vim: tabstop=2 shiftwidth=2
