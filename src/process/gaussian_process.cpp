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
// Defines the GaussianProcess class.
//
///////////////////////////////////////////////////////////////////////////////

#include <process/gaussian_process.hpp>
#include <optimization/cost_functors.hpp>

#include <ceres/ceres.h>
#include <random>

namespace gp {

  GaussianProcess::GaussianProcess(const Kernel::Ptr& kernel, double noise,
                                   size_t dimension, size_t max_points)
    : kernel_(kernel),
      noise_(noise),
      dimension_(dimension),
      points_(new std::vector<VectorXd>),
      max_points_(max_points),
      targets_(max_points),
      regressed_(max_points),
      covariance_(max_points, max_points) {
    CHECK_NOTNULL(kernel_.get());
    CHECK_GE(max_points_, 1);
    CHECK_GE(dimension_, 1);
    CHECK_GT(noise_, 0.0);

    // Random number generator.
    std::random_device rd;
    std::default_random_engine rng; //(rd()); uncomment this to change the random seed. 
    std::uniform_real_distribution<double> unif(-1.0, 1.0);
    std::normal_distribution<double> normal(0.0, 0.1);

    // Populate 'points_' and 'targets_'.
    points_->reserve(max_points_);
    targets_.setZero();
    covariance_.setZero();
    regressed_.setZero();
    
    //for (size_t ii = 0; ii < max_points / 10 + 1; ii++) {
    //  VectorXd x(dimension);
    //
    //  for (size_t jj = 0; jj < dimension_; jj++)
    //    x(jj) = unif(rng);
    //
    //  points_->push_back(x);
    //  targets_(ii) = normal(rng);
    //}

    // Compute covariance matrix.
    Covariance();

    // Compute Cholesky decomposition of covariance matrix.
    llt_.compute(covariance_.topLeftCorner(points_->size(), points_->size()));

    // Compute regressed targets.
    regressed_.head(points_->size()) =
      llt_.solve(targets_.head(points_->size()));
  }

  GaussianProcess::GaussianProcess(const Kernel::Ptr& kernel, double noise,
                                   const PointSet& points, size_t max_points)
    : kernel_(kernel),
      noise_(noise),
      dimension_(0),
      points_(points),
      max_points_(max_points),
      targets_(max_points),
      regressed_(max_points),
      covariance_(max_points, max_points) {
    CHECK_NOTNULL(kernel_.get());
    CHECK_NOTNULL(points_.get());
    CHECK_GE(points_->size(), 1);
    CHECK_GE(max_points_, 1);
    CHECK_LE(points_->size(), max_points_);
    CHECK_GT(noise_, 0.0);

    // Set dimension.
    dimension_ = points_->at(0).size();
    covariance_.setZero();
    regressed_.setZero();

    // Random number generator.
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::normal_distribution<double> normal(0.0, 0.1);

    // Populate 'targets_'.
    for (size_t ii = 0; ii < points_->size(); ii++)
      targets_(ii) = normal(rng);

    // Compute covariance matrix.
    Covariance();

    // Compute Cholesky decomposition of covariance matrix.
    llt_.compute(covariance_.topLeftCorner(points_->size(), points_->size()));

    // Compute regressed targets.
    regressed_.head(points_->size()) =
      llt_.solve(targets_.head(points_->size()));
  }

  GaussianProcess::GaussianProcess(const Kernel::Ptr& kernel, double noise,
                                   const PointSet& points,
                                   const VectorXd& targets,
                                   size_t max_points)
    : kernel_(kernel),
      noise_(noise),
      points_(points),
      max_points_(max_points),
      targets_(max_points),
      regressed_(max_points),
      covariance_(max_points, max_points) {
    CHECK_NOTNULL(kernel_.get());
    CHECK_GE(max_points_, 1);
    CHECK_GE(points_->size(), 1);
    CHECK_LE(points_->size(), max_points_);
    CHECK_EQ(points_->size(), targets.size());
    CHECK_GT(noise_, 0.0);

    // Set dimension.
    dimension_ = points_->at(0).size();
    covariance_.setZero();
    regressed_.setZero();

    // Set 'targets_'.
    targets_.head(points_->size()) = targets;

    // Compute covariance matrix.
    Covariance();

    // Compute Cholesky decomposition of covariance matrix.
    llt_.compute(covariance_.topLeftCorner(points_->size(), points_->size()));

    // Compute regressed targets.
    regressed_.head(points_->size()) =
      llt_.solve(targets_.head(points_->size()));
  }

  // Evaluate mean and variance at a point.
  void GaussianProcess::Evaluate(const VectorXd& x,
                                 double& mean, double& variance) const {
    // Compute cross covariance.
    VectorXd cross(points_->size());
    cross.setZero();
    CrossCovariance(x, cross);

    // Compute mean and variance.
    mean = cross.dot(regressed_.head(points_->size()));
    variance = 1.0 - cross.dot(llt_.solve(cross));
  }

  // Evaluate at the ii'th training point.
  void GaussianProcess::EvaluateTrainingPoint(
     size_t ii, double& mean, double& variance) const {
    CHECK_LT(ii, points_->size());

    // Extract cross covariance (must subtract off added noise).
    VectorXd cross = covariance_.col(ii);
    cross(ii) -= noise_;

    // Compute mean and variance.
    mean = cross.dot(regressed_);
    variance = 1.0 - cross.dot(llt_.solve(cross));
  }

