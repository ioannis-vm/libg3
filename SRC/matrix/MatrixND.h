/* 
 * Objectives:
 * - little to no overhead above C-style arrays
 * - value semantics; objects do not decay to pointers;
 *
 * This code is influenced by the following sources
 *  list initialization:
 *  - https://stackoverflow.com/questions/42068882/list-initialization-for-a-matrix-class
 *
 *  style/practices
 *  - https://quuxplusone.github.io/blog/2021/04/03/static-constexpr-whittling-knife/
 * 
 *  Operator overloading / semantics
 *  - https://stackoverflow.com/questions/9851188/does-it-make-sense-to-use-move-semantics-for-operator-and-or-operator/9851423#9851423
 *
 *  compile-time template restrictions/concepts:
 *  - https://codereview.stackexchange.com/questions/259038/compile-time-matrix-class
 *    (C++ 20)
 *  - https://github.com/calebzulawski/cotila/
 *    (C++ 17)
 */

// Claudio Perez
#ifndef MatrixND_H
#define MatrixND_H
#include <math.h>
#include <assert.h>
#include <array>
#include <stdexcept>
#include <functional>
#include <iostream> // overloading <<

#include "VectorND.h"
#include "Matrix.h"
#include "Vector.h"
#include "blasdecl.h"

#if __cplusplus < 202000L
#define consteval
#define requires(X)
#endif

#define G23_STACK_MAX 10

namespace OpenSees {

template <index_t NR, index_t NC, typename T=double>
requires(NR > 0 && NC > 0)
struct MatrixND {
  double values[NC][NR];

  // Convert to regular Matrix class
  operator Matrix() { return Matrix(&values[0][0], NR, NC);}

  //
  // Indexing
  //
  constexpr std::array<T, NC> &
  operator[](index_t index) {return values[index];}

  constexpr const std::array<T, NC> & // [i] indexing
  operator[](index_t index) const {return values[index];}
  
  constexpr T & // (i,j) indexing
  operator()(index_t index_r, index_t index_c) {
    assert(index_r >= 0 && index_c >= 0);
    assert(index_r < NR && index_c < NC);
    // column-major
    return values[index_c][index_r];
  }

  constexpr const T & // (i,j) indexing
  operator()(index_t index_r, index_t index_c) const {
    assert(index_r >= 0 && index_c >= 0);
    assert(index_r < NR && index_c < NC);
    return values[index_c][index_r];
  }

  constexpr VectorND<NR>
  column(index_t index) const {
    assert(index >= 0);
    assert(index < NC);
    // TODO: ugly temporary implementation
    return *(VectorND<NR>*)(&(values[index]));
  }

  constexpr VectorND<NC>
  row(index_t index) const {
    assert(index >= 0);
    assert(index < NR);

    VectorND<NC,T> rw;
    for (index_t j = 0; j < NC; ++j) {
      rw[j] = values[j][index];
    }
    return rw;
  }

  consteval VectorND<2,int>
  size() const {return {NR, NC};}

  //
  //
  //
  void zero(void)
  {
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] = 0.0;
      }
    }
  }

  constexpr MatrixND<NC, NR>
  transpose() const {
    MatrixND<NC, NR> result = {};
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        result.values[i][j] = values[j][i];
      }
    }
    return result;
  }

  constexpr T
  trace() const
  requires(NR == NC) 
  {
    T sum = 0.0;
    for (index_t i = 0; i < NR; ++i) {
      sum += values[i][i];
    }
    return sum;
  }

  constexpr T
  determinant() const
  requires(NR == NC && NR == 2)
  {
    return values[0][0] * values[1][1] - values[0][1] * values[1][0];
  }

  int solve(const VectorND<NR> &V, VectorND<NR> &res) const
    requires(NR == NC)
  {
    MatrixND<NR,NC> work = *this;
    int pivot_ind[NR];
    int nrhs = 1;
    int nr = NR;
    int nc = NC;
    int info = 0;
    res = V; // X will be overwritten with the solution
    DGESV(&nr, &nrhs, &work.values[0][0], &nr, pivot_ind, res.values, &nc, &info);
    return -abs(info);
  }

  int solve(const Vector &V, Vector &res) const
    requires(NR == NC)
  {
    MatrixND<NR,NC> work = *this;
    int pivot_ind[NR];
    int nrhs = 1;
    int nr = NR;
    int nc = NC;
    int info = 0;
    res = V; // X will be overwritten with the solution
    DGESV(&nr, &nrhs, work.values, &nr, &pivot_ind, res.theData, &nc, &info);
    return -abs(info);
  }

  int solve(const Matrix &M, Matrix &res)
  {
    MatrixND<NR,NC> work = *this;
    int pivot_ind[NR];
    int *nrhs = M.noCols();
    int nr = NR;
    int nc = NC;
    int info = 0;
    res = M; // M will be overwritten with the solution
    DGESV(&nr, &nrhs, &work(0,0), &nr, &pivot_ind, &res(0,0), &nc, &info);
    return -abs(info);
  }

