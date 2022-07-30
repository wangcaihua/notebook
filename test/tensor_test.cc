#include <gtest/gtest.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <array>
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/framework/tensor_shape.h"
#include "tensorflow/core/framework/tensor_types.h"
#include "tensorflow/core/framework/tensor_util.h"
#include "tensorflow/core/framework/types.h"
#include "tensorflow/core/lib/core/status_test_util.h"

/*
template <typename T, int NDIMS = 1, typename IndexType = Eigen::DenseIndex>
struct TTypes {
  // Scalar tensor (implemented as a rank-0 tensor) of scalar type T.
  typedef Eigen::TensorMap<
      Eigen::TensorFixedSize<T, Eigen::Sizes<>, Eigen::RowMajor, IndexType>,
      Eigen::Aligned>
      Scalar;
  typedef Eigen::TensorMap<Eigen::TensorFixedSize<const T, Eigen::Sizes<>,
                                                  Eigen::RowMajor, IndexType>,
                           Eigen::Aligned>
      ConstScalar;

  // Rank-1 tensor (vector) of scalar type T.
  typedef Eigen::TensorMap<Eigen::Tensor<T, 1, Eigen::RowMajor, IndexType>,
                           Eigen::Aligned>
      Flat;
  typedef Eigen::TensorMap<
      Eigen::Tensor<const T, 1, Eigen::RowMajor, IndexType>, Eigen::Aligned>
      ConstFlat;

  typedef Eigen::TensorMap<Eigen::Tensor<T, 1, Eigen::RowMajor, IndexType>,
                           Eigen::Aligned>
      Vec;
  typedef Eigen::TensorMap<
      Eigen::Tensor<const T, 1, Eigen::RowMajor, IndexType>, Eigen::Aligned>
      ConstVec;

  // Rank-2 tensor (matrix) of scalar type T.
  typedef Eigen::TensorMap<Eigen::Tensor<T, 2, Eigen::RowMajor, IndexType>,
                           Eigen::Aligned>
      Matrix;
  typedef Eigen::TensorMap<
      Eigen::Tensor<const T, 2, Eigen::RowMajor, IndexType>, Eigen::Aligned>
      ConstMatrix;

  // Rank-<NDIMS> tensor of scalar type T.
  typedef Eigen::TensorMap<Eigen::Tensor<T, NDIMS, Eigen::RowMajor, IndexType>,
                           Eigen::Aligned>
      Tensor;
  typedef Eigen::TensorMap<
      Eigen::Tensor<const T, NDIMS, Eigen::RowMajor, IndexType>, Eigen::Aligned>
      ConstTensor;
};
*/

/*
 * TensorMap, 即将一个指针与dims信息(m_data, m_dimensions) 应射成一个Tensor Like对象, 
 * 并提供索引与赋值能力, 主要可以修改值.
 * 
 * TensorBase提供了一系列函数, 但它只要
 */
namespace tensorflow {
TEST(Tensor, CreateAndInitialization) {
  Tensor tensor(DT_INT64, TensorShape({4, 5}));
  // 从Tensor中获取基本信息
  std::cout << "NumElements: "<< tensor.NumElements() << ", TotalBytes: " << tensor.TotalBytes() << std::endl;
  std::cout << "Num of Dims: " << tensor.dims() << ", Shape is " << tensor.shape() << std::endl;
  std::cout << "The first dim is " << tensor.dim_size(0) << ", The second dim is " << tensor.dim_size(1) << std::endl;

  /*
    scala<T>() -> TTypes<T>::Scalar, TTypes<T>::ConstScalar
    vec<T>() -> TTypes<T>::Vec, TTypes<T>::ConstVec
    matrix<T>() -> TTypes<T>::Matrix, TTypes<T>::ConstMatrix
    tensor<T, NDIMS>() -> TTypes<T, NDIMS>::Tensor, TTypes<T, NDIMS>::ConstTensor
    flat<T>() -> TTypes<T>::ConstFlat, TTypes<T>::Flat
  */

 TTypes<int64_t>::Matrix mat = tensor.matrix<int64_t>();
 
 // TTypes<int64_t>::Matrix 很小, 只有24个byte, 只tensor的一个view
 std::cout << "sizeof mat is " << sizeof(mat) << std::endl;

  // 几种常用的初始化方法
  // setConstant(const Scalar& val)
  // setZero()
  // setRandom()

  mat.setConstant(12);  // inplace修改, 并返回当前对象的引用
  std::cout << "setConstant: " << std::endl << mat << std::endl;

  mat.setZero();  // inplace修改, 并返回当前对象的引用
  std::cout << "setZero: " << std::endl << mat << std::endl;

  mat.setRandom();  // inplace修改, 并返回当前对象的引用
  std::cout << "setRandom: " << std::endl << mat << std::endl;
}

TEST(Tensor, GeometricalOperations) {
  Tensor tensor(DT_FLOAT, TensorShape({4, 5}));
  // Eigen::TensorMap<Eigen::Tensor<T, 2, Eigen::RowMajor, IndexType>,
  //                  Eigen::Aligned>
  TTypes<float_t>::Matrix mat = tensor.matrix<float_t>();
  mat.setRandom();
  std::cout << "tensor is " << std::endl << mat << std::endl;
  mat(1, 3) = 20;

  // reshape, 返回一个reshaped matirx, 原matrix shape不变, 它们都是tensor的view
  std::array<int, 2> new_dims{{2, 10}};
  mat = mat.reshape(new_dims);
  std::cout<< "reshape" << std::endl << mat << std::endl;
  std::cout<< "origin" << std::endl << mat <<std::endl;

  // mat.slice(offsets, extents), 用于切片, offsets是开始点, extents是切取的块大小
  Eigen::array<Eigen::Index, 2> offsets = {1, 1};
  Eigen::array<Eigen::Index, 2> extents = {2, 3};
  // Eigen::TensorSlicingOp<const std::array<long, 2>,  -> StartIndices
  //                        const std::array<long, 2>,  -> Sizes
  //                        Eigen::TensorMap<Eigen::Tensor<float, 2, 1>, 16>  0>
  //                       >
  auto sliced = mat.slice(offsets, extents);
  std::cout << "slice startIndiecs {1, 1} sizes {2, 3}" << std::endl << sliced << std::endl;

  // mat.chip(offset, dim), slice的特例, 用于取一行或一列数据
  std::cout << "chip, the second column" << std::endl << mat.chip(1, 1) << std::endl;
  std::cout << "chip, the third row" << std::endl << mat.chip(2, 0) << std::endl;
  std::cout<< "chip" << std::endl << mat.chip(1, 1) <<std::endl;

}
}  // namespace tensorflow