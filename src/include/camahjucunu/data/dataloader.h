
/*
  create this class to hable just one file (kline type)

  create a composite dataset to join them base of a BNF instruction
  
  Fix the irregular skips:
    - binary datafile should be correct, interpolation search is optimal only if the data distribution is uniform
    use std::numeric_limits<double>::quiet_NaN() for missing values
      then torch::Tensor mask = data.isnan().logical_not().to(torch::kFloat32);
  
  if no exact value is found, make sure to use the correct timestamp, do not allow the model to look into the future

  if missed values are included: 
    (data augmentation via random masking): during training, repeat the sequences with mask at random to train the model in arbitrary missing values
        Normalization Layers: Be cautious with layers like Batch Normalization, as the statistics can be affected by masked inputs. Consider alternatives like Layer Normalization.

    hours knlines have much less probability of mask than minutes klines for instance

  learning:
    Curriculum Learning: first learn from full data, then learn from masks
    dont use droput or standard variation
    - L2 regularization is alright
    - srink and perturb is also alright
    Continual Backpropagation (Richard Sutton):
      - inspecting the network for the neurons that are beeing activated (utility mesarures) the less and reinitializing their parameters is nice. 
    
  Architrecute:
    - use of residual networks ResNets


  note, maybe usefull padded sequences
    auto packed_input = torch::nn::utils::rnn::pack_padded_sequence(inputs, lengths, batch_first=>true);
*/
