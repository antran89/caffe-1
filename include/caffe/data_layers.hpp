#ifndef CAFFE_DATA_LAYERS_HPP_
#define CAFFE_DATA_LAYERS_HPP_

#include <string>
#include <utility>
#include <vector>

#include "boost/scoped_ptr.hpp"
#include "hdf5.h"

#include "caffe/blob.hpp"
#include "caffe/common.hpp"
#include "caffe/data_transformer.hpp"
#include "caffe/dataset.hpp"
#include "caffe/filler.hpp"
#include "caffe/internal_thread.hpp"
#include "caffe/layer.hpp"
#include "caffe/proto/caffe.pb.h"

namespace caffe {

/**
 * @brief Provides base for data layers that feed blobs to the Net.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class BaseDataLayer : public Layer<Dtype> {
 public:
  explicit BaseDataLayer(const LayerParameter& param);
  virtual ~BaseDataLayer() {}
  // LayerSetUp: implements common data layer setup functionality, and calls
  // DataLayerSetUp to do special data layer setup for individual layer types.
  // This method may not be overridden except by the BasePrefetchingDataLayer.
  virtual void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {}
  // Data layers have no bottoms, so reshaping is trivial.
  virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {}

  virtual void Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {}
  virtual void Backward_gpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {}

 protected:
  TransformationParameter transform_param_;
  DataTransformer<Dtype> data_transformer_;
  Caffe::Phase phase_;
  bool output_labels_;
};

template <typename Dtype>
class BasePrefetchingDataLayer :
    public BaseDataLayer<Dtype>, public InternalThread {
 public:
  explicit BasePrefetchingDataLayer(const LayerParameter& param)
      : BaseDataLayer<Dtype>(param) {}
  virtual ~BasePrefetchingDataLayer() {}
  // LayerSetUp: implements common data layer setup functionality, and calls
  // DataLayerSetUp to do special data layer setup for individual layer types.
  // This method may not be overridden.
  void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual void CreatePrefetchThread();
  virtual void JoinPrefetchThread();
  // The thread's function
  virtual void InternalThreadEntry() {}

 protected:
  Blob<Dtype> prefetch_data_;
  Blob<Dtype> prefetch_label_;
  Blob<Dtype> transformed_data_;
};

template <typename Dtype>
class DataLayer : public BasePrefetchingDataLayer<Dtype> {
 public:
  explicit DataLayer(const LayerParameter& param)
      : BasePrefetchingDataLayer<Dtype>(param) {}
  virtual ~DataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int MinTopBlobs() const { return 1; }
  virtual inline int MaxTopBlobs() const { return 2; }

 protected:
  virtual void InternalThreadEntry();

  shared_ptr<Dataset<string, Datum> > dataset_;
  Dataset<string, Datum>::const_iterator iter_;
};

/**
 * @brief Provides data to the Net generated by a Filler.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class DummyDataLayer : public Layer<Dtype> {
 public:
  explicit DummyDataLayer(const LayerParameter& param)
      : Layer<Dtype>(param) {}
  virtual void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  // Data layers have no bottoms, so reshaping is trivial.
  virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {}

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_DUMMY_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int MinTopBlobs() const { return 1; }

 protected:
  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {}
  virtual void Backward_gpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {}

  vector<shared_ptr<Filler<Dtype> > > fillers_;
  vector<bool> refill_;
};

/**
 * @brief Provides data to the Net from HDF5 files.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class HDF5DataLayer : public Layer<Dtype> {
 public:
  explicit HDF5DataLayer(const LayerParameter& param)
      : Layer<Dtype>(param) {}
  virtual ~HDF5DataLayer();
  virtual void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  // Data layers have no bottoms, so reshaping is trivial.
  virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {}

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_HDF5_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int MinTopBlobs() const { return 1; }

 protected:
  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {}
  virtual void Backward_gpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom) {}
  virtual void LoadHDF5FileData(const char* filename);

  std::vector<std::string> hdf_filenames_;
  unsigned int num_files_;
  unsigned int current_file_;
  hsize_t current_row_;
  std::vector<shared_ptr<Blob<Dtype> > > hdf_blobs_;
};

/**
 * @brief Write blobs to disk as HDF5 files.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class HDF5OutputLayer : public Layer<Dtype> {
 public:
  explicit HDF5OutputLayer(const LayerParameter& param);
  virtual ~HDF5OutputLayer();
  virtual void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {}
  // Data layers have no bottoms, so reshaping is trivial.
  virtual void Reshape(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top) {}

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_HDF5_OUTPUT;
  }
  // TODO: no limit on the number of blobs
  virtual inline int ExactNumBottomBlobs() const { return 2; }
  virtual inline int ExactNumTopBlobs() const { return 0; }

  inline std::string file_name() const { return file_name_; }

 protected:
  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Backward_cpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom);
  virtual void Backward_gpu(const vector<Blob<Dtype>*>& top,
      const vector<bool>& propagate_down, const vector<Blob<Dtype>*>& bottom);
  virtual void SaveBlobs();

  std::string file_name_;
  hid_t file_id_;
  Blob<Dtype> data_blob_;
  Blob<Dtype> label_blob_;
};

/**
 * @brief Provides data to the Net from image files.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class ImageDataLayer : public BasePrefetchingDataLayer<Dtype> {
 public:
  explicit ImageDataLayer(const LayerParameter& param)
      : BasePrefetchingDataLayer<Dtype>(param) {}
  virtual ~ImageDataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 2; }

 protected:
  shared_ptr<Caffe::RNG> prefetch_rng_;
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

  vector<std::pair<std::string, int> > lines_;
  int lines_id_;
};

/**
 * @brief Provides data to the Net from memory.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class MemoryDataLayer : public BaseDataLayer<Dtype> {
 public:
  explicit MemoryDataLayer(const LayerParameter& param)
      : BaseDataLayer<Dtype>(param), has_new_data_(false) {}
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_MEMORY_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 2; }

  virtual void AddDatumVector(const vector<Datum>& datum_vector);

  // Reset should accept const pointers, but can't, because the memory
  //  will be given to Blob, which is mutable
  void Reset(Dtype* data, Dtype* label, int n);

  int batch_size() { return batch_size_; }
  int channels() { return channels_; }
  int height() { return height_; }
  int width() { return width_; }

 protected:
  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  int batch_size_, channels_, height_, width_, size_;
  Dtype* data_;
  Dtype* labels_;
  int n_;
  int pos_;
  Blob<Dtype> added_data_;
  Blob<Dtype> added_label_;
  bool has_new_data_;
};

/**
 * @brief Provides data to the Net from windows of images files, specified
 *        by a window data file.
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class WindowDataLayer : public BasePrefetchingDataLayer<Dtype> {
 public:
  explicit WindowDataLayer(const LayerParameter& param)
      : BasePrefetchingDataLayer<Dtype>(param) {}
  virtual ~WindowDataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_WINDOW_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 2; }

 protected:
  virtual unsigned int PrefetchRand();
  virtual void InternalThreadEntry();

  shared_ptr<Caffe::RNG> prefetch_rng_;
  vector<std::pair<std::string, vector<int> > > image_database_;
  enum WindowField { IMAGE_INDEX, LABEL, OVERLAP, X1, Y1, X2, Y2, NUM };
  vector<vector<float> > fg_windows_;
  vector<vector<float> > bg_windows_;
  Blob<Dtype> data_mean_;
  vector<Dtype> mean_values_;
  bool has_mean_file_;
  bool has_mean_values_;
  bool cache_images_;
  vector<std::pair<std::string, Datum > > image_database_cache_;
};
/** hyeonwoonoh add
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class ImageSegCtrlPrefetchingDataLayer : public BasePrefetchingDataLayer<Dtype> {
 public:
  explicit ImageSegCtrlPrefetchingDataLayer(const LayerParameter& param)
      : BasePrefetchingDataLayer<Dtype>(param) {}
  virtual ~ImageSegCtrlPrefetchingDataLayer() {}
  // LayerSetUp: implements common data layer setup functionality, and calls
  // DataLayerSetUp to do special data layer setup for individual layer types.
  // This method may not be overridden.
  void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  // The thread's function
  virtual void InternalThreadEntry() {}

 protected:
  Blob<Dtype> prefetch_ctrl_data_;
  bool output_ctrl_data_;
};

/** hyeonwoonoh add
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class ImageSegCtrlClsPrefetchingDataLayer : public BasePrefetchingDataLayer<Dtype> {
 public:
  explicit ImageSegCtrlClsPrefetchingDataLayer(const LayerParameter& param)
      : BasePrefetchingDataLayer<Dtype>(param) {}
  virtual ~ImageSegCtrlClsPrefetchingDataLayer() {}
  // LayerSetUp: implements common data layer setup functionality, and calls
  // DataLayerSetUp to do special data layer setup for individual layer types.
  // This method may not be overridden.
  void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  // The thread's function
  virtual void InternalThreadEntry() {}

 protected:
  Blob<Dtype> prefetch_ctrl_data_;
  bool output_ctrl_data_;
  Blob<Dtype> prefetch_cls_data_;
  bool output_cls_data_;
};

/** Jay add
 * @brief prefetching data layer which also prefetches data dimensions
 *
 * TODO(dox): thorough documentation for Forward and proto params.
 */
template <typename Dtype>
class ImageDimPrefetchingDataLayer : public BasePrefetchingDataLayer<Dtype> {
/*
 notice:
 this code is based on the following implementation.
 https://bitbucket.org/deeplab/deeplab-public/
 */

