/*
 * Copyright (c) 2017, The Regents of the University of California (Regents).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *    3. Neither the name of the copyright holder nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS AS IS
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Please contact the author(s) of this library if you have any questions.
 * Authors: David Fridovich-Keil   ( dfk@eecs.berkeley.edu )
 */
///////////////////////////////////////////////////////////////////////////////
//
// Defines the RbfKernel class, which is derived from the Kernel base class.
// The RBF kernel is a function k(x, y) = exp(-0.5 (x-y)^T inv(L) (x-y)), where
// x, y are points in R^n and L is a diagonal matrix of squared length scales.
//
///////////////////////////////////////////////////////////////////////////////

#include <kernels/rbf_kernel.hpp>

#include <math.h>

namespace gp {

  // Factory method.
  Kernel::Ptr RbfKernel::Create(const VectorXd& lengths) {
    Kernel::Ptr ptr(new RbfKernel(lengths));
    return ptr;
  }

  // Constructor.
  RbfKernel::RbfKernel(const VectorXd& lengths)
    : Kernel(lengths) {}

  // Pure virtual methods to be implemented in a derived class.
  double RbfKernel::Evaluate(const VectorXd& x, const VectorXd& y) const {
    const VectorXd diff = x - y;

    return std::exp(-0.5 * diff.cwiseQuotient(params_).squaredNorm());
  }

  double RbfKernel::Partial(const VectorXd& x, const VectorXd& y,
                            size_t ii) const {
    CHECK_LT(ii, params_.size());
    const VectorXd diff = x - y;

    // Evaluate the kernel.
    const double kernel =
      std::exp(-0.5 * diff.cwiseQuotient(params_).squaredNorm());

    return kernel * diff(ii) * diff(ii) /
      (params_(ii) * params_(ii) * params_(ii));
  }

  void RbfKernel::Gradient(const VectorXd& x, const VectorXd& y,
                           VectorXd& gradient) const {
    const VectorXd diff = x - y;

    // Evaluate the kernel.
    const double kernel =
      std::exp(-0.5 * diff.cwiseQuotient(params_).squaredNorm());

    gradient = kernel * diff.cwiseProduct(diff).cwiseQuotient(
      params_.cwiseProduct(params_).cwiseProduct(params_));
  }


}  //\namespace gp