#if 0
  int invert(MatrixND<NR,NC> &res) const
  {
    return 0;
  }
  int invert(Matrix &res) const
  {
    return 0;
  }
  int invert()
  {
    return 0;
  }

  int addMatrix(double alpha, const MatrixND &other, double beta) {
  }

  int addMatrixTranspose(double factThis, const Matrix &other, double factOther)
  {
  }
  // AB
  int addMatrixProduct(double factThis, const Matrix &A, const Matrix &B, double factOther)
  {
  }
  // A'B
  int addMatrixTransposeProduct(double factThis, const Matrix &A, const Matrix &B, double factOther)
  {
  }
  // A'BA
  int addMatrixTripleProduct(double factThis, const Matrix &A, const Matrix &B, double factOther)
  {
  }
  //A'BC
  int addMatrixTripleProduct(double factThis, const Matrix &A, const Matrix &B, const Matrix &C, double otherFact)
  {
  } 
#endif 

  constexpr MatrixND &
  operator=(const Matrix &other)
  {  
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] = other(i,j);
      }
    }
    return *this;
  }

  constexpr MatrixND &
  operator=(const MatrixND<NR,NC> &other)
  {  
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] = other.values[j][i];
      }
    }
    return *this;
  }

  constexpr MatrixND &
  operator+=(const double value) {
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] += value;
      }
    }
    return *this;
  }

  constexpr MatrixND &
  operator+=(const MatrixND &other) {
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] += other.values[j][i];
      }
    }
    return *this;
  }
  
  constexpr MatrixND &
  operator-=(const MatrixND &other) {
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] -= other.values[j][i];
      }
    }
    return *this;
  }

  constexpr MatrixND &
  operator*=(T const scalar) {
    for (index_t j = 0; j < NC; ++j){
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] *= scalar;
      }
    }
    return *this;
  }

  constexpr MatrixND &
  operator/=(T const scalar) {
    for (index_t j = 0; j < NC; ++j) {
      for (index_t i = 0; i < NR; ++i) {
        values[j][i] /= scalar;
      }
    }
    return *this;
  }


// Notes on operators:
// - define friend operators inside class for use as header-only library
// - LHS is passed by (copied) value
  friend constexpr MatrixND
  operator+(MatrixND left, const MatrixND &right) {left += right; return left;}

  friend constexpr MatrixND
  operator-(MatrixND left, const MatrixND &right) {left -= right; return left;}
  
  friend constexpr MatrixND // scalar * Matrix
  operator*(T scalar, MatrixND mat) {mat *= scalar; return mat;}

  friend constexpr MatrixND // Matrix * scalar
  operator*(MatrixND mat, T scalar) {mat *= scalar; return mat;}
  
  template <index_t J>
  constexpr friend MatrixND<NR, J>
  operator*(const MatrixND<NR, NC> &left, const MatrixND<NC, J> &right) {
    MatrixND<NR, J> prod;
//  prod.zero();
    for (index_t i = 0; i < NR; ++i) {
      for (index_t j = 0; j < J; ++j) {
        prod(i, j) = 0.0;
        for (index_t k = 0; k < NC; ++k) {
          prod(i, j) += left(i,k) * right(k,j);
        }
      }
    }
    return prod;
  }

  friend  VectorND<NR>
  operator*(const MatrixND<NR, NC> &left, Vector &right) {
    VectorND<NR> prod;
    for (index_t i = 0; i < NR; ++i) {
        prod[i] = 0.0;
        for (index_t k = 0; k < NC; ++k) {
          prod[i] += left(i,k) * right(k);
        }
    }
    return prod;
  }


  template <index_t J>
  friend  MatrixND<NR, J>
  operator*(const MatrixND<NR, NC> &left, const Matrix &right) {
    MatrixND<NR, J> prod;
    for (index_t i = 0; i < NR; ++i) {
      for (index_t j = 0; j < J; ++j) {
        prod(i, j) = 0.0;
        for (index_t k = 0; k < NC; ++k) {
          prod(i, j) += left(i,k) * right(k,j);
        }
      }
    }
    return prod;
  }

  friend constexpr MatrixND
  operator/(MatrixND mat, T scalar) {mat /= scalar; return mat;}
  
  friend std::ostream &
  operator<<(std::ostream &out, MatrixND const &mat) {
    out << "{";
    for (int r=0; r<NR; r++){
      out << "{";
      for (int c=0; c<NC; c++)
        out << mat(r,c);
      out << "}, ";
    }
    return out << "}\n";
  }

};

template <int nr, int nc, typename T=double>
MatrixND(const T (&)[nc][nr])->MatrixND<nr, nc, T>;

} // namespace OpenSees

#endif // MatrixND_H