 public:
  explicit ImageDimPrefetchingDataLayer(const LayerParameter& param)
      : BasePrefetchingDataLayer<Dtype>(param) {}
  virtual ~ImageDimPrefetchingDataLayer() {}
  // LayerSetUp: implements common data layer setup functionality, and calls
  // DataLayerSetUp to do special data layer setup for individual layer types.
  // This method may not be overridden.
  void LayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual void Forward_cpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);
  virtual void Forward_gpu(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  // The thread's function
  virtual void InternalThreadEntry() {}

 protected:
  Blob<Dtype> prefetch_data_dim_;
  bool output_data_dim_;
};

template <typename Dtype>
class ImageSegDataLayer : public ImageDimPrefetchingDataLayer<Dtype> {
 public:
  explicit ImageSegDataLayer(const LayerParameter& param)
    : ImageDimPrefetchingDataLayer<Dtype>(param) {}
  virtual ~ImageSegDataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> transformed_label_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  vector<std::pair<std::string, std::string> > lines_;
  int lines_id_;
};

template <typename Dtype>
class WindowSegDataLayer : public ImageDimPrefetchingDataLayer<Dtype> {
 public:
  explicit WindowSegDataLayer(const LayerParameter& param)
    : ImageDimPrefetchingDataLayer<Dtype>(param) {}
  virtual ~WindowSegDataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> transformed_label_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  typedef struct SegItems {
    std::string imgfn;
    std::string segfn;
    int x1, y1, x2, y2;
  } SEGITEMS;

