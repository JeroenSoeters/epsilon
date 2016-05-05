

#include "epsilon/linear/dense_matrix_impl.h"

#include "epsilon/linear/lapack.h"
#include "epsilon/util/string.h"
#include "epsilon/vector/vector_util.h"

namespace linear_map {

DenseMatrixImpl::DenseMatrixImpl(const DenseMatrix& A)
    : LinearMapImpl(DENSE_MATRIX),
      m_(A.rows()),
      n_(A.cols()),
      data_ptr_(new Data()),
      trans_('N') {
  data_ptr_->data.reset(new Scalar[m_*n_]);
  memcpy(data(), A.data(), m_*n_*sizeof(Scalar));
}

LinearMapImpl* DenseMatrixImpl::Inverse() const {
  VLOG(1) << "Factoring " << m() << " x " << n();

  // NOTE(mwytock): This assumes matrix is symmetric, do we need non-symmetric?
  CHECK_EQ(m(), n());
  Eigen::LDLT<DenseMatrix> ldlt;
  ldlt.compute(dense());
  CHECK_EQ(ldlt.info(), Eigen::Success) << DebugString();
  return new DenseMatrixImpl(ldlt.solve(DenseMatrix::Identity(n(), n())));
}

std::string DenseMatrixImpl::DebugString() const {
  return "";
  // return StringPrintf(
  //     "dense matrix %d x %d\n%s", m(), n(), MatrixDebugString(A_).c_str());
}

bool DenseMatrixImpl::operator==(const LinearMapImpl& other) const {
  if (other.type() != DENSE_MATRIX ||
      other.m() != m() ||
      other.n() != n())
    return false;
  const DenseMatrixImpl& A = static_cast<const DenseMatrixImpl&>(other);
  return (A.trans_ == trans_ && A.data_ptr_.get() == data_ptr_.get());
}

DenseMatrixImpl::DenseVector DenseMatrixImpl::Apply(const DenseVector& x) const {
  double alpha = 1;
  double beta = 0;
  int* m = const_cast<int*>(&m_);
  int* n = const_cast<int*>(&n_);
  int incx = 1;
  int incy = 1;
  DenseVector y(m_);
  dgemv_(trans(), m, n, &alpha, data(), m,
         const_cast<double*>(x.data()), &incx, &beta,
         const_cast<double*>(y.data()), &incy);
  return y;
}

DenseMatrixImpl::DenseMatrix DenseMatrixImpl::AsDense() const {
  //if (trans_ == 'N') {
  return Eigen::Map<DenseMatrix>(data(), m_, n_);
  // } else {
  //   return Eigen::Map<DenseMatrix>(n_, m_, data_.get()).transpose();
  // }
}



}  // namespace
