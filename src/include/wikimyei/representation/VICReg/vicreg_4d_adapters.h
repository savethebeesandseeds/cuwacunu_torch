/* vicreg_4d_adapters.h */
#pragma once

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

template<
  typename Datasample_t,
  std::enable_if_t<
    std::is_same_v<Datasample_t, cuwacunu::camahjucunu::data::observation_sample_t>, int
  >
>
Datasample_t VICReg_4D::encode(
  Datasample_t batch,
  bool use_swa,
  bool detach_to_cpu)
{
  batch.encoding = this->encode(
    batch.features,
    batch.mask,
    use_swa,
    detach_to_cpu
  );
  return batch;
}

template<
  typename Datasample_t,
  std::enable_if_t<
    std::is_same_v<Datasample_t, cuwacunu::camahjucunu::data::observation_sample_t>, int
  >
>
torch::Tensor VICReg_4D::encode_projected(
  const Datasample_t& batch,
  bool use_swa,
  bool detach_to_cpu)
{
  return this->encode_projected(
    batch.features,
    batch.mask,
    use_swa,
    detach_to_cpu
  );
}

template<typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler>
RepresentationDataloaderView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t, Sampler>
VICReg_4D::make_representation_dataloader(
  cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler>& raw_loader,
  bool use_swa,
  bool debug)
{
  return RepresentationDataloaderView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t, Sampler>(raw_loader, *this, use_swa, debug);
}

template<typename Dataset_t, typename Datasample_t, typename Datatype_t>
RepresentationDatasetView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t>
VICReg_4D::make_representation_dataset(
  Dataset_t& raw_dataset,
  torch::Device device,
  bool use_swa,
  bool detach_to_cpu)
{
  return RepresentationDatasetView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t>(raw_dataset, *this, device, use_swa, detach_to_cpu);
}

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