  vector<SEGITEMS> lines_;
  int lines_id_;
};

template <typename Dtype>
class WindowSegBinaryLayer : public ImageDimPrefetchingDataLayer<Dtype> {
 public:
  explicit WindowSegBinaryLayer(const LayerParameter& param)
    : ImageDimPrefetchingDataLayer<Dtype>(param) {}
  virtual ~WindowSegBinaryLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> transformed_label_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  typedef struct SegItems {
    std::string imgfn;
    std::string segfn;
    int x1, y1, x2, y2;
  } SEGITEMS;

  vector<SEGITEMS> lines_;
  int lines_id_;
};

/*
 * hyeonwoonoh add
 */
template <typename Dtype>
class SegBinaryCtrlLayer : public ImageSegCtrlPrefetchingDataLayer<Dtype> {
 public:
  explicit SegBinaryCtrlLayer(const LayerParameter& param)
    : ImageSegCtrlPrefetchingDataLayer<Dtype>(param) {}
  virtual ~SegBinaryCtrlLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> transformed_label_;
  Blob<Dtype> transformed_ctrl_data_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  typedef struct SegItems {
    std::string imgfn;
    std::string segfn;
    int x1,   y1,  x2,  y2;
    int tx1, ty1, tx2, ty2;
    int inst_label;
    int cls_label;
  } SEGITEMS;

  vector<SEGITEMS> lines_;
  int lines_id_;
};
template <typename Dtype>
class SelectSegBinaryLayer : public ImageDimPrefetchingDataLayer<Dtype> {
 public:
  explicit SelectSegBinaryLayer(const LayerParameter& param)
    : ImageDimPrefetchingDataLayer<Dtype>(param) {}
  virtual ~SelectSegBinaryLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> transformed_label_;
  Blob<Dtype> class_label_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  typedef struct SegItems {
    std::string imgfn;
    std::string segfn;
    int x1, y1, x2, y2;
    vector<int> cls_label;
  } SEGITEMS;

  vector<SEGITEMS> lines_;
  int lines_id_;
  int label_dim_;
};


template <typename Dtype>
class WindowClsDataLayer : public ImageDimPrefetchingDataLayer<Dtype> {
 public:
  explicit WindowClsDataLayer(const LayerParameter& param)
    : ImageDimPrefetchingDataLayer<Dtype>(param) {}
  virtual ~WindowClsDataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> seg_label_buffer_;
  Blob<Dtype> transformed_label_;
  Blob<Dtype> computed_label_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  typedef struct SegItems {
    std::string imgfn;
    std::string segfn;
    int x1, y1, x2, y2;
  } SEGITEMS;

  vector<SEGITEMS> lines_;
  int lines_id_;
  int label_dim_;
};

template <typename Dtype>
class WindowInstSegDataLayer : public ImageDimPrefetchingDataLayer<Dtype> {
 public:
  explicit WindowInstSegDataLayer(const LayerParameter& param)
    : ImageDimPrefetchingDataLayer<Dtype>(param) {}
  virtual ~WindowInstSegDataLayer();
  virtual void DataLayerSetUp(const vector<Blob<Dtype>*>& bottom,
      const vector<Blob<Dtype>*>& top);

  virtual inline LayerParameter_LayerType type() const {
    return LayerParameter_LayerType_IMAGE_DATA;
  }
  virtual inline int ExactNumBottomBlobs() const { return 0; }
  virtual inline int ExactNumTopBlobs() const { return 3; }
  virtual inline bool AutoTopBlobs() const { return true; }

 protected:
  virtual void ShuffleImages();
  virtual void InternalThreadEntry();

 protected:
  Blob<Dtype> transformed_label_;

  shared_ptr<Caffe::RNG> prefetch_rng_;

  typedef struct InstItems {
    std::string imgfn;
    std::string segfn;
    std::string instfn;
    int x1, y1, x2, y2, inst_label;
  } INSTITEMS;

  vector<INSTITEMS> lines_;
  int lines_id_;
};

}  // namespace caffe

#endif  // CAFFE_DATA_LAYERS_HPP_