  // Add new point(s). Returns whether or not points were added (points will
  // only be added until 'max_points' is reached).
  bool GaussianProcess::Add(const VectorXd& x, double target) {
    const size_t N = points_->size();

    if (N < max_points_) {
      CHECK_EQ(x.size(), dimension_);

      // Add a row/column to the covariance matrix.
      for (size_t ii = 0; ii < N; ii++) {
        covariance_(ii, N) = kernel_->Evaluate(x, points_->at(ii));
        covariance_(N, ii) = covariance_(ii, N);
      }

      covariance_(N, N) = 1.0 + noise_;

      // Add the new point/target.
      targets_(N) = target;
      points_->push_back(x);

      // Recompute Cholesky decomposition and regressed targets.
      llt_.compute(covariance_.topLeftCorner(N + 1, N + 1));
      regressed_.head(N + 1) = llt_.solve(targets_.head(N + 1));

      return true;
    }

    return false;
  }

  // Add new point(s). Returns whether or not points were added (points will
  // only be added until 'max_points' is reached).
  bool GaussianProcess::Add(const std::vector<VectorXd>& points,
                            const VectorXd& targets) {
    CHECK_EQ(points.size(), targets.size());
    const bool has_room = points_->size() + points.size() <= max_points_;

    // Add points one at a time.
    for (size_t ii = 0; ii < points.size(); ii++) {
      CHECK_EQ(points[ii].size(), dimension_);
      const size_t N = points_->size();

      if (N >= max_points_)
        break;

      // Add a row/column to the covariance matrix.
      for (size_t jj = 0; jj < N; jj++) {
        covariance_(jj, N) = kernel_->Evaluate(points[ii], points_->at(jj));
        covariance_(N, jj) = covariance_(jj, N);
      }

      covariance_(N, N) = 1.0 + noise_;

      // Add the new point/target.
      targets_(N) = targets(ii);
      points_->push_back(points[ii]);
    }

    // Recompute Cholesky decomposition and regressed targets.
    llt_.compute(covariance_.topLeftCorner(points_->size(), points_->size()));

    regressed_.head(points_->size()) =
      llt_.solve(targets_.head(points_->size()));

    return has_room;
  }

  // Update the training targets in the direction of the gradient of the
  // mean squared error at the given points. Returns the mean squared error.
  // If 'finalize' is set, computes regressed targets - only set to false if
  // you are doing repeated updates, and be sure to set true on final update.
  double GaussianProcess::UpdateTargets(const std::vector<VectorXd>& points,
                                        const std::vector<double>& targets,
                                        double step_size, bool finalize) {
    CHECK_EQ(points.size(), targets.size());

    // Initialize 'mse' and 'grad' to zero.
    double mse = 0.0;
    VectorXd grad(VectorXd::Zero(targets_.size()));

    // Accumulate across all points/targets.
    VectorXd cross(points_->size());
    VectorXd regressed_cross(points_->size());
    for (size_t ii = 0; ii < points.size(); ii++) {
      // Get the cross covariance against the training points.
      CrossCovariance(points[ii], cross);

      // Compute regressed cross covariance.
      regressed_cross = llt_.solve(cross);

      // Compute error and accumulate MSE and gradient.
      const double error =
        regressed_cross.dot(targets_.head(points_->size())) - targets[ii];

      mse += error * error;
      grad += error * regressed_cross;
    }

    // Tack on constants.
    mse /= static_cast<double>(points.size());
    grad *= 2.0 / static_cast<double>(points.size());

    // Gradient update.
    targets_.head(points_->size()) -= step_size * grad;

    // Maybe update regressed targets.
    if (finalize)
      regressed_.head(points_->size()) =
        llt_.solve(targets_.head(points_->size()));

    return mse;
  }


  // Learn kernel hyperparameters by maximizing the log-likelihood of the
  // training data.
  bool GaussianProcess::LearnHyperparams() {
    // Create a Ceres problem.
    TrainingLogLikelihood* cost =
      new TrainingLogLikelihood(points_, &targets_, kernel_, noise_);
    ceres::GradientProblem problem(cost);

    // Create a parameter vector.
    VectorXd& kernel_params = kernel_->Params();
    double parameters[kernel_params.size()];
    for (size_t ii = 0; ii < kernel_params.size(); ii++)
      parameters[ii] = kernel_params(ii);

    // Set solver parameters.
    ceres::GradientProblemSolver::Summary summary;
    ceres::GradientProblemSolver::Options options;
    options.minimizer_progress_to_stdout = false;
    options.max_num_iterations = 100;
    options.max_num_line_search_step_size_iterations = 50;
    options.max_num_line_search_direction_restarts = 25;
    options.max_lbfgs_rank = 15;
    //    options.line_search_type = ceres::ARMIJO;
    //    options.line_search_direction_type = ceres::NONLINEAR_CONJUGATE_GRADIENT;

    ceres::Solve(options, problem, parameters, &summary);

    // Store the parameters back in the kernel.
    for (size_t ii = 0; ii < kernel_params.size(); ii++)
      kernel_params(ii) = parameters[ii];

    // Recompute covariance, cholesky, and regressed targets.
    Covariance();

    llt_.compute(covariance_.topLeftCorner(points_->size(), points_->size()));

    regressed_.head(points_->size()) =
      llt_.solve(targets_.head(points_->size()));

    return summary.IsSolutionUsable();
  }

  // Compute the covariance and cross covariance against the training points.
  void GaussianProcess::Covariance() {
    for (size_t ii = 0; ii < points_->size(); ii++) {
      covariance_(ii, ii) = 1.0 + noise_;

      for (size_t jj = 0; jj < ii; jj++) {
        covariance_(ii, jj) =
          kernel_->Evaluate(points_->at(ii), points_->at(jj));
        covariance_(jj, ii) = covariance_(ii, jj);
      }
    }
  }

  void GaussianProcess::CrossCovariance(const VectorXd& x, VectorXd& cross) const {
    for (size_t ii = 0; ii < points_->size(); ii++)
      cross(ii) = kernel_->Evaluate(points_->at(ii), x);
  }
}  //\namespace gp
