
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <Eigen/Dense>

#include "epsilon/expression.pb.h"
#include "epsilon/expression/expression.h"
#include "epsilon/expression/expression_util.h"
#include "epsilon/expression/expression_testutil.h"
#include "epsilon/affine/affine.h"
#include "epsilon/vector/vector_util.h"
#include "epsilon/vector/vector_testutil.h"


void TestBuildAffineOperator(
    const Expression& expr,
    const Eigen::MatrixXd& expected_A,
    const Eigen::VectorXd& expected_b) {
  VariableOffsetMap var_map;
  var_map.Insert(expr);
  const int m = GetDimension(expr);
  const int n = var_map.n();

  DynamicMatrix A = DynamicMatrix::FromDense(Eigen::MatrixXd::Zero(m,n));
  DynamicMatrix b = DynamicMatrix::FromDense(Eigen::VectorXd::Zero(m));
  BuildAffineOperator(expr, var_map, &A, &b);
  EXPECT_TRUE(MatrixEquals(expected_A, A.dense(), 1e-3));
  EXPECT_TRUE(VectorEquals(expected_b, b.dense(), 1e-3));
}

TEST(BuildAffineOperator, VectorConstant) {
  TestBuildAffineOperator(
      TestConstant(TestVector({1,2,3})),
      TestMatrix({{},{},{}}),
      TestVector({1,2,3}));
}

TEST(BuildAffineOperator, MatrixConstant) {
  TestBuildAffineOperator(
      TestConstant(TestMatrix({{1,2,3},{4,5,6}})),
      TestMatrix({{},{},{},{},{},{}}),
      TestVector({1,4,2,5,3,6}));
}

TEST(BuildAffineOperator, IndexVectorConstant) {
  TestBuildAffineOperator(
      expression::Index(1, 2, TestConstant(TestVector({1,2,3}))),
      TestMatrix({{},{}}),
      TestVector({2,3}));
}

TEST(BuildAffineOperator, IndexMatrixConstant) {
  TestBuildAffineOperator(
      expression::Index(
          0, 1, 0, 2,
          TestConstant(TestMatrix({{1,2,3},{4,5,6}}))),
      TestMatrix({{},{}}),
      TestVector({1,2}));
}

// Test to make sure we're not doing anything dumb with memory
TEST(BuildAffineOperator, IndexMatrixConstant_Large) {
  const int m = 1000;
  const int n = 2000;
  Eigen::MatrixXd C = Eigen::MatrixXd::Constant(m, n, 1);
  TestBuildAffineOperator(
      expression::Index(0, 500, 1000, 500, TestConstant(C)),
      Eigen::MatrixXd(500*500, 0),
      Eigen::VectorXd::Constant(500*500, 1));
}

// Ax
TEST(BuildAffineOperator, MultiplyVectorVariable) {
  const int m = 3;
  const int n = 2;
  TestBuildAffineOperator(
      expression::Multiply(
          TestConstant(TestMatrix({{1,2},{3,4},{5,6}})),
          TestVariable(n, 1)),
      TestMatrix({{1,2},{3,4},{5,6}}),
      Eigen::VectorXd::Zero(m));
}

// AX
TEST(BuildAffineOperator, MultiplyMatrixVariable) {
  const int m = 4;
  const int n = 2;
  const int k = 3;
  TestBuildAffineOperator(
      expression::Multiply(
          TestConstant(TestMatrix({{1,2},{3,4},{5,6},{7,8}})),
          TestVariable(n, k)),
      BlockDiag(TestMatrix({{1,2},{3,4},{5,6},{7,8}}), k),
      Eigen::VectorXd::Zero(m*k));
}

TEST(BuildAffineOperator, HStack) {
  const int m = 3;
  const int n = 2;

  TestBuildAffineOperator(
      expression::HStack({
          expression::Variable(m, n, "x"),
          expression::Variable(m, n, "y")}),
      Eigen::MatrixXd::Identity(m*n*2, m*n*2),
      Eigen::VectorXd::Zero(m*n*2));
}

TEST(BuildAffineOperator, HStack_Offset) {
  const int m = 3;
  const int n = 2;

  Expression hstack = expression::HStack({expression::Variable(m, n, "y")});
  hstack.mutable_stack_params()->set_offset(2);
  *hstack.mutable_size() = CreateSize(3, 4);

  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(m*n*2, m*n);
  A.block(m*n, 0, m*n, m*n) = Eigen::MatrixXd::Identity(m*n, m*n);
  TestBuildAffineOperator(hstack, A, Eigen::VectorXd::Zero(m*n*2));
}

TEST(BuildAffineOperator, VStack) {
  const int m = 3;
  const int n = 2;

  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(m*n*2, m*n*2);
  A.block(0, 0, m, m) = Eigen::MatrixXd::Identity(3,3);
  A.block(m*n, m, m, m) = Eigen::MatrixXd::Identity(3,3);
  A.block(m, m*n, m, m) = Eigen::MatrixXd::Identity(3,3);
  A.block(m*m, m*m, m, m) = Eigen::MatrixXd::Identity(3,3);

  TestBuildAffineOperator(
      expression::VStack({
          expression::Variable(m, n, "x"),
          expression::Variable(m, n, "y")}),
      A, Eigen::VectorXd::Zero(m*n*2));
}

TEST(BuildAffineOperator, VStack_Offset) {
  const int m = 3;
  const int n = 2;

  Eigen::MatrixXd A = Eigen::MatrixXd::Zero(m*n*2, m*n);
  A.block(m, 0, m, m) = Eigen::MatrixXd::Identity(m,m);
  A.block(m*m, m, m, m) = Eigen::MatrixXd::Identity(m,m);

  Expression vstack = expression::VStack({expression::Variable(m, n, "y")});
  vstack.mutable_stack_params()->set_offset(3);
  *vstack.mutable_size() = CreateSize(6, 2);
  TestBuildAffineOperator(vstack, A, Eigen::VectorXd::Zero(m*n*2));
}


TEST(GetProjection, Basic) {
  VariableOffsetMap a;
  Expression x = expression::Variable(4, 1, "x");
  Expression y = expression::Variable(3, 1, "y");
  a.Insert(x);
  a.Insert(y);

  {
    VariableOffsetMap b;
    b.Insert(x);
    SparseXd P = GetProjection(a, b);
    Eigen::MatrixXd expected_P = Eigen::MatrixXd::Zero(4, 7);
    expected_P.block(0, 0, 4, 4) += Eigen::MatrixXd::Identity(4, 4);
    EXPECT_TRUE(MatrixEquals(expected_P, P));
  }

  {
    VariableOffsetMap b;
    b.Insert(y);
    SparseXd P = GetProjection(a, b);
    Eigen::MatrixXd expected_P = Eigen::MatrixXd::Zero(3, 7);
    expected_P.block(0, 4, 3, 3) += Eigen::MatrixXd::Identity(3, 3);
    EXPECT_TRUE(MatrixEquals(expected_P, P));
  }

}
